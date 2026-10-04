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

function assertReleased(module,owner) {
  assert.equal(module.sourceExtensionLease,undefined);
  assert.equal(module.sourceExtensionAttaching,undefined);
  assert.equal(module.sourceExtensionDetaching,undefined);
  assert.equal(owner.sourceExtensionLease,undefined);
}

test('throwing second-method setter rolls back the first wrapper and permits restart',()=>{
  const {module,owner}=fixture(),original=owner.SetSliders;
  let value=owner.SetCategoriesList,fail=true;
  Object.defineProperty(owner,'SetCategoriesList',{configurable:true,get(){return value;},
    set(next){if(fail)throw Error('setter');value=next;}});
  assert.equal(module.SourceExtensionAttach(owner),false);
  assert.equal(owner.SetSliders,original);assertReleased(module,owner);
  assert.equal(module.sourceExtensionStatus,'attachment-failed');
  fail=false;assert.equal(module.SourceExtensionAttach(owner),true);
  module.SourceExtensionDetach();assertReleased(module,owner);
});

test('silently refused wrapper assignment fails admission and can be repaired',()=>{
  const {module,owner}=fixture(),original=owner.SetSliders,category=owner.SetCategoriesList;
  Object.defineProperty(owner,'SetCategoriesList',{value:category,writable:false,configurable:true});
  assert.equal(module.SourceExtensionAttach(owner),false);
  assert.equal(owner.SetSliders,original);assertReleased(module,owner);
  Object.defineProperty(owner,'SetCategoriesList',{writable:true});
  assert.equal(module.SourceExtensionAttach(owner),true);module.SourceExtensionDetach();
});

test('setter re-entry cannot create a second transaction and setup detach cancels',()=>{
  const {module,owner}=fixture(),other=fixture().owner;
  let value=owner.SetSliders,nested,assignments=0,cancel=false;
  Object.defineProperty(owner,'SetSliders',{configurable:true,get(){return value;},set(next){
    assignments++;nested=module.SourceExtensionAttach(other);value=next;
    if(cancel)module.SourceExtensionDetach();
  }});
  assert.equal(module.SourceExtensionAttach(owner),true);assert.equal(nested,false);
  assert.equal(assignments,1);assert.equal(other.sourceExtensionLease,undefined);
  module.SourceExtensionDetach();
  cancel=true;assert.equal(module.SourceExtensionAttach(owner),false);assertReleased(module,owner);
  cancel=false;assert.equal(module.SourceExtensionAttach(owner),true);module.SourceExtensionDetach();
});

test('owner getter exceptions and getter attach re-entry clear the early guard',()=>{
  const {module,owner}=fixture();let fail=true,nested;
  Object.defineProperty(owner,'modeSelect',{configurable:true,get(){
    nested=module.SourceExtensionAttach(owner);if(fail)throw Error('getter');return {};
  }});
  assert.equal(module.SourceExtensionAttach(owner),false);assert.equal(nested,false);
  assertReleased(module,owner);fail=false;
  assert.equal(module.SourceExtensionAttach(owner),true);module.SourceExtensionDetach();
});

test('metadata publication refusal restores pre-existing status and schema',()=>{
  const {module,owner}=fixture(),original=owner.SetSliders;
  owner.sourceExtensionStatus='before';
  Object.defineProperty(owner,'sourceExtensionSchema',{value:17,writable:false,configurable:true});
  assert.equal(module.SourceExtensionAttach(owner),false);assertReleased(module,owner);
  assert.equal(owner.SetSliders,original);assert.equal(owner.sourceExtensionStatus,'before');
  assert.equal(owner.sourceExtensionSchema,17);
  Object.defineProperty(owner,'sourceExtensionSchema',{writable:true});
  assert.equal(module.SourceExtensionAttach(owner),true);module.SourceExtensionDetach();
});

test('later setter invalidating an earlier publication cannot report attachment success',()=>{
  const {module,owner}=fixture();let schema;
  Object.defineProperty(owner,'sourceExtensionSchema',{configurable:true,get(){return schema;},
    set(value){schema=value;if(value===1)owner.sourceExtensionLease=undefined;}});
  assert.equal(module.SourceExtensionAttach(owner),false);assertReleased(module,owner);
  Object.defineProperty(owner,'sourceExtensionSchema',{value:undefined,writable:true,configurable:true});
  assert.equal(module.SourceExtensionAttach(owner),true);module.SourceExtensionDetach();
});

