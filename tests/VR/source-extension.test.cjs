const {test}=require('node:test');
const assert=require('node:assert/strict');
const fs=require('node:fs');
const vm=require('node:vm');
const path=require('node:path');
const movieSource=fs.readFileSync(path.join(__dirname,'../../ui/VR/SourceExtension/RaceMenuVR2Extension.as'),'utf8');
const source=movieSource.split('// BEGIN LIFECYCLE\n')[1].split('// END LIFECYCLE')[0];
function fixture() {
  const context={}; vm.createContext(context); vm.runInContext(source,context);
  const calls=[];
  const owner={modeSelect:{},SetSliders(...args){calls.push([this,...args]);return 7;},
    SetCategoriesList(){},SetRaceList(){},onItemPress(){return 9;}};
  const module={_parent:owner};
  for(const name of source.matchAll(/^function (\w+)\(/gm)) module[name[1]]=context[name[1]];
  return {module,owner,calls};
}
test('attach preserves callback receiver, arguments and return; repeat is idempotent',()=>{
  const {module,owner,calls}=fixture(); const original=owner.SetSliders;
  assert.equal(module.SourceExtensionAttach(owner),true);
  const wrapper=owner.SetSliders, revision=owner.sourceExtensionRevision;
  assert.equal(module.SourceExtensionAttach(owner),true); assert.equal(owner.SetSliders,wrapper);
  assert.equal(owner.SetSliders(1,'two'),7); assert.deepEqual(calls,[[owner,1,'two']]);
  assert.equal(owner.sourceExtensionRevision,revision+1);
  module.SourceExtensionDetach(); assert.equal(owner.SetSliders,original);
  assert.equal(owner.sourceExtensionStatus,'detached');
});
test('detach preserves a later wrapper and retained callbacks still delegate',()=>{
  const {module,owner,calls}=fixture(); module.SourceExtensionAttach(owner);
  const ours=owner.SetSliders; const later=function(...args){return ours.apply(this,args);};
  owner.SetSliders=later; module.SourceExtensionDetach();
  assert.equal(owner.SetSliders,later); const revision=owner.sourceExtensionRevision;
  assert.equal(owner.SetSliders(3),7); assert.equal(calls.length,1);
  assert.equal(owner.sourceExtensionRevision,revision);
});
test('only one module owns the lease, and attachment can be renewed after unload',()=>{
  const {module,owner}=fixture(), other=fixture().module;
  assert.equal(module.SourceExtensionAttach(owner),true);
  assert.equal(other.SourceExtensionAttach(owner),false);
  module.SourceExtensionDetach(); assert.equal(other.SourceExtensionAttach(owner),true);
});
test('unsupported owner has a bounded retry and is not modified',()=>{
  const {module}=fixture(); const owner={}; module._parent=owner;
  module.SourceExtensionStart(); for(let i=0;i<120;i++) module.SourceExtensionTick();
  assert.equal(module.sourceExtensionStatus,'unsupported-owner');
  assert.equal(module.onEnterFrame,undefined); assert.deepEqual(owner,{});
});
test('late owner readiness attaches and stops retries',()=>{
  const {module,owner}=fixture(); module._parent=undefined;
  module.SourceExtensionStart(); module.SourceExtensionTick();
  module._parent=owner; module.SourceExtensionTick();
  assert.equal(owner.sourceExtensionStatus,'attached'); assert.equal(module.onEnterFrame,undefined);
  module.onUnload(); assert.equal(owner.sourceExtensionStatus,'detached');
});

test('throwing invalidation cannot suppress originals or interrupt detach',()=>{
  const {module,owner,calls}=fixture();
  owner.InvalidateDiagnosticControls=()=>{throw Error('optional failure');};
  const original=owner.SetSliders;
  assert.equal(module.SourceExtensionAttach(owner),true);
  assert.equal(owner.SetSliders(4),7); assert.deepEqual(calls,[[owner,4]]);
  assert.equal(owner.sourceExtensionNotificationStatus,'invalidation-failed');
  assert.equal(owner.sourceExtensionInvalidating,undefined);
  module.SourceExtensionDetach();
  assert.equal(owner.SetSliders,original);
  assert.equal(owner.sourceExtensionLease,undefined);
  assert.equal(module.sourceExtensionLease,undefined);
  assert.equal(owner.sourceExtensionStatus,'detached');
  assert.equal(fixture().module.SourceExtensionAttach(owner),true);
});

test('original exceptions remain intact when optional invalidation throws',()=>{
  const {module,owner}=fixture(); const error=Error('original failure');
  let invoked=0;
  owner.SetSliders=function(){invoked++;throw error;};
  owner.InvalidateDiagnosticControls=()=>{throw Error('optional failure');};
  module.SourceExtensionAttach(owner);
  assert.throws(()=>owner.SetSliders(),e=>e===error);
  assert.equal(invoked,1);
});

test('invalidation can re-enter a wrapped method without recursive notification',()=>{
  const {module,owner,calls}=fixture(); let notified=0;
  module.SourceExtensionAttach(owner);
  owner.InvalidateDiagnosticControls=()=>{notified++;owner.SetSliders('nested');};
  assert.equal(owner.SetSliders('outer'),7);
  assert.equal(notified,1);
  assert.deepEqual(calls,[[owner,'nested'],[owner,'outer']]);
  assert.equal(owner.sourceExtensionInvalidating,undefined);
});

test('detach notification sees released ownership and cannot reacquire or recurse',()=>{
  const {module,owner}=fixture(); let notified=0,observed;
  module.SourceExtensionAttach(owner);
  owner.InvalidateDiagnosticControls=()=>{
    notified++;
    const ownerLease=owner.sourceExtensionLease,childLease=module.sourceExtensionLease;
    module.SourceExtensionDetach();
    observed={ownerLease,childLease,reattached:module.SourceExtensionAttach(owner)};
  };
  module.SourceExtensionDetach(); assert.equal(notified,1);
  assert.deepEqual(observed,{ownerLease:undefined,childLease:undefined,reattached:false});
  assert.equal(module.sourceExtensionDetaching,undefined);
  delete owner.InvalidateDiagnosticControls;
  assert.equal(module.SourceExtensionAttach(owner),true);
});

test('invalidation-triggered detach still delegates the captured original',()=>{
  const {module,owner,calls}=fixture();module.SourceExtensionAttach(owner);
  owner.InvalidateDiagnosticControls=()=>module.SourceExtensionDetach();
  assert.equal(owner.SetSliders(6),7);assert.deepEqual(calls,[[owner,6]]);
  assert.equal(owner.sourceExtensionLease,undefined);
  assert.equal(module.sourceExtensionLease,undefined);
});

test('attach notification cannot orphan a lease and reports synchronous detach',()=>{
  const {module,owner}=fixture();const other=fixture().owner;let nested;
  owner.InvalidateDiagnosticControls=()=>{
    nested=module.SourceExtensionAttach(other);
    module.SourceExtensionDetach();
  };
  assert.equal(module.SourceExtensionAttach(owner),false);
  assert.equal(nested,false);assert.equal(owner.sourceExtensionLease,undefined);
  assert.equal(module.sourceExtensionLease,undefined);
  assert.equal(other.sourceExtensionLease,undefined);
  delete owner.InvalidateDiagnosticControls;
  assert.equal(module.SourceExtensionAttach(owner),true);
});

test('one child cannot orphan its first owner by attaching to another',()=>{
  const {module,owner}=fixture(); const other=fixture().owner;
  const originalA=owner.SetSliders,originalB=other.SetSliders;
  module.SourceExtensionAttach(owner); const lease=module.sourceExtensionLease;
  assert.equal(module.SourceExtensionAttach(other),false);
  assert.equal(module.sourceExtensionLease,lease);
  assert.equal(other.SetSliders,originalB);assert.equal(other.sourceExtensionLease,undefined);
  module._parent=other;module.SourceExtensionStart();module.SourceExtensionTick();
  assert.equal(module.sourceExtensionLease,lease);
  module.SourceExtensionDetach();assert.equal(owner.SetSliders,originalA);
  assert.equal(owner.sourceExtensionLease,undefined);
  assert.equal(module.SourceExtensionAttach(other),true);
  module.SourceExtensionDetach();assert.equal(other.SetSliders,originalB);
});

test('inherited methods reveal the current prototype after detach',()=>{
  const {module}=fixture();const inherited=fixture().owner;
  const owner=Object.create(inherited);owner.modeSelect={};
  const original=inherited.SetSliders;
  assert.equal(Object.hasOwn(owner,'SetSliders'),false);
  module.SourceExtensionAttach(owner);
  assert.equal(Object.hasOwn(owner,'SetSliders'),true);
  const replacement=function(){return 42;};inherited.SetSliders=replacement;
  module.SourceExtensionDetach();
  assert.equal(Object.hasOwn(owner,'SetSliders'),false);
  assert.equal(owner.SetSliders,replacement);assert.equal(owner.SetSliders(),42);
  assert.notEqual(owner.SetSliders,original);
});

test('retained wrapper states release child, owner and full lease references',()=>{
  const {module,owner,calls}=fixture();module.SourceExtensionAttach(owner);
  const lease=module.sourceExtensionLease,state=lease.states.SetSliders,ours=owner.SetSliders;
  const later=function(...args){return ours.apply(this,args);};owner.SetSliders=later;
  module.SourceExtensionDetach();
  for(const field of ['module','owner','lease']) assert.equal(state[field],undefined);
  for(const field of ['module','owner','states','wrappers']) assert.equal(lease[field],undefined);
  assert.equal(state.active,false);assert.equal(owner.SetSliders,later);
  assert.equal(owner.SetSliders(5),7);assert.deepEqual(calls,[[owner,5]]);
});

test('optional race-list callback can be absent without changing the owner shape',()=>{
  const {module,owner}=fixture();delete owner.SetRaceList;
  assert.equal(module.SourceExtensionAttach(owner),true);
  assert.equal(Object.hasOwn(owner,'SetRaceList'),false);
  module.SourceExtensionDetach();assert.equal(Object.hasOwn(owner,'SetRaceList'),false);
});
