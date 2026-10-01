const {test} = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');
const path = require('node:path');
const source = fs.readFileSync(path.join(__dirname, '../tools/vr-racesex-patches/Diagnostics.as.inc'), 'utf8');
function fixture() {
  const calls = [];
  const context = {
    RaceMenuDefines: {ENTRY_TYPE_SLIDER:1, ENTRY_TYPE_RACE:2, STATIC_SLIDER_SEX:99, CATEGORY_RACE:2},
    _global: {eventPrefix:'test', skse:{IsVR:()=>true, plugins:{CharGen:{SetMenuExtensionValue:(...args)=>calls.push(['extension',...args])}}}},
    gfx:{io:{GameDelegate:{call:(name,args)=>calls.push([name,...args])}}},
    skse:{SendModEvent:()=>{}},
  };
  vm.createContext(context);
  vm.runInContext(source, context);
  const slider = (id=10) => ({type:1, sliderID:id,text:'Slider '+id,callbackName:'SetSlider',sliderMin:-1,sliderMax:10,interval:1,position:0,enabled:true,filterFlag:4});
  const menu = {
    modeSelect:{getMode:()=>0}, bMenuInitialized:true, racePanel:{_visible:true},
    colorField:{}, makeupPanel:{}, textEntry:{},
    categoryList:{entryList:[{flag:4,enabled:true,filterFlag:4},{flag:2,enabled:true,filterFlag:2}]},
    itemList:{entryList:[slider(),slider(11),{type:2,raceID:20,text:'Race',enabled:true}],listState:{},requestUpdate:()=>{}},
    updateItemDescriptor:()=>{},onItemPress:e=>calls.push(['race',e.index]),
  };
  for (const name of source.matchAll(/^   function (\w+)\(/gm)) menu[name[1]]=context[name[1]];
  const query=()=>{menu.RefreshVRDiagnosticControls();return menu.vrDiagnosticGeneration;};
  return {menu,calls,query,slider};
}
test('query and dispatch consume tokens and serialize the exact requested identity',()=>{
  const {menu,calls,query}=fixture();
  const generation=query();
  assert.equal(menu.SetVRDiagnosticSlider(generation,0,2),true);
  assert.deepEqual(calls[0],['SetSlider',2,10]);
  const result=JSON.parse(menu.vrDiagnosticResultJson);
  assert.equal(result.requestGeneration,generation);
  assert.equal(result.sliderId,10); assert.equal(result.slot,0); assert.equal(result.value,2);
  assert.equal(result.callback,'SetSlider'); assert.notEqual(result.generation,generation);
  assert.equal(menu.SetVRDiagnosticSlider(generation,0,3),false);
  assert.equal(menu.vrDiagnosticResult.code,'stale-generation');
  assert.equal(menu.SetVRDiagnosticSlider(query(),0,3),true);
});
test('ready snapshot is required and every query supersedes the previous query',()=>{
  const {menu,calls,query}=fixture();
  menu.InvalidateVRDiagnosticControls();
  assert.equal(menu.SetVRDiagnosticSlider(menu.vrDiagnosticGeneration,0,1),false);
  assert.equal(menu.vrDiagnosticResult.code,'snapshot-required');
  const old=query(); query();
  assert.equal(menu.SetVRDiagnosticSlider(old,0,1),false);assert.equal(calls.length,0);
});
for (const [name,mutate] of Object.entries({
  reorder:m=>m.itemList.entryList.reverse(),
  replace:m=>m.itemList.entryList[0]={...m.itemList.entryList[0]},
  id:m=>m.itemList.entryList[0].sliderID++,
  callback:m=>m.itemList.entryList[0].callbackName='Other',
  provider:m=>m.itemList.entryList[0].vrProvider='other',
  control:m=>m.itemList.entryList[0].vrControl='other',
  range:m=>m.itemList.entryList[0].sliderMax=9,
  step:m=>m.itemList.entryList[0].interval=0.5,
  manualValue:m=>m.itemList.entryList[0].position=1,
  disabled:m=>m.itemList.entryList[0].enabled=false,
  category:m=>m.categoryList.entryList[0].enabled=false,
  raceIdentity:m=>m.itemList.entryList[2].raceID=21,
})) test('rejects changed snapshot: '+name,()=>{
  const {menu,calls,query}=fixture();const generation=query();mutate(menu);
  assert.equal(menu.SetVRDiagnosticSlider(generation,0,2),false);
  assert.equal(menu.vrDiagnosticResult.code,'slider-identity-changed');assert.equal(calls.length,0);
});
test('refresh/reorder cannot re-enter an action with the consumed token',()=>{
  const {menu,calls,query}=fixture();const generation=query();let reentered;
  menu.itemList.requestUpdate=()=>{menu.itemList.entryList.reverse();reentered=menu.SetVRDiagnosticSlider(generation,0,3);};
  assert.equal(menu.SetVRDiagnosticSlider(generation,0,2),true);
  assert.equal(reentered,false);assert.equal(calls.filter(c=>c[0]==='SetSlider').length,1);
  assert.equal(JSON.parse(menu.vrDiagnosticResultJson).value,2);
});
test('extension, sex and race use menu callbacks and consume snapshots',()=>{
  const {menu,calls,query}=fixture();const entry=menu.itemList.entryList[0];
  Object.assign(entry,{vrNativeExtension:true,vrProvider:'p',vrControl:'c'});
  assert.equal(menu.SetVRDiagnosticSlider(query(),0,1),true);assert.deepEqual(calls[0],['extension','p','c',1]);
  Object.assign(entry,{vrNativeExtension:false,callbackName:'ChangeSex',sliderID:99});
  assert.equal(menu.SetVRDiagnosticSlider(query(),0,1),false);assert.equal(menu.vrDiagnosticResult.code,'use-select-sex');
  assert.equal(menu.SelectVRDiagnosticSex(query(),0,1),true);
  const generation=query();assert.equal(menu.SelectVRDiagnosticRace(generation,20),true);
  assert.deepEqual(calls.at(-1),['race',2]);
  const result=JSON.parse(menu.vrDiagnosticResultJson);assert.equal(result.raceId,20);assert.equal(result.requestGeneration,generation);
});
test('invalid values, inactive tabs, modals and rebuilding never dispatch',()=>{
  const {menu,calls,query}=fixture();
  for(const value of [-2,11,0.5,NaN,Infinity,'2']) assert.equal(menu.SetVRDiagnosticSlider(query(),0,value),false);
  menu.itemList.entryList[0].interval=Infinity;assert.equal(menu.SetVRDiagnosticSlider(query(),0,1),false);
  for(const [set,code] of [
    [()=>menu.modeSelect.getMode=()=>2,'sliders-tab-inactive'],
    [()=>{menu.modeSelect.getMode=()=>0;menu.textEntry._visible=true;},'modal-open'],
    [()=>{menu.textEntry._visible=false;menu.vrDiagnosticRebuilding=true;},'sliders-rebuilding'],
  ]) {set();assert.equal(menu.SetVRDiagnosticSlider(query(),0,1),false);assert.equal(menu.vrDiagnosticResult.code,code);}
  assert.equal(calls.length,0);
});
