// SPDX-License-Identifier: GPL-3.0-or-later
// Source-contract and independent mathematical reference checks; not native execution.
const test = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const source = fs.readFileSync(path.join(__dirname, '../skee64/RaceSexMenuFaceView.cpp'), 'utf8');
const policy = fs.readFileSync(path.join(__dirname, '../skee64/AvatarBoundProbe.h'), 'utf8');
function body(name) {
  const declaration = new RegExp('(?:bool|nlohmann::json)\\s+'+name+'\\(').exec(source);
  assert.ok(declaration, name);
  const start = source.indexOf('{', declaration.index); let depth = 1, end = start+1;
  while (depth && end < source.length) { if (source[end] === '{') depth++; if (source[end] === '}') depth--; end++; }
  assert.equal(depth, 0); return source.slice(start+1,end-1);
}
test('explicit graph probe bypasses only fixed-bound rejection, without changing production defaults', () => {
  assert.match(source, /GraphInspection\* inspection = nullptr/);
  const collect = body('Collect');
  assert.match(collect, /kFixedBound\) && !inspection\)\s*return Fail\("ancestor-fixed-bound"/);
  assert.match(collect, /kFixedBound\) && !inspection\)\s*return Fail\("object-fixed-bound"/);
  assert.match(collect, /inspection->validatedNodes = i\+1/);
  for (const reason of ['ancestor-bounds-target','object-compose-target','child-array-contract','child-parent-mismatch','child-cycle-or-limit'])
    assert.match(collect, new RegExp('Fail\\("'+reason+'"[^;]*diagnostic\\)'));
  assert.match(body('Fail'), /auto& failure = diagnostic \? \*diagnostic : refusal/);
  const probe = body('InspectGraphBounds');
  assert.match(probe, /Collect\(root, nodes, ancestors, &inspection\)/);
  assert.doesNotMatch(probe.replace(/\/\/[^\n]*/g,''), /Propagate\(|ApplyPreview\(|SceneFunction|refusal\s*=|rejected\s*=|->(?:world|local|worldBound)\s*=/);
  assert.match(source, /\{"avatarBoundProbe", InspectGraphBounds\(\)\}/);
  assert.match(source, /if \(!Collect\(root, nodes, ancestors\)\) return false/);
});
test('partial coverage is explicit; containment never asserts rotation qualification', () => {
  const probe = body('InspectGraphBounds');
  assert.match(probe, /"rotationQualified", false/);
  assert.match(probe, /"fixedBoundGuardRelaxedForApplication", false/);
  assert.match(probe, /i < inspection.validatedNodes/);
  assert.match(probe, /"sphereCoverage"\] = "validated-nodes-only"/);
  assert.match(probe, /complete && invalid == 0 && positive > 0/);
  assert.match(probe, /containsSampledFullTurnEnvelope"\] = nullptr/);
  assert.match(probe, /fixedObjects.size\(\) < 8/);
  assert.match(probe, /skin-owned sphere\/AABB.*remain unqualified/);
  assert.match(probe, /composerSkipFlagBit9Count/);
});
test('both copied readouts expose only finite bounds, independent of live scene reads', () => {
  const capture = source.slice(source.indexOf('InspectionNode CaptureNode'),source.indexOf('bool RequestInspection'));
  assert.match(capture, /result.worldBound = node->worldBound/);
  assert.match(capture, /result.boundValid = BoundProbe::Valid/);
  for (const name of ['ReadInspection','InspectionJSON']) {
    const start = source.indexOf(name+'('), end = source.indexOf('\n        }', start);
    const reader = source.slice(start,end);
    assert.match(reader, /node.boundValid/);
    assert.match(reader, /fixedBound/);
    assert.match(reader, /worldBound/);
    assert.doesNotMatch(reader, /Get3D|GetSingleton|GetFlags/);
  }
});
test('full-turn sphere reference contains rotated samples about arbitrary pivots', () => {
  const pivot = [13,-71,22];
  const samples = [{center:[19,-12,125],radius:9},{center:[-90,-130,4],radius:17}];
  const distance = (a,b) => Math.hypot(...a.map((x,i)=>x-b[i]));
  const radius = Math.max(...samples.map(s=>distance(pivot,s.center)+s.radius));
  for (const s of samples) for (const angle of [-Math.PI,-2.4,-1,0,1.3,Math.PI]) {
    const [x,y,z] = s.center.map((v,i)=>v-pivot[i]);
    const rotated = [pivot[0]+x*Math.cos(angle)-y*Math.sin(angle),pivot[1]+x*Math.sin(angle)+y*Math.cos(angle),pivot[2]+z];
    assert.ok(distance(pivot,rotated)+s.radius <= radius+1e-10);
  }
  assert.match(policy, /Distance\(envelope.center, sample.center\) \+ sample.radius/);
  assert.match(policy, /sample.radius == 0\) return true/);
});
test('reference containment excludes empty, nonfinite, negative and marginal bounds', () => {
  const valid = s => s.center.every(Number.isFinite) && Number.isFinite(s.radius) && s.radius >= 0;
  const contains = (c,e,m) => valid(c)&&valid(e)&&c.radius>0&&e.radius>0&&Number.isFinite(m)&&m>=0&&
    Math.hypot(...c.center.map((x,i)=>x-e.center[i]))+e.radius+m<=c.radius;
  const envelope={center:[0,0,0],radius:100};
  assert.equal(contains({center:[10,0,0],radius:110.01},envelope,.01),true);
  for (const s of [{center:[10,0,0],radius:110},{center:[0,0,0],radius:0},
    {center:[0,0,0],radius:-1},{center:[NaN,0,0],radius:200},{center:[0,0,0],radius:Infinity}])
    assert.equal(contains(s,envelope,.01),false);
  assert.equal(contains({center:[0,0,0],radius:200}, {...envelope,radius:0},.01),false);
  assert.equal(contains({center:[0,0,0],radius:200}, envelope,-1),false);
  assert.match(policy, /std::isfinite\(s.radius\) && s.radius >= 0/);
  assert.match(policy, /container.radius > 0/);
});
