// SPDX-License-Identifier: GPL-3.0-or-later
// Source contracts and reference-model behaviour, not native policy execution.
const test = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const read = p => fs.readFileSync(path.join(__dirname, '..', p), 'utf8');
const policy = read('skee64/ViewDirectionTrialPolicy.h');
const face = read('skee64/RaceSexMenuFaceView.cpp');
const trace = read('skee64/RaceSexMenuVRInput.cpp');
const extensions = read('skee64/MenuExtensions.cpp');

test('quiet-window is explicit, finite, range-checked, latest-value and sign-inverted', () => {
  assert.match(policy, /quietMilliseconds = 400/);
  assert.match(policy, /!std::isfinite\(value\).*std::abs\(value\) > 60/);
  assert.match(policy, /requested = static_cast<float>\(value\); lastInput = now/);
  assert.match(policy, /now < lastInput \|\| now-lastInput < quietMilliseconds/);
  assert.match(policy, /applied != baseline.*Cancel\(\)/);
  assert.match(policy, /pending = false; \+\+commits/);
  assert.match(policy, /return -requested/);
});
test('reference model: burst, quiet boundary, duplicate requests, cancellation and final zero', () => {
  // Independent state-machine expectation used for next live assay.
  let pending = false, requested, baseline, last;
  const submit = (value, applied, now) => {
    if (!Number.isFinite(value) || Math.abs(value)>60 || !Number.isFinite(applied)) return false;
    if (!pending) baseline = applied;
    pending=true; requested=value; last=now; return true;
  };
  const take = (applied,now) => {
    if (!pending) return;
    if (applied!==baseline) { pending=false; return; }
    if (now<last || now-last<400) return;
    pending=false; return -requested;
  };
  assert.equal(submit(NaN,0,0),false); assert.equal(submit(61,0,0),false);
  submit(1,0,0); submit(2,0,100); submit(3,0,200);
  assert.equal(take(0,599),undefined); assert.equal(take(0,600),-3);
  assert.equal(take(0,999),undefined);
  submit(-5,0,1000); submit(-5,0,1300);
  assert.equal(take(0,1699),undefined); assert.equal(take(0,1700),5);
  submit(20,0,2000); assert.equal(take(5,2400),undefined);
  assert.equal(take(0,2500),undefined); submit(0,5,3000);
  assert.equal(take(5,2999),undefined); assert.equal(take(5,3400),-0);
});
test('native update owns commit; pending value is not overwritten and close cancels', () => {
  assert.match(face, /Register\(menu->uiMovie.get\(\)\);\s*ObserveAvatarLifecycle\(\);\s*CommitViewTrial\(\)/);
  assert.match(face, /if \(!viewTrial.pending\).*SetValue\(provider, "viewYaw", -FaceView::ViewYaw\(\)\)/);
  assert.match(face, /void Restore\(\)[\s\S]*viewTrial.Cancel\(\)/);
  assert.match(face, /refusal = nullptr; viewTrial = \{\}/);
  assert.doesNotMatch(policy, /thread|sleep|AddTask|GetSingleton|GFx/);
});
test('diagnostics preserve guards and copied snapshot; trace observes both callback boundaries', () => {
  for (const reason of ['ancestor-bounds-target','object-compose-target','object-fixed-bound','child-parent-mismatch','avatar-parent-frame'])
    assert.ok(face.includes('Fail("'+reason+'"'), reason);
  assert.match(face, /next.controls = CharacterInspection::CaptureDiagnostics\(\)/);
  assert.match(face, /result\["controls"\] = snapshot.controls/);
  assert.match(face, /quiet-window-not-release/);
  assert.match(extensions, /extension_gfx_request/);
  assert.match(extensions, /extension_game_callback/);
  assert.match(face, /view_trial_applied/);
  assert.match(trace, /RecordExtensionTrace[\s\S]*if \(!TraceActive\(movie\)\) return/);
  assert.match(trace, /requestedValue/);
  assert.match(trace, /appliedNativeYaw/);
});