test('throwing restoration retires every state, reports incomplete detach and permits repaired restart',()=>{
  const {module,owner,calls}=fixture();const original=owner.SetSliders,category=owner.SetCategoriesList;
  let value=original,fail=false;
  Object.defineProperty(owner,'SetSliders',{configurable:true,get(){return value;},
    set(next){if(fail)throw Error('restore');value=next;}});
  module.SourceExtensionAttach(owner);
  const lease=module.sourceExtensionLease,state=lease.states.SetSliders;
  fail=true;module.SourceExtensionDetach();assertReleased(module,owner);
  assert.equal(module.sourceExtensionStatus,'detach-incomplete');
  assert.equal(owner.SetCategoriesList,category);
  assert.equal(state.active,false);
  for(const field of ['module','owner','lease'])assert.equal(state[field],undefined);
  for(const field of ['module','owner','states','wrappers','metadata'])assert.equal(lease[field],undefined);
  assert.equal(owner.SetSliders(21),7);assert.deepEqual(calls,[[owner,21]]);
  fail=false;owner.SetSliders=original;
  assert.equal(module.SourceExtensionAttach(owner),true);module.SourceExtensionDetach();
});

test('non-deletable inherited override preserves inactive delegation and releases remaining methods',()=>{
  const {module,calls}=fixture(),prototype=fixture().owner;
  const owner=Object.create(prototype);owner.modeSelect={};
  module.SourceExtensionAttach(owner);const wrapper=owner.SetSliders;
  Object.defineProperty(owner,'SetSliders',{value:wrapper,writable:true,configurable:false});
  module.SourceExtensionDetach();assertReleased(module,owner);
  assert.equal(module.sourceExtensionStatus,'detach-incomplete');
  assert.equal(Object.hasOwn(owner,'SetCategoriesList'),false);
  assert.equal(owner.SetSliders(),7);
  // Repair a writable but non-deletable slot, then own-slot restoration works.
  owner.SetSliders=prototype.SetSliders;
  assert.equal(module.SourceExtensionAttach(owner),true);module.SourceExtensionDetach();
  assert.equal(owner.SetSliders,prototype.SetSliders);assert.equal(calls.length,0);
});

test('undeletable owner lease fails closed without retaining the graph or stuck child guards',()=>{
  const {module,owner}=fixture();module.SourceExtensionAttach(owner);
  const lease=module.sourceExtensionLease;
  Object.defineProperty(owner,'sourceExtensionLease',{value:lease,writable:true,configurable:false});
  module.SourceExtensionDetach();assert.equal(module.sourceExtensionStatus,'detach-incomplete');
  assert.equal(module.sourceExtensionLease,undefined);assert.equal(module.sourceExtensionDetaching,undefined);
  assert.equal(lease.active,false);
  for(const field of ['module','owner','states','wrappers','metadata'])assert.equal(lease[field],undefined);
  assert.equal(module.SourceExtensionAttach(owner),false);
  owner.sourceExtensionLease=undefined;
  assert.equal(module.SourceExtensionAttach(owner),true);module.SourceExtensionDetach();
});

test('throwing lease getter at callback and detach cannot suppress originals or leave child stuck',()=>{
  const {module,owner,calls}=fixture();module.SourceExtensionAttach(owner);
  const lease=module.sourceExtensionLease;
  Object.defineProperty(owner,'sourceExtensionLease',{configurable:true,get(){throw Error('lease getter');}});
  assert.equal(owner.SetSliders(22),7);assert.deepEqual(calls,[[owner,22]]);
  module.SourceExtensionDetach();assert.equal(module.sourceExtensionStatus,'detach-incomplete');
  assert.equal(module.sourceExtensionLease,undefined);assert.equal(module.sourceExtensionDetaching,undefined);
  assert.equal(lease.owner,undefined);delete owner.sourceExtensionLease;
  assert.equal(module.SourceExtensionAttach(owner),true);module.SourceExtensionDetach();
});

test('partial setter mutation and failed rollback retire orphan wrappers and report incompleteness',()=>{
  const {module,owner}=fixture(),category=owner.SetCategoriesList;
  let value=category,fail=true,retained;
  Object.defineProperty(owner,'SetCategoriesList',{configurable:true,get(){return value;},set(next){
    if(fail){value=next;retained=module.sourceExtensionLease;throw Error('partial mutation');}
    value=next;
  }});
  assert.equal(module.SourceExtensionAttach(owner),false);assertReleased(module,owner);
  assert.equal(module.sourceExtensionStatus,'attachment-rollback-incomplete');
  assert.equal(retained.active,false);
  for(const field of ['module','owner','states','wrappers','metadata'])assert.equal(retained[field],undefined);
  fail=false;owner.SetCategoriesList=category;
  assert.equal(module.SourceExtensionAttach(owner),true);module.SourceExtensionDetach();
});

test('refused detach status reporting still releases the child guard and lease',()=>{
  const {module,owner}=fixture();module.SourceExtensionAttach(owner);
  Object.defineProperty(owner,'sourceExtensionStatus',{value:'attached',writable:false,configurable:true});
  module.SourceExtensionDetach();assertReleased(module,owner);
  assert.equal(module.sourceExtensionStatus,'detach-incomplete');
  Object.defineProperty(owner,'sourceExtensionStatus',{writable:true});
  assert.equal(module.SourceExtensionAttach(owner),true);module.SourceExtensionDetach();
});
