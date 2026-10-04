// SPDX-License-Identifier: GPL-3.0-or-later
// Actual original AS2 adapter, with a category-list surrogate. Not a live GFx test.
const test = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const source = fs.readFileSync(path.join(__dirname, '../tools/vr-racesex-patches/Appearance.as.inc'), 'utf8');
const prototype = Function('return ({' + source.replace(/^   function (\w+)\(/gm, '$1(').replace(/^   }[ \t]*$/gm, '},') + '})')();
global.RaceMenuDefines = { CATEGORY_RACE: 2 };

function fixture(flag = 2) {
  global._global = {skse: {plugins: {CharGen: {initialCategoryFlag: flag}}}};
  const menu = Object.assign({}, prototype, {bMenuInitialized: true, vrInitialCategoriesReady: true});
  const entries = [{flag: 2044, text: '$All', enabled: true, filterFlag: 1},
    {flag: 4, text: '$Body', enabled: true, filterFlag: 1},
    {flag: 2, text: 'Translated Race', enabled: true, filterFlag: 1}];
  const list = menu.categoryList = {
    entryList: entries, selectedIndex: 0, selectedEntry: entries[0], presses: [], invalidations: 0,
    InvalidateData() {
      this.invalidations++;
      if (this.suspended) return;
      this.entryList.forEach((entry, index) => {entry.itemIndex = index; entry.clipIndex = this.entryList.length - index - 1;});
      this.afterInvalidate?.();
    },
    getListEnumIndex(index) { return this.filtered === index ? undefined : this.entryList[index].clipIndex; },
    onItemPress(index, input) {
      this.presses.push([index, input]);
      this.selectedIndex = index;
      this.selectedEntry = this.entryList[index];
      // Same category callback as the menu's itemPress listener, not ChangeRace.
      menu.CancelVRInitialCategory();
      menu.filter = this.selectedEntry.flag;
    }
  };
  menu.ApplyVRCategoryPresentation = () => {};
  return menu;
}

test('All/default/missing bridge preserves stock selection and filter without invalidating', () => {
  for (const flag of [0, undefined, 2044, 4, NaN]) {
    const menu = fixture(flag);
    if (flag === undefined) delete _global.skse.plugins.CharGen.initialCategoryFlag;
    menu.ApplyVRInitialCategory();
    assert.equal(menu.categoryList.invalidations, 0);
    assert.equal(menu.categoryList.selectedIndex, 0);
    assert.equal(menu.filter, undefined);
    assert.equal(menu.vrInitialCategoryDone, true);
  }
});
test('Race uses stable identity and the normal press path, not translated labels/rendered indices', () => {
  const menu = fixture();
  menu.ApplyVRInitialCategory();
  assert.deepEqual(menu.categoryList.presses, [[2, 0]]);
  assert.equal(menu.categoryList.selectedEntry.flag, 2);
  assert.equal(menu.filter, 2);
  assert.equal(menu.categoryList.selectedEntry.clipIndex, 0);
});
test('both readiness orders apply only after categories and sliders are ready', () => {
  for (const first of ['bMenuInitialized', 'vrInitialCategoriesReady']) {
    const menu = fixture();
    menu.bMenuInitialized = menu.vrInitialCategoriesReady = false;
    menu.ApplyVRInitialCategory();
    menu[first] = true;
    menu.ApplyVRInitialCategory();
    assert.equal(menu.categoryList.presses.length, 0);
    assert.equal(menu.vrInitialCategoryDone, undefined);
    menu.bMenuInitialized = menu.vrInitialCategoriesReady = true;
    menu.ApplyVRInitialCategory();
    assert.equal(menu.categoryList.presses.length, 1);
  }
});
test('once per movie; race/sex rebuilds, extension refresh and mode returns cannot reset navigation', () => {
  const menu = fixture();
  menu.ApplyVRInitialCategory();
  menu.categoryList.onItemPress(1, 0);
  for (let i = 0; i < 4; i++) menu.ApplyVRInitialCategory();
  assert.equal(menu.categoryList.presses.length, 2);
  assert.equal(menu.filter, 4);
  const reopened = fixture();
  reopened.ApplyVRInitialCategory();
  assert.equal(reopened.categoryList.presses.length, 1);
});
test('hidden/missing/restricted/disabled/filtered/suspended Race retains All', () => {
  for (const setup of [
    m => {m.ApplyVRCategoryPresentation = () => m.categoryList.entryList.pop();},
    m => {m.categoryList.entryList.pop();},
    m => {m.categoryList.entryList[2].filterFlag = 0;},
    m => {m.categoryList.entryList[2].enabled = false;},
    m => {m.categoryList.filtered = 2;},
    m => {m.categoryList.suspended = true;}
  ]) {
    const menu = fixture(); setup(menu); menu.ApplyVRInitialCategory();
    assert.equal(menu.categoryList.presses.length, 0);
    assert.equal(menu.categoryList.selectedIndex, 0);
    assert.equal(menu.vrInitialCategoryDone, true);
  }
});
test('early category/mode navigation cancels pending preference, including re-entrant invalidation', () => {
  const menu = fixture();
  menu.bMenuInitialized = false;
  menu.CancelVRInitialCategory();
  menu.bMenuInitialized = true;
  menu.ApplyVRInitialCategory();
  assert.equal(menu.categoryList.presses.length, 0);
  const reentrant = fixture();
  reentrant.categoryList.afterInvalidate = () => {reentrant.ApplyVRInitialCategory(); reentrant.CancelVRInitialCategory();};
  reentrant.ApplyVRInitialCategory();
  assert.equal(reentrant.categoryList.invalidations, 1);
  assert.equal(reentrant.categoryList.presses.length, 0);
});
test('blocked input and active keyboard are never forced or retried', () => {
  for (const setup of [m => {m.categoryList.disableInput = true;}, m => {m.categoryList.disableSelection = true;},
    m => {m.bTextEntryMode = true;}, m => {m.vrTextInputActive = true;}]) {
    const menu = fixture(); setup(menu); menu.ApplyVRInitialCategory();
    menu.categoryList.disableInput = menu.categoryList.disableSelection = menu.bTextEntryMode = menu.vrTextInputActive = false;
    menu.ApplyVRInitialCategory();
    assert.equal(menu.categoryList.presses.length, 0);
    assert.equal(menu.categoryList.invalidations, 0);
  }
});
test('recipe wires readiness and navigation at unique anchors and only in VR', () => {
  const recipe = fs.readFileSync(path.join(__dirname, '../tools/patch-vr-racesex-swf.ps1'), 'utf8');
  for (const anchor of ['Initial category slider-ready anchor missing or ambiguous.',
    'Initial category categories-ready anchor missing or ambiguous.', 'Initial category navigation anchor missing or ambiguous.'])
    assert.ok(recipe.includes(anchor));
  assert.ok(recipe.includes('if(_global.skse.IsVR()) this.ApplyVRInitialCategory();'));
  assert.ok(recipe.includes('if(_global.skse.IsVR() && event.index != 0) this.CancelVRInitialCategory();'));
  assert.ok(recipe.includes('this.vrInitialCategoriesReady = true;'));
  const native = fs.readFileSync(path.join(__dirname, '../skee64/MenuConfiguration.cpp'), 'utf8');
  assert.match(native, /readOption\("Menu Appearance", "sInitialCategory"\)/);
  assert.match(native, /RegisterNumber\(root, "initialCategoryFlag", static_cast<unsigned>\(initialCategory\)\)/);
});
