// SPDX-License-Identifier: GPL-3.0-or-later
// Source contracts and independent rigid-transform references, NOT native/live tests.
const test = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const source = fs.readFileSync(require('node:path').join(__dirname, '../skee64/RaceSexMenuFaceView.cpp'), 'utf8')
  .split('namespace SKEE::CharacterInspection')[1];
function body(name) {
  const declaration = new RegExp('(?:bool|void|RE::UI_MESSAGE_RESULTS)\\s+'+name+'\\([^;{}]*\\)\\s*\\{').exec(source);
  assert.ok(declaration, name);
  const start = source.indexOf('{', declaration.index); let end=start+1, depth=1;
  while (depth && end<source.length) { if(source[end]==='{')depth++; if(source[end]==='}')depth--; end++; }
  assert.equal(depth,0); return source.slice(start+1,end-1);
}
test('avatar slider directly selects the tested preview mode without a separate unsafe control', () => {
  assert.match(source, /bool unsafeTrialEnabled\{\}/);
  assert.doesNotMatch(source, /UnsafeTrialSlider|"Unsafe avatar trial"/);
  const register=body('Register');
  assert.match(register, /"avatarYaw","Avatar rotation",-180,180,1,0/);
  assert.match(register, /else\s*\{[\s\S]*unsafeTrialEnabled = true/);
  assert.doesNotMatch(register, /ApplyPreview\(|ApplyTrialPreview\(|->(?:local|world|worldBound)\s*=/);
  assert.ok(register.indexOf('RegisterSlider(') < register.indexOf('unsafeTrialEnabled = true'));
  const apply=body('ApplyPreview');
  assert.match(apply, /if \(unsafeTrialEnabled\) return ApplyTrialPreview\(root\)/);
  assert.match(apply, /if \(!Collect\(root, nodes, ancestors\)\) return false/);
  assert.match(body('Collect'), /if \(!CollectAncestors\(root, ancestors\)\) return false/);
  assert.match(body('Restore'), /unsafeTrialEnabled = false/);
  assert.match(body('ProcessHook'), /!rejected && requestedYaw != 0 && !ApplyPreview\(\)/);
  assert.match(source, /"activation", "avatar-slider-no-separate-opt-in"/);
  assert.match(source, /"rotationQualified", false/);
});
test('trial validates a whole bounded avatar-only subtree before writing and calls no transform or refit target', () => {
  const apply=body('ApplyTrialPreview'), collect=body('CollectTrialBranch');
  for(const guard of ['trial-protected-tracking-or-ui-node','trial-object-vtable','trial-object-frame-or-sphere',
    'trial-child-array-contract','trial-child-parent-mismatch','trial-child-cycle-or-limit','trial-ancestor-cycle-or-depth'])
    assert.ok(collect.includes(guard),guard);
  assert.match(collect, /depth\+1 > maxDepth \|\| nodes.size\(\) >= maxNodes/);
  assert.match(collect, /seen.count\(p\)/);
  assert.match(apply, /if \(!CollectTrialBranch\(root, nodes\)\) return false/);
  assert.doesNotMatch(collect, /->(?:local|world|worldBound|parent)\s*=|Propagate\(|SceneFunction|->Update\(/);
  assert.ok(apply.indexOf('planned.push_back') < apply.indexOf('root->local.rotate ='));
  assert.ok(apply.indexOf('trialNodes = std::move(planned)') < apply.indexOf('root->local.rotate ='));
  assert.doesNotMatch(apply.replace(/\/\/[^\n]*/g,''), /Propagate\(|SceneFunction|SetAngle\(|->Update\(|UpdateWorldData|0x3CC170|0xD9F380/);
  assert.match(apply, /copy.nativeBound.radius > 0/);
  assert.match(apply, /copy.trialWorld.rotate = yaw\*copy.nativeWorld.rotate/);
  assert.match(apply, /pivot\+yaw\*\(copy.nativeWorld.translate-pivot\)/);
  assert.doesNotMatch(apply, /->local.translate\s*=|->local.scale\s*=|->parent->(?:world|local|worldBound)\s*=/);
});

test('replacement resets angle and stale tokens only after bounded qualification; same-root latch survives', () => {
  const observe=body('ObserveAvatarLifecycle');
  assert.match(source, /RE::NiPointer<RE::NiAVObject> lifecycleRoot/);
  assert.match(observe, /if \(lifecycleRoot.get\(\) == root\) return false/);
  assert.ok(observe.indexOf('lifecycleRoot.get() == root') < observe.indexOf('RemovePreview(false)'));
  assert.ok(observe.indexOf('RemovePreview(false)') < observe.indexOf('lifecycleRoot.reset(root); ++avatarReplacements'));
  assert.match(observe, /requestedYaw = appliedYaw = 0/);
  assert.match(observe, /unsafeTrialEnabled \? CollectTrialBranch\(root, nodes\)/);
  assert.match(observe, /Collect\(root, nodes, ancestors\)/);
  assert.ok(observe.indexOf('CollectTrialBranch(root, nodes)') < observe.indexOf('rejected = !valid'));
  assert.match(observe, /replacementRefusal = refusal/);
  assert.equal((observe.match(/RegisterSlider\(/g)||[]).length,1);
  assert.match(observe, /if \(!registered\)/);
  assert.doesNotMatch(observe, /ApplyPreview\(|->(?:local|world|worldBound|parent)\s*=|Propagate\(|->Update\(/);
  assert.match(body('AvatarSlider'), /if \(ObserveAvatarLifecycle\(\)\) return/);
  const hook=body('ProcessHook');
  assert.ok(hook.indexOf('ObserveAvatarLifecycle()') < hook.indexOf('original(menu, message)'));
  assert.ok(hook.lastIndexOf('ObserveAvatarLifecycle()') > hook.indexOf('original(menu, message)'));
  assert.match(body('Restore'), /lifecycleRoot.reset\(\); avatarReplacements = 0/);
});

test('root lifecycle independent reference covers rejection, rebuild, missing 3D and token cancellation', () => {
  // State-machine reference, not native execution or proof of GFx task order.
  const state={root:'Nord1',rejected:true,yaw:0,token:1,optIn:true,replacements:0};
  const observe=(root,valid)=>{
    if(!root){state.yaw=0;state.rejected=true;return true;}
    if(root===state.root)return false;
    state.root=root;state.yaw=0;state.rejected=!valid;state.replacements++;state.token++;
    return true;
  };
  assert.equal(observe('Nord1',true),false);assert.equal(state.rejected,true);
  const queuedToken=state.token;
  assert.equal(observe('Argonian1',true),true);assert.equal(state.rejected,false);
  assert.notEqual(queuedToken,state.token);assert.equal(state.yaw,0);assert.equal(state.optIn,true);
  state.yaw=45;
  observe(null,false);assert.equal(state.yaw,0);assert.equal(state.rejected,true);
  observe('Argonian1',true);assert.equal(state.rejected,true);
  observe('Nord2',true);assert.equal(state.rejected,false);
  observe('Broken3',false);assert.equal(state.rejected,true);
  observe('Broken3',true);assert.equal(state.rejected,true); // no same-root automatic retry
  observe('Nord4',true);assert.equal(state.rejected,false);
  assert.equal(state.replacements,4);
});
test('trial stages inverse-current-pose removal before writes and keeps conservative refusal recovery', () => {
  assert.match(body('RemovePreview'), /if \(!trialNodes.empty\(\)\) return RemoveTrialPreview\(\)/);
  const remove=body('RemoveTrialPreview');
  assert.match(remove, /auto\* live = LiveAvatar\(\)/);
  assert.match(remove, /avatar.get\(\) == live && avatar && avatar->parent == parent.get\(\)/);
  assert.match(remove, /object->parent != node.parentIdentity/);
  assert.match(remove, /SameWorld\(object->world, node.trialWorld\)\) object->world = node.nativeWorld/);
  assert.match(remove, /SamePoint\(object->worldBound.center, node.trialBound.center\)/);
  assert.match(remove, /object->worldBound.radius == node.trialBound.radius/);
  assert.match(remove, /SameRotation\(avatar->local.rotate, previewRotation\)/);
  assert.match(remove, /unsafeTrialRestoreConflicts/);
  assert.match(remove, /trialNodes.clear\(\)/);
  assert.ok(remove.indexOf('owned = PlanTrialRemoval(planned)') < remove.indexOf('avatar->local.rotate = nativeRotation'));
  assert.match(remove, /for \(const auto& node : planned\)/);
  assert.match(remove, /basis-or-topology-conflict/);
  const hook=body('ProcessHook');
  assert.ok(hook.indexOf('RemovePreview(false)')<hook.indexOf('original(menu, message)'));
  assert.equal((hook.match(/original\(menu, message\)/g)||[]).length,1);
  assert.match(hook, /if \(close\) Restore\(\)/);
});

test('restore observations are bounded copied values and do not add scene writes or callbacks', () => {
  assert.match(source, /std::array<TrialConflict, 8> samples/);
  assert.match(source, /std::array<char, 129> name/);
  const observe=body('ObserveTrialNode');
  assert.ok(observe.indexOf('++report.conflictingNodes') < observe.indexOf('report.sampleCount == report.samples.size()'));
  assert.ok(observe.indexOf('report.fieldCounts[i]') < observe.indexOf('report.sampleCount == report.samples.size()'));
  assert.match(observe, /i\+1 < sample.name.size\(\) && name\[i\]/);
  assert.match(observe, /sample.currentWorld = object->world/);
  assert.match(observe, /sample.currentBound = object->worldBound/);
  assert.doesNotMatch(observe, /object->(?:world|worldBound|local|parent)\s*=|object->parent->|push_back|new\s|nlohmann|SceneFunction|Propagate\(|->Update\(/);
  const remove=body('RemoveTrialPreview');
  assert.ok(remove.indexOf('report.currentLocal = avatar->local.rotate') < remove.indexOf('avatar->local.rotate = nativeRotation'));
  assert.ok(remove.indexOf('ObserveTrialNode(node, index++)') < remove.indexOf('owned = PlanTrialRemoval(planned)'));
  assert.match(remove, /discarded-replaced-root/);
  assert.match(remove, /root-parent-conflict/);
  assert.match(body('Restore'), /trialRestoreReport = \{\}; trialAppliedAt = 0/);
  assert.match(source, /"lastRestore", TrialRestoreObservation\(\)/);
  assert.match(source, /"writerIdentified", false/);
});

test('independent conflict-count reference keeps totals beyond its eight retained samples', () => {
  // Reference for the bounded sampling contract, not execution of ObserveTrialNode.
  const counts=Array(6).fill(0), samples=[];
  for(let i=0;i<4096;i++) {
    const changed=[i===0,true,i%2===0,false,i%3===0,false];
    changed.forEach((flag,j)=>{if(flag)counts[j]++;});
    if(samples.length<8)samples.push({index:i,changed});
  }
  assert.equal(samples.length,8);
  assert.deepEqual(counts,[1,4096,2048,0,1366,0]);
  // Worst-case escaped names and float spellings remain bounded independently
  // of subtree size; reserve 32 KiB for the surrounding existing capture.
  const scalar=-3.4028234663852886e38;
  const world={position:Array(3).fill(scalar),rotationRowMajor:Array(9).fill(scalar),scale:scalar,validFrame:false};
  const bound={center:Array(3).fill(scalar),radius:scalar};
  const sample={identity:'0xffffffffffffffff',name:'\u0001'.repeat(128),index:4095,depth:128,
    expectedParent:'0xffffffffffffffff',currentParent:'0xffffffffffffffff',
    changed:{parent:true,worldRotation:true,worldPosition:true,worldScale:true,boundCenter:true,boundRadius:true},
    worldMatchesNative:false,boundMatchesNative:false,
    nativeWorld:world,previewWorld:world,currentWorld:world,nativeBound:bound,previewBound:bound,currentBound:bound};
  const size=Buffer.byteLength(JSON.stringify({samples:Array(8).fill(sample),fieldCounts:counts,
    nativeRootLocalRotation:world.rotationRowMajor,previewRootLocalRotation:world.rotationRowMajor,currentRootLocalRotation:world.rotationRowMajor}));
  assert.ok(size+32768<65536,`reference sample bytes ${size}`);
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
});

test('inverse removal preserves animated poses rather than replaying stale snapshots (independent reference)', () => {
  const turn=(v,p,a)=>{const [x,y,z]=v.map((n,i)=>n-p[i]);return[p[0]+Math.cos(a)*x-Math.sin(a)*y,p[1]+Math.sin(a)*x+Math.cos(a)*y,p[2]+z];};
  const pivot=[2337,2381,114], stale=[2300,2400,234], animated=[2302,2397,235];
  const near=(a,b)=>a.every((v,i)=>Math.abs(v-b[i])<1e-8);
  for (const deg of [-180,-90,-21,0,21,90,180]) {
    const yaw=deg*Math.PI/180;
    let pose=[...animated], orientation=0.123;
    for(let cycle=0;cycle<1000;cycle++) {
      pose=turn(turn(pose,pivot,yaw),pivot,-yaw);
      orientation=(orientation+yaw)-yaw;
    }
    assert.ok(near(pose,animated)); assert.ok(!near(pose,stale));
    assert.ok(Math.abs(orientation-0.123)<1e-12);
  }
  // If a writer instead supplies an unrecognisable NATIVE pose, this trial's
  // preview-basis assumption gives the wrong answer. Keep that limitation explicit.
  assert.ok(!near(turn(animated,pivot,-21*Math.PI/180),animated));
});

test('inverse plan requires stable basis and exact complete topology without relaxing strict mode', () => {
  const plan=body('PlanTrialRemoval');
  for(const guard of ['SameRotation(avatar->local.rotate, previewRotation)',
    'SameWorld(avatar->world, trialNodes.front().trialWorld)', 'SameWorld(parent->world, trialParentWorld)',
    'object->parent != node.parentIdentity', '!members.count(child)', '!childrenSeen.insert(child).second',
    'childrenSeen.size()+1 != members.size()', '!Frame(object->local)', '!Frame(object->world)',
    '!BoundProbe::Valid(WorldSphere(object))']) assert.ok(plan.includes(guard),guard);
  assert.match(plan, /inverse\*object->world.rotate/);
  assert.match(plan, /trialPivot\+inverse\*\(object->world.translate-trialPivot\)/);
  assert.match(plan, /copy.nativeWorld = object->world/);
  assert.match(plan, /copy.nativeBound = bound/);
  assert.match(plan, /bound.radius > 0/);
  assert.doesNotMatch(plan, /object->(?:world|worldBound|local|parent)\s*=|avatar->local.rotate\s*=|Propagate\(|SceneFunction|->Update\(/);
  assert.match(source, /"externalWriterBasisQualified", false/);
  // Independent edge-identity reference, not execution of the native traversal.
  const topology=(members,edges)=>edges.length===members.length-1 &&
    new Set(edges.map(e=>e[1])).size===edges.length &&
    edges.every(e=>members.includes(e[0]) && members.includes(e[1]) && e[1]!==members[0]);
  assert.ok(topology(['root','a','b'],[['root','a'],['a','b']]));
  assert.ok(!topology(['root','a','b'],[['root','a']]));
  assert.ok(!topology(['root','a'],[['root','a'],['root','new']]));
  assert.ok(!topology(['root','a','b'],[['root','a'],['root','a']]));
});
