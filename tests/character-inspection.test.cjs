// SPDX-License-Identifier: GPL-3.0-or-later
// Source-contract checks; not native execution or a live Scaleform assay.
const test = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const source = fs.readFileSync(path.join(__dirname, '../skee64/RaceSexMenuFaceView.cpp'), 'utf8');
function body(name) {
  const start = source.indexOf(name + '(');
  assert.ok(start >= 0, name);
  const open = source.indexOf('{', start);
  let depth = 1, end = open + 1;
  while (depth && end < source.length) {
    if (source[end] === '{') depth++;
    if (source[end] === '}') depth--;
    end++;
  }
  assert.equal(depth, 0);
  return source.slice(open + 1, end - 1);
}
test('capture is explicit, one-task bounded, movie/session checked and main-thread queued', () => {
  const capture = body('RequestInspection');
  assert.match(capture, /inspectionQueued.exchange\(true\)/);
  assert.match(capture, /tasks->AddTask/);
  assert.match(capture, /generation.load\(\) != session/);
  assert.match(capture, /menu->uiMovie.get\(\) != identity/);
  assert.match(capture, /IsMenuOpen/);
  assert.match(capture, /generation.load\(\) == session && inspection.serial == serial/);
  assert.match(capture, /catch \(\.\.\.\).*capture-failed/);
  assert.doesNotMatch(capture, /SetAngle|SetRotation|->Update\(|->(?:local|world)\s*=|setInterval/);
});
test('node traversal and payload are bounded, absent and truncated evidence explicit', () => {
  assert.match(source, /std::array<std::string, 16> ancestors/);
  assert.match(source, /std::array<InspectionNode, 10> nodes/);
  const capture = body('CaptureNode');
  assert.match(capture, /if \(!node\) return result/);
  assert.match(capture, /ancestorCount < result.ancestors.size\(\)/);
  assert.match(capture, /ancestryComplete = !ancestor/);
  assert.match(capture, /substr\(0, 128\)/);
  assert.doesNotMatch(capture, /->Update\(|->(?:local|world)\s*=/);
});
test('getter serializes copied data, guards wrong movie/expiry and omits invalid transforms', () => {
  const getter = body('ReadInspection');
  const copy = body('CopyInspection');
  assert.match(copy, /lock\(inspectionMutex\)/);
  assert.match(copy, /snapshot = inspection/);
  assert.match(copy, /snapshot.movieIdentity != movie/);
  assert.match(copy, /snapshot.session != generation.load\(\)/);
  assert.match(getter, /CopyInspection\(movie\)/);
  assert.match(getter, /snapshot.state\) != "captured"\) return/);
  assert.match(getter, /if \(node.localValid\) WriteTransform/);
  assert.match(getter, /if \(node.worldValid\) WriteTransform/);
  assert.doesNotMatch(getter, /GetSingleton|Get3D|GetVRNodeData|GetAngle\(|GetObjectByName/);
  assert.match(body('Restore'), /inspection.state = "expired"/);
});
test('pointer identities and counters are strings, and both aliases are registered', () => {
  assert.match(body('NodeIdentity'), /std::hex << reinterpret_cast<std::uintptr_t>\(node\)/);
  const getter = body('ReadInspection');
  assert.match(getter, /std::to_string\(snapshot.serial\)/);
  assert.match(getter, /std::to_string\(snapshot.session\)/);
  const register = body('Register');
  assert.match(register, /_root.CaptureVRCharacterInspection/);
  assert.match(register, /_root.GetVRCharacterInspection/);
});
test('Papyrus readout explicitly publishes a bounded movie-local string without retaining live nodes', () => {
  const publish = body('PublishInspection');
  assert.match(publish, /InspectionJSON\(CopyInspection\(movie\)\)/);
  assert.match(publish, /text.size\(\) > 64 \* 1024/);
  assert.match(publish, /SetVariable\("_root.VRCharacterInspectionJSON"/);
  assert.match(publish, /catch \(\.\.\.\)/);
  assert.match(publish, /readout-failed/);
  assert.doesNotMatch(publish, /GetSingleton|Get3D|GetVRNodeData|GetAngle\(|GetObjectByName|AddTask|ofstream/);
  const register = body('Register');
  assert.match(register, /_root.PublishVRCharacterInspection/);
  assert.match(register, /not-published/);
  assert.match(source, /operation == 7[\s\S]*?args.argCount == 0 && PublishInspection\(args.movie\)/);
});
test('JSON readout retains object-schema validity gates, string identities and bounded arrays', () => {
  const json = body('InspectionJSON');
  assert.match(json, /snapshot.state\) == "captured"/);
  assert.match(json, /std::to_string\(snapshot.serial\)/);
  assert.match(json, /std::to_string\(snapshot.session\)/);
  assert.match(json, /i < snapshot.nodes.size\(\)/);
  assert.match(json, /j < node.ancestorCount/);
  assert.match(json, /if \(snapshot.actorAngleValid\)/);
  assert.match(json, /if \(snapshot.actorPositionValid\)/);
  assert.match(json, /if \(node.localValid\)/);
  assert.match(json, /if \(node.worldValid\)/);
  assert.match(json, /error_handler_t::replace/);
  assert.doesNotMatch(json, /GetSingleton|Get3D|GetVRNodeData|GetAngle\(|GetObjectByName/);
});
