// Execute the actual production AS2 bodies with a controlled keyboard/delegate.
// This does not test GFx event delivery, OpenVR or an in-headset keyboard.
const test = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const source = fs.readFileSync(path.join(__dirname,'../tools/vr-racesex-patches/TextEntry.as.inc'),'utf8');
const prototype = Function('return ({'+source.replace(/^   function (\w+)\(/gm,'$1(').replace(/^   }[ \t]*$/gm,'},')+'\n})')();
const methods = ['RequestVRCharacterCreationFinish','FinishVRCharacterCreation','BeginVRTextInput',
  'PollVRTextInput','CancelVRTextInput','ShowVRTextDraft'];
if (process.env.SKEE_REBUILT_RACEMENU_AS) {
  const rebuilt = fs.readFileSync(process.env.SKEE_REBUILT_RACEMENU_AS,'utf8');
  for (const name of methods) {
    const signature=rebuilt.indexOf('function '+name+'(');
    assert.ok(signature>=0,'compiled method '+name);
    assert.equal(rebuilt.indexOf('function '+name+'(',signature+1),-1,'duplicate compiled method '+name);
    const start=rebuilt.indexOf('{',signature);
    const parameters=rebuilt.slice(signature,start).match(/\(([^)]*)\)/)[1];
    let end=start+1, depth=1;
    for (;depth && end<rebuilt.length;end++) {
      if (rebuilt[end]==='{') depth++;
      if (rebuilt[end]==='}') depth--;
    }
    assert.equal(depth,0,'compiled body '+name);
    prototype[name]=Function(parameters,rebuilt.slice(start+1,end-1));
  }
}
let starts, finishes, renames, cancels, polls, outcome, beginOutcome;
const api={
  BeginVRTextEntry(kind,initial){starts.push([kind,initial]);return beginOutcome;},
  PollVRTextEntry(id){polls.push(id);return outcome;},
  CancelVRTextEntry(id){cancels.push(id);},
  SetCharacterName(name){renames.push(name);return true;}
};
global._global={skse:{plugins:{CharGen:api}}};
global.Shared={GlobalFunc:{StringTrim:value=>value.trim()}};
global.gfx={io:{GameDelegate:{call(method,args){finishes.push([method,args]);}}}};
global.skyui={components:{SearchWidget:{S_FILTER:'$FILTER'}}};
global.setInterval=()=>7;
global.clearInterval=()=>{};
function fixture() {
  return Object.assign(Object.create(prototype),{
    bottomBar:{playerInfo:{PlayerName:{text:'Prisoner'}}},
    vrNameButton:{label:'Name: Prisoner',textField:{}},
    vrFilterButton:{label:'$FILTER',textField:{}},
    colorField:{_visible:false},textEntry:{_visible:false},makeupPanel:{_visible:false},
    navPanel:{updateButtons(){}},_nameFilter:{filterText:''},
    searchWidget:{isDisabled:false,textField:{SetText(text){this.text=text;}},dispatchEvent(event){this.event=event;}},
    setStatusText(text){this.status=text;},updateBottomBar(){},
    SetNameText(text){this.bottomBar.playerInfo.PlayerName.text=text;this.vrNameButton.label='Name: '+text;}
  });
}
test.beforeEach(()=>{
  starts=[];finishes=[];renames=[];cancels=[];polls=[];
  outcome={status:'pending',text:''};beginOutcome={status:'pending',id:12,error:''};
  delete api.promptForNameOnExit;
});
test('missing/off option preserves current finish with no keyboard',()=>{
  for (const value of [undefined,false,0]) {
    api.promptForNameOnExit=value;
    assert.equal(fixture().RequestVRCharacterCreationFinish(),true);
  }
  assert.deepEqual(starts,[]);
  assert.deepEqual(finishes,Array(3).fill(['ChangeName',['Prisoner']]));
});
test('original finish request prompts once; acceptance finishes once without intermediate rename',()=>{
  api.promptForNameOnExit=true;
  const owner=fixture();owner.bottomBar.playerInfo.PlayerName.text='Configured Hero';
  assert.equal(owner.RequestVRCharacterCreationFinish(),true);
  assert.deepEqual(starts,[['name','Configured Hero']]);
  assert.equal(owner.RequestVRCharacterCreationFinish(),false);
  outcome={status:'pending',text:'draft'};owner.PollVRTextInput();
  assert.deepEqual(finishes,[]);assert.deepEqual(renames,[]);
  outcome={status:'accepted',text:'  Finished Hero  '};owner.PollVRTextInput();
  assert.deepEqual(finishes,[['ChangeName',['Finished Hero']]]);
  assert.deepEqual(renames,[]);assert.equal(starts.length,1);
  assert.equal(owner.vrTextInputFinishAfterName,false);
  owner.PollVRTextInput();assert.equal(finishes.length,1);assert.deepEqual(polls,[12,12]);
});
test('cancellation, timeout and invalid accepted name preserve name and leave menu open',()=>{
  api.promptForNameOnExit=true;
  for (const result of [{status:'cancelled',text:'discard'},
                        {status:'unavailable',error:'keyboard_timeout'},
                        {status:'accepted',text:'   '}]) {
    const owner=fixture();owner.RequestVRCharacterCreationFinish();
    outcome=result;owner.PollVRTextInput();
    assert.deepEqual(finishes,[]);assert.deepEqual(renames,[]);
    assert.equal(owner.bottomBar.playerInfo.PlayerName.text,'Prisoner');
    assert.equal(owner.vrTextInputActive,false);
    assert.equal(owner.vrTextInputFinishAfterName,false);
  }
});
test('ordinary Name click after cancelled finish still renames without closing',()=>{
  api.promptForNameOnExit=true;
  const owner=fixture();owner.RequestVRCharacterCreationFinish();
  outcome={status:'cancelled'};owner.PollVRTextInput();
  owner.onVRNameButtonClicked();
  assert.equal(owner.vrTextInputFinishAfterName,false);
  outcome={status:'accepted',text:'Keep Editing'};owner.PollVRTextInput();
  assert.deepEqual(renames,['Keep Editing']);assert.deepEqual(finishes,[]);
  assert.equal(owner.bottomBar.playerInfo.PlayerName.text,'Keep Editing');
});
test('Filter cannot acquire finish intent, including accidental finish argument',()=>{
  api.promptForNameOnExit=true;
  const owner=fixture();assert.equal(owner.BeginVRTextInput('filter',true),true);
  assert.equal(owner.vrTextInputFinishAfterName,false);
  outcome={status:'accepted',text:'hair'};owner.PollVRTextInput();
  assert.deepEqual(owner.searchWidget.event,{type:'inputEnd',data:'hair'});
  assert.deepEqual(finishes,[]);assert.deepEqual(renames,[]);
});
test('keyboard refusal or missing API does not finish or set active intent',()=>{
  api.promptForNameOnExit=true;
  beginOutcome={status:'unavailable',error:'keyboard_busy',id:0};
  const owner=fixture();assert.equal(owner.RequestVRCharacterCreationFinish(),false);
  assert.ok(!owner.vrTextInputActive && !owner.vrTextInputFinishAfterName);
  const saved=api.BeginVRTextEntry;delete api.BeginVRTextEntry;
  try {assert.equal(fixture().RequestVRCharacterCreationFinish(),false);}
  finally {api.BeginVRTextEntry=saved;}
  assert.deepEqual(finishes,[]);assert.deepEqual(renames,[]);
});
test('explicit cancel/unload clears finish intent and late polls are inert',()=>{
  api.promptForNameOnExit=true;
  const owner=fixture();owner.SetupVRTextButtons();owner.RequestVRCharacterCreationFinish();
  owner.onUnload();
  assert.deepEqual(cancels,[12]);assert.equal(owner.vrTextInputFinishAfterName,false);
  outcome={status:'accepted',text:'Late'};owner.PollVRTextInput();
  assert.deepEqual(finishes,[]);assert.deepEqual(renames,[]);assert.deepEqual(polls,[]);
});
test('modal/diagnostic input guards refuse an exit prompt',()=>{
  api.promptForNameOnExit=true;
  for (const key of ['colorField','textEntry','makeupPanel']) {
    const owner=fixture();owner[key]._visible=true;
    assert.equal(owner.RequestVRCharacterCreationFinish(),false);
  }
  for (const key of ['vrInputTraceArmed','vrStockButtonAssayActive']) {
    const owner=fixture();owner[key]=true;
    assert.equal(owner.RequestVRCharacterCreationFinish(),false);
  }
  assert.deepEqual(starts,[]);assert.deepEqual(finishes,[]);
});
