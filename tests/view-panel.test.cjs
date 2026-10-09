// SPDX-License-Identifier: GPL-3.0-or-later
// Execute original glue against a bounded AS2 surrogate; not GFx or native proof.
const test = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');
const path = require('node:path');
const read = name => fs.readFileSync(path.join(__dirname, '../tools/vr-racesex-patches', name), 'utf8');
const source = read('ViewPanel.as.inc');
const appearance = read('Appearance.as.inc');
const names = [...source.matchAll(/function (\w+)\(/g)].map(m => m[1]);
function fixture() {
  const sent = [], rows = [], cleared = [];
  const api = {GetMenuExtensionsRevision: () => 4, SetMenuExtensionValue: (...a) => sent.push(a)};
  const context = {Math, isNaN, Number, clearInterval: id => cleared.push(id),
    _global: {skse: {plugins: {CharGen: api}}}, RaceMenuDefines: {ENTRY_TYPE_SLIDER: 1}};
  vm.createContext(context);
  vm.runInContext(source, context);
  const owner = {bMenuInitialized:true, bRaceChanging:false, vrDiagnosticRebuilding:false,
    modeSelect: {getMode: () => 0}, racePanel: {_visible:true}, itemList: {},
    colorField: {}, makeupPanel:{}, textEntry:{}, vrMenuProfile:{consolidate:1}, layouts:0,
    LayoutVRPanel() {this.layouts++;}, getNextHighestDepth: () => 1,
    createEmptyMovieClip() {
      const panel = {getNextHighestDepth: () => 1, removeMovieClip() {this.removed=true;},
        attachMovie(linkage) {
          assert.equal(linkage, 'SliderListEntry');
          const row = {_parent:panel, SliderInstance:{initialized:true}, valueField:{SetText() {}},
            setEntry(entry) {this.entry=entry; this.SliderInstance.entryObject=entry;
              this.SliderInstance.entryClip=this; this.SliderInstance.position=entry.position; this.setCalls=(this.setCalls||0)+1;},
            removeMovieClip() {this.removed=true;}, getBounds: () => ({xMin:0,xMax:400,yMin:0,yMax:40})};
          rows.push(row); return row;
        }};
      return panel;
    }};
  for (const name of names) owner[name] = context[name];
  const controls = [{id:'avatarYaw',label:'Avatar rotation',minimum:-180,maximum:180,step:1,value:0},
    {id:'viewYaw',label:'View direction',minimum:-60,maximum:60,step:1,value:0}];
  const sections = [{provider:'RaceMenuVR2',id:'view',controls}];
  return {owner, rows, api, sent, sections, controls, cleared};
}
test('two stock rows are independent of category and stable through value/revision changes', () => {
  const f=fixture(); assert.equal(f.owner.RefreshVRViewPanel(f.sections,4),true);
  const panel=f.owner.vrViewPanel;
  assert.equal(f.rows.length,2);
  f.controls[0].value=90; f.owner.RefreshVRViewPanel(f.sections,5);
  assert.equal(f.owner.vrViewPanel,panel); assert.equal(f.rows.length,2);
  assert.equal(f.rows[0].SliderInstance.position,90); assert.equal(f.rows[0].setCalls,1);
  assert.equal(f.sent.length,0); assert.equal(f.rows[0].itemIndex,undefined);
});
test('same native callback route requires current revision and owner, no engine data writes', () => {
  const f=fixture(); f.owner.RefreshVRViewPanel(f.sections,4);
  const row=f.rows[0], widget=row.SliderInstance;
  widget.position=37; row.entry.internalCallback.call(widget);
  assert.deepEqual(f.sent,[['RaceMenuVR2','avatarYaw',37]]);
  f.api.GetMenuExtensionsRevision=()=>5;
  row.entry.internalCallback.call(widget); assert.equal(f.sent.length,1);
  f.api.GetMenuExtensionsRevision=()=>4; f.owner.ReleaseVRViewPanel();
  row.entry.internalCallback.call(widget); assert.equal(f.sent.length,1);
});
test('race rebuild, every modal and non-sliders modes block input immediately', () => {
  for (const field of ['bRaceChanging','vrDiagnosticRebuilding','vrTextInputActive','bTextEntryMode']) {
    const f=fixture(); f.owner.RefreshVRViewPanel(f.sections,4); f.owner[field]=true;
    f.rows[0].entry.internalCallback.call(f.rows[0].SliderInstance); assert.equal(f.sent.length,0,field);
    f.owner.RefreshVRViewPanel(f.sections,4); assert.equal(f.rows[0].SliderInstance.disabled,true,field);
  }
  for (const name of ['colorField','makeupPanel','textEntry']) {
    const f=fixture(); f.owner.RefreshVRViewPanel(f.sections,4); f.owner[name]._visible=true;
    f.rows[0].entry.internalCallback.call(f.rows[0].SliderInstance); assert.equal(f.sent.length,0,name);
  }
  const f=fixture(); f.owner.RefreshVRViewPanel(f.sections,4); f.owner.modeSelect.getMode=()=>3;
  f.rows[0].entry.internalCallback.call(f.rows[0].SliderInstance); assert.equal(f.sent.length,0);
  f.owner.RefreshVRViewPanel(f.sections,4); assert.equal(f.owner.vrViewPanel._visible,false);
});
test('missing renderer, missing native control and changed ranges retain fallback and release callbacks', () => {
  const f=fixture(); f.owner.RefreshVRViewPanel(f.sections,4);
  f.controls[0].maximum=200; assert.equal(f.owner.RefreshVRViewPanel(f.sections,5),false);
  assert.equal(f.owner.vrViewPanel,undefined); assert.ok(f.rows.every(r=>r.removed && r.vrOwner===undefined));
  const g=fixture(); g.sections[0].controls.pop(); assert.equal(g.owner.RefreshVRViewPanel(g.sections,4),false);
  const h=fixture(); h.owner.createEmptyMovieClip=()=>({rows:[],getNextHighestDepth:()=>1,attachMovie:()=>undefined,removeMovieClip(){}});
  assert.equal(h.owner.RefreshVRViewPanel(h.sections,4),false); assert.equal(h.owner.vrViewPanel,undefined);
  const k=fixture(); k.owner.vrMenuProfile.consolidate=0;
  assert.equal(k.owner.RefreshVRViewPanel(k.sections,4),false); assert.equal(k.rows.length,0);
});
test('panel stays inside existing surface with uniform row scaling and reserved footer height', () => {
  const f=fixture(); f.owner.RefreshVRViewPanel(f.sections,4);
  f.owner.LayoutVRViewPanel({xMin:10,xMax:350,yMin:0,yMax:800});
  assert.equal(f.owner.vrViewPanel._x,22); assert.equal(f.owner.vrViewPanel._y,692);
  assert.equal(f.rows[1]._y,48); assert.equal(f.rows[0]._xscale,f.rows[0]._yscale);
  assert.match(appearance,/ITEMLIST_HEIGHT_FULL -= viewHeight/);
  assert.match(appearance,/panel.yMax-160-viewHeight/);
  assert.match(appearance,/RefreshVRViewPanel\(sections,revision\)/);
  assert.match(appearance,/owner.ReleaseVRViewPanel\(\)/);
  assert.doesNotMatch(source,/setInterval|SetAngle|changedCallback\(|GameDelegate/);
});
