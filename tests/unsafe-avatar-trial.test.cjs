// SPDX-License-Identifier: GPL-3.0-or-later
// Source contracts and independent rigid-transform references, NOT native/live tests.
const test = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const source = fs.readFileSync(require('node:path').join(__dirname, '../skee64/RaceSexMenuFaceView.cpp'), 'utf8')
  .split('namespace SKEE::CharacterInspection')[1];
function body(name) {
  const declaration = new RegExp('(?:bool|void|RE::UI_MESSAGE_RESULTS)\\s+'+name+'\\(').exec(source);
  assert.ok(declaration, name);
  const start = source.indexOf('{', declaration.index); let end=start+1, depth=1;
  while (depth && end<source.length) { if(source[end]==='{')depth++; if(source[end]==='}')depth--; end++; }
  assert.equal(depth,0); return source.slice(start+1,end-1);
}
test('unsafe path needs explicit per-menu opt-in and leaves strict production collection intact', () => {
  assert.match(source, /bool unsafeTrialEnabled\{\}/);
  assert.match(body('Register'), /"unsafeAvatarTrial","Unsafe avatar trial",0,1,1,0/);
  const apply=body('ApplyPreview');
  assert.match(apply, /if \(unsafeTrialEnabled\) return ApplyTrialPreview\(root\)/);
  assert.match(apply, /if \(!Collect\(root, nodes, ancestors\)\) return false/);
  assert.match(body('Collect'), /if \(!CollectAncestors\(root, ancestors\)\) return false/);
  assert.match(body('Restore'), /unsafeTrialEnabled = false/);
  assert.match(body('UnsafeTrialSlider'), /value != 0 && value != 1/);
  assert.ok(body('UnsafeTrialSlider').indexOf('RemovePreview()') < body('UnsafeTrialSlider').indexOf('unsafeTrialEnabled = value == 1'));
  assert.match(source, /"rotationQualified", false/);
});
test('trial validates a whole bounded avatar-only subtree before writing and calls no transform or refit target', () => {
  const apply=body('ApplyTrialPreview');
  for(const guard of ['trial-protected-tracking-or-ui-node','trial-object-vtable','trial-object-frame-or-sphere',
    'trial-child-array-contract','trial-child-parent-mismatch','trial-child-cycle-or-limit','trial-ancestor-cycle-or-depth'])
    assert.ok(apply.includes(guard),guard);
  assert.match(apply, /depth\+1 > maxDepth \|\| nodes.size\(\) >= maxNodes/);
  assert.match(apply, /seen.count\(p\)/);
  assert.ok(apply.indexOf('planned.push_back') < apply.indexOf('root->local.rotate ='));
  assert.ok(apply.indexOf('trialNodes = std::move(planned)') < apply.indexOf('root->local.rotate ='));
  assert.doesNotMatch(apply.replace(/\/\/[^\n]*/g,''), /Propagate\(|SceneFunction|SetAngle\(|->Update\(|UpdateWorldData|0x3CC170|0xD9F380/);
  assert.match(apply, /copy.nativeBound.radius > 0/);
  assert.match(apply, /copy.trialWorld.rotate = yaw\*copy.nativeWorld.rotate/);
  assert.match(apply, /pivot\+yaw\*\(copy.nativeWorld.translate-pivot\)/);
  assert.doesNotMatch(apply, /->local.translate\s*=|->local.scale\s*=|->parent->(?:world|local|worldBound)\s*=/);
});
test('trial restores copied owned values before the native update and never clobbers replacement/foreign state', () => {
  assert.match(body('RemovePreview'), /if \(!trialNodes.empty\(\)\) return RemoveTrialPreview\(\)/);
  const remove=body('RemoveTrialPreview');
  assert.match(remove, /avatar.get\(\) == LiveAvatar\(\).*avatar->parent == parent.get\(\)/);
  assert.match(remove, /object->parent != node.parentIdentity/);
  assert.match(remove, /SameWorld\(object->world, node.trialWorld\)\) object->world = node.nativeWorld/);
  assert.match(remove, /SamePoint\(object->worldBound.center, node.trialBound.center\)/);
  assert.match(remove, /object->worldBound.radius == node.trialBound.radius/);
  assert.match(remove, /SameRotation\(avatar->local.rotate, previewRotation\)/);
  assert.match(remove, /unsafeTrialRestoreConflicts/);
  assert.match(remove, /trialNodes.clear\(\)/);
  const hook=body('ProcessHook');
  assert.ok(hook.indexOf('RemovePreview(false)')<hook.indexOf('original(menu, message)'));
  assert.equal((hook.match(/original\(menu, message\)/g)||[]).length,1);
  assert.match(hook, /if \(close\) Restore\(\)/);
});
test('rigid-preview reference turns positions and positive sphere centers about the root without scale/pivot drift', () => {
  const turn=(v,p,a)=>{const [x,y,z]=v.map((n,i)=>n-p[i]);return[p[0]+Math.cos(a)*x-Math.sin(a)*y,p[1]+Math.sin(a)*x+Math.cos(a)*y,p[2]+z];};
  const pivot=[2337,2381,114], sample=[2300,2400,234];
  const distance=(a,b)=>Math.hypot(...a.map((v,i)=>v-b[i]));
  for(const deg of [-180,-90,1,13,90,180]) {
    const a=deg*Math.PI/180, moved=turn(sample,pivot,a), restored=turn(moved,pivot,-a);
    assert.ok(Math.abs(distance(moved,pivot)-distance(sample,pivot))<1e-9);
    assert.ok(distance(restored,sample)<1e-9);
    assert.deepEqual(turn(pivot,pivot,a),pivot);
  }
  // Foreign writes fail the copied-preview ownership check; never assert safety.
  const restore=(now,preview,baseline)=>now===preview?baseline:now;
  assert.equal(restore(13,13,0),0); assert.equal(restore(14,13,0),14);
});
