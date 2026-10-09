// SPDX-License-Identifier: GPL-3.0-or-later
// Executes production glue with a bounded JS surrogate, not AS2/GFx proof.
const test = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');
const path = require('node:path');
const read = name => fs.readFileSync(path.join(__dirname, '../tools/vr-racesex-patches', name), 'utf8');
const appearance = read('Appearance.as.inc');
const helpers = read('ViewCategoryControls.as.inc');
const extension = appearance.slice(appearance.indexOf('   function RefreshVRMenuExtensions()'));
const categoryPresentation = appearance.slice(appearance.indexOf('   function ApplyVRCategoryPresentation()'),appearance.indexOf('   function RequestVRMenuView()'));
const executed = helpers+'\n'+categoryPresentation+'\n'+extension;
function fixture() {
  const calls = [], context = {Math, Number, isNaN,
    RaceMenuDefines:{ENTRY_TYPE_CAT:1, ENTRY_TYPE_SLIDER:2, CATEGORY_RACE:2}};
  let revision = 1;
  const controls = [{id:'avatarYaw',label:'Avatar rotation',minimum:-180,maximum:180,step:1,value:0},
    {id:'viewYaw',label:'View direction',minimum:-60,maximum:60,step:1,value:0}];
  const sections = [{provider:'RaceMenuVR2',id:'view',flag:1<<20,order:100,label:'View',controls}];
  const presentation = {label:'',visible:-9999,order:-9999,controlIds:''};
  const api = {GetMenuExtensionsRevision:()=>revision,GetMenuExtensions:()=>sections,
    GetMenuCategoryPresentation:(_flag,provider)=>provider==='RaceMenuVR2' ? presentation : {label:'',visible:-9999,order:-9999,controlIds:''},SetMenuExtensionValue:(...args)=>calls.push(args)};
  context._global={skse:{plugins:{CharGen:api}}};
  vm.createContext(context); vm.runInContext(executed,context);
  const menu = {bMenuInitialized:true,invalidations:0,updates:0,
    InvalidateVRDiagnosticControls(){this.invalidations++;},
    categoryList:{entryList:[{flag:2044,enabled:true},{flag:2,enabled:true},{flag:4,enabled:true},{flag:8,enabled:true}],requestInvalidate(){}},
    itemList:{entryList:[{type:3,raceID:1,filterFlag:2},{type:2,text:'Body',filterFlag:4},{type:2,text:'Hair',filterFlag:8,priority:-5}],requestInvalidate(){menu.updates++;}}};
  for (const match of executed.matchAll(/^   function (\w+)\(/gm)) menu[match[1]]=context[match[1]];
  return {menu,calls,sections,controls,presentation,bump:()=>revision++,
    pair:()=>menu.itemList.entryList.filter(e=>menu.IsVRViewControl(e)),
    visible:flag=>menu.itemList.entryList.filter(e=>e.filterFlag & flag).sort((a,b)=>a.priority-b.priority)};
}
test('one native pair is first in every ordinary category and All, never Race',()=>{
  const f=fixture(), backing=f.menu.itemList.entryList, body=backing[1], hair=backing[2];
  f.menu.RefreshVRMenuExtensions(); const pair=f.pair();
  assert.equal(pair.length,2); assert.equal(backing[1],body); assert.equal(backing[2],hair);
  assert.equal(body.priority,0); assert.equal(hair.priority,-5);
  for(const flag of [4,8,2044,1<<20]) assert.deepEqual(f.visible(flag).slice(0,2),pair);
  assert.equal(f.visible(2).length,1); assert.equal(f.visible(2)[0].raceID,1);
  assert.equal(backing.filter(e=>e.vrNativeExtension).length,2);
});
test('normal widget callback preserves exact provider/control/value routing',()=>{
  const f=fixture(); f.menu.RefreshVRMenuExtensions(); const pair=f.pair();
  pair[0].internalCallback.call({entryObject:pair[0],position:37});
  pair[1].internalCallback.call({entryObject:pair[1],position:-12});
  assert.deepEqual(f.calls,[['RaceMenuVR2','avatarYaw',37],['RaceMenuVR2','viewYaw',-12]]);
  assert.equal(pair[0].GetTextureList(),null); assert.equal(pair[0].hasColor(),false);
});
test('steady refresh retains entry objects and does not invalidate snapshots or lists',()=>{
  const f=fixture(); f.menu.RefreshVRMenuExtensions(); const pair=f.pair();
  const before=[f.menu.invalidations,f.menu.updates];
  for(let i=0;i<5;i++) f.menu.RefreshVRMenuExtensions();
  assert.deepEqual(f.pair(),pair); assert.deepEqual([f.menu.invalidations,f.menu.updates],before);
});
test('category-only changes refresh masks without cloning or stale removed flags',()=>{
  const f=fixture(); f.menu.RefreshVRMenuExtensions(); const pair=f.pair();
  f.menu.categoryList.entryList[2].flag=16;
  let before=f.menu.invalidations; f.menu.RefreshVRMenuExtensions();
  assert.equal(f.menu.invalidations,before+1); assert.deepEqual(f.pair(),pair);
  assert.equal(pair[0].filterFlag & 4,0); assert.equal(pair[0].filterFlag & 16,16);
  f.menu.categoryList.entryList[2].enabled=false; f.menu.RefreshVRMenuExtensions();
  assert.equal(pair[0].filterFlag & 16,0);
  f.menu.categoryList.entryList[2].enabled=true; f.menu.categoryList.entryList[2].filterFlag=0;
  f.menu.RefreshVRMenuExtensions(); assert.equal(pair[0].filterFlag & 16,0);
});
test('race/sex item rebuild recreates only one pair and waits for ready state',()=>{
  const f=fixture(); f.menu.RefreshVRMenuExtensions(); const old=f.pair();
  f.menu.itemList.entryList=[]; f.menu.vrDiagnosticRebuilding=true;
  f.menu.RefreshVRMenuExtensions(); assert.equal(f.pair().length,0);
  f.menu.vrDiagnosticRebuilding=false; f.menu.RefreshVRMenuExtensions();
  assert.equal(f.pair().length,2); assert.notEqual(f.pair()[0],old[0]);
  f.menu.bRaceChanging=true; f.bump(); let before=f.menu.invalidations;
  f.menu.RefreshVRMenuExtensions(); assert.equal(f.menu.invalidations,before);
});
test('native control presentation and hidden View category remain respected',()=>{
  const f=fixture(); f.presentation.controlIds='viewYaw'; f.presentation.visible=0;
  f.menu.RefreshVRMenuExtensions(); assert.equal(f.pair().length,1);
  assert.equal(f.pair()[0].vrControl,'viewYaw'); assert.ok(f.visible(2044).includes(f.pair()[0]));
  let before=f.menu.invalidations; f.menu.RefreshVRMenuExtensions(); assert.equal(f.menu.invalidations,before);
  f.presentation.controlIds=''; f.bump(); f.menu.RefreshVRMenuExtensions(); assert.equal(f.pair().length,2);
});
test('unrelated extension rows and explicit priorities are not spread or overwritten',()=>{
  const f=fixture(); f.sections.push({provider:'other',id:'shape',flag:1<<21,order:101,label:'Shape',controls:[{id:'width',label:'Width',minimum:0,maximum:1,step:.1,value:.5}]});
  f.menu.RefreshVRMenuExtensions(); const other=f.menu.itemList.entryList.find(e=>e.vrProvider==='other');
  assert.equal(other.filterFlag,1<<21); assert.equal(other.priority,0);
  assert.deepEqual(f.visible(1<<21).slice(0,2),f.pair());
  other.priority=-30; f.menu.RefreshVRMenuExtensions();
  assert.equal(other.priority,-30); assert.equal(f.pair()[0].priority,-32);
});
test('without built-in controls, ordinary priorities and model are unchanged',()=>{
  const f=fixture(); f.sections.length=0; f.menu.RefreshVRMenuExtensions();
  assert.equal(f.menu.itemList.entryList[1].priority,undefined); assert.equal(f.pair().length,0);
});
test('removed panel leaves pre-panel geometry and stock input untouched',()=>{
  assert.doesNotMatch(appearance,/vrViewPanel|viewHeight|RefreshVRViewPanel|LayoutVRViewPanel/);
  assert.match(appearance,/bottomBar\._y = panel\.yMax-160;/);
  assert.doesNotMatch(helpers,/attachMovie|createEmptyMovieClip|setInterval|changedCallback|GameDelegate|SetAngle/);
  const recipe=fs.readFileSync(path.join(__dirname,'../tools/patch-vr-racesex-swf.ps1'),'utf8');
  assert.match(recipe,/ViewCategoryControls\.as\.inc/); assert.doesNotMatch(recipe,/ViewPanel\.as\.inc/);
});
