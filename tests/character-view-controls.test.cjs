// SPDX-License-Identifier: GPL-3.0-or-later
// Source-contract checks and mathematical reference checks, not a native/live assay.
const test = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const read = name => fs.readFileSync(path.join(__dirname, '../skee64', name), 'utf8');
const face = read('RaceSexMenuFaceView.cpp');
const source = face.split('namespace SKEE::CharacterInspection')[1];
const extensions = read('MenuExtensions.cpp');
function body(name) {
  const declaration = new RegExp('(?:bool|void|RE::UI_MESSAGE_RESULTS|RE::NiAVObject\\*)\\s+' + name + '\\(').exec(source);
  assert.ok(declaration, name); const start = declaration.index;
  const open = source.indexOf('{', start); let depth = 1, end = open + 1;
  while (depth && end < source.length) { if (source[end] === '{') depth++; if (source[end] === '}') depth--; end++; }
  assert.equal(depth, 0); return source.slice(open + 1, end - 1);
}
test('independent View sliders use existing extension interface and explicit zero defaults', () => {
  assert.match(source, /"view", "View", 1u<<30/);
  assert.match(source, /"avatarYaw","Avatar rotation",-180,180,1,0/);
  assert.match(source, /"viewYaw","View direction",-60,60,1,FaceView::ViewYaw\(\)/);
  assert.match(body('Register'), /movie == movieIdentity/);
  assert.match(body('Register'), /Restore\(\); movieIdentity = movie/);
  assert.doesNotMatch(source, /CreateFunction|SetVariable|SetAngle|SetRotation|RenderHook/);
});
test('preview surrounds exactly one native update and never changes saved facing', () => {
  const hook = body('ProcessHook');
  assert.equal((hook.match(/original\(menu, message\)/g) || []).length, 1);
  assert.ok(hook.indexOf('RemovePreview(false)') < hook.indexOf('original(menu, message)'));
  assert.ok(hook.indexOf('original(menu, message)') < hook.indexOf('ApplyPreview()'));
  assert.match(hook, /kUpdate/); assert.match(hook, /kHide/); assert.match(hook, /kForceHide/);
  assert.match(hook, /liveMenu.get\(\) == menu/);
  assert.match(hook, /menu->uiMovie.get\(\) == movieIdentity/);
  assert.match(body('ApplyPreview'), /p.Transpose\(\)\*yaw\*p\*baseline/);
  assert.match(body('ApplyPreview'), /root->local.rotate = previewRotation/);
  assert.doesNotMatch(source.replace(/\/\/[^\r\n]*/g, ''), /->local\s*=|->local.translate\s*=|->local.scale\s*=|->Update\(|UpdateWorldData|SetAngle\(/);
});
test('restoration retains translation/scale and refuses foreign or replacement poses', () => {
  const remove = body('RemovePreview');
  assert.match(remove, /avatar.get\(\) != LiveAvatar\(\)/);
  assert.match(remove, /avatar->parent != parent.get\(\)/);
  assert.match(remove, /SameRotation\(avatar->local.rotate, previewRotation\)/);
  assert.match(remove, /avatar->local.rotate = nativeRotation/);
  assert.match(remove, /Collect\(avatar.get\(\), nodes, ancestors\)/);
  assert.match(body('Restore'), /UnregisterProvider\(provider\)/);
});
test('graph collection validates bounded topology and pure native dispatch before writing', () => {
  const collect = body('Collect');
  assert.match(source, /maxNodes = 4096, maxDepth = 64/);
  for (const rva of ['C9BCE0','C9DEA0','C9DC10','CB78C0','C9C700']) assert.ok(collect.includes(rva), rva);
  assert.match(collect, /children.free_idx\(\) > children.capacity\(\)/);
  assert.match(collect, /child->parent != node/);
  assert.match(collect, /!seen.insert\(child\).second/);
  assert.match(collect, /kFixedBound/);
  assert.match(collect, /std::array<RE::NiAVObject\*, 4> protectedNodes/);
  for (const node of ['RoomNode','HmdNode','uiNode','InWorldUIQuadGeo']) assert.ok(collect.includes('vr->'+node+'.get()'), node);
  assert.match(collect, /std::find\(protectedNodes.begin\(\), protectedNodes.end\(\), object\)/);
  const apply = body('ApplyPreview');
  assert.ok(apply.indexOf('Collect(root, nodes, ancestors)') < apply.indexOf('root->local.rotate ='));
  const propagate = body('Propagate');
  assert.match(propagate, /nodes.rbegin\(\)/);
  assert.match(propagate, /for \(const auto& node : ancestors\) refit/);
});
test('hook qualification is exact VR-only, no foreign detour replacement', () => {
  const install = body('Install');
  assert.match(install, /defined\(ENABLE_SKYRIM_VR\)/);
  assert.match(install, /!REL::Module::IsVR\(\)/);
  assert.match(install, /REL::Version\(1,4,15,0\)/);
  assert.match(install, /\[4\] != REL::Module::get\(\).base\(\)\+0x8DD6E0/);
  assert.match(install, /write_vfunc\(4, ProcessHook\)/);
  assert.match(install, /Code\(0xCB78C0/);
});
test('queued extension inputs require exact live movie and fresh registration token', () => {
  assert.match(extensions, /\[key, token, value, identity\]/);
  assert.match(extensions, /menu->uiMovie.get\(\) != identity/);
  assert.match(extensions, /!ui->IsMenuOpen/);
  assert.match(extensions, /it->second.token != token/);
  assert.match(extensions, /item->second.value != value/);
  const input = extensions.slice(extensions.indexOf('tasks->AddTask([key, token, value, identity]'));
  assert.doesNotMatch(input, /\+\+service.revision/);
  assert.match(body('ViewSlider'), /ApplyViewYawOnGameTask/);
  assert.doesNotMatch(body('ViewSlider'), /RequestViewYaw/);
  assert.match(read('CharacterCreationInterface.cpp'), /CharacterInspection::Restore\(\);\s*SKEE::FaceView::Restore\(\)/);
});
test('view slider retains final queued input and structural avatar changes refresh face anchor', () => {
  assert.doesNotMatch(face, /yawQueued.exchange\(true\)/);
  assert.match(face, /yawPending.fetch_add\(1\)/);
  assert.match(face, /yawPending.fetch_sub\(1\)/);
  assert.match(face, /anchorRevision != avatarRevision.load\(\)/);
  assert.match(source, /priorYaw != appliedYaw/);
  assert.match(face, /if \(yawRoom\) yawAvatar = nodes.avatar/);
});
test('world-axis conjugation reference preserves pivot for arbitrary parent rotations', () => {
  const mul = (a,b) => a.map((row,i) => row.map((_,j) => row.reduce((sum,x,k) => sum+x*b[k][j],0)));
  const transpose = a => a[0].map((_,i) => a.map(row => row[i]));
  const yaw = angle => [[Math.cos(angle),-Math.sin(angle),0],[Math.sin(angle),Math.cos(angle),0],[0,0,1]];
  const parent = [[1,0,0],[0,0,-1],[0,1,0]], native = yaw(.7);
  for (const degrees of [-180,-90,0,37,180]) {
    const z = yaw(degrees*Math.PI/180);
    const local = mul(mul(mul(transpose(parent),z),parent),native);
    const actual = mul(parent,local), expected = mul(z,mul(parent,native));
    for (let i=0;i<3;i++) for (let j=0;j<3;j++) assert.ok(Math.abs(actual[i][j]-expected[i][j]) < 1e-12);
    const unit = mul(transpose(local),local);
    for (let i=0;i<3;i++) for (let j=0;j<3;j++) assert.ok(Math.abs(unit[i][j]-(i===j?1:0)) < 1e-12);
  }
  assert.match(body('Frame'), /determinant - 1.F/);
});
