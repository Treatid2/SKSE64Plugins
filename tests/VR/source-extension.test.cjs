const {test}=require('node:test');
const assert=require('node:assert/strict');
const fs=require('node:fs');
const vm=require('node:vm');
const path=require('node:path');
const source=fs.readFileSync(path.join(__dirname,'../../ui/VR/SourceExtension/Lifecycle.as.inc'),'utf8');
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
