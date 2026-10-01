   // DevBench-facing, menu-owned diagnostic API. Call through UI.Invoke* on
   // Race Sex Menu; read vrDiagnosticSnapshot / vrDiagnosticResult afterwards.
   // A generation is valid only for this movie's current live slider set.
   function InvalidateVRDiagnosticControls()
   {
      if(this.vrDiagnosticGeneration == undefined)
         this.vrDiagnosticGeneration = Math.floor(Math.random() * 8000000) + 1;
      else
         this.vrDiagnosticGeneration = this.vrDiagnosticGeneration % 16000000 + 1;
      this.vrDiagnosticSnapshot = undefined;
      this.vrDiagnosticIdentity = undefined;
      this.vrDiagnosticSnapshotJson = "";
   }
   function VRDiagnosticIsSexSlider(entry)
   {
      return entry.callbackName == "ChangeSex" || entry.sliderID == RaceMenuDefines.STATIC_SLIDER_SEX;
   }
   function VRDiagnosticJsonQuote(value)
   {
      var text = String(value);
      var result = '"';
      var slash = String.fromCharCode(92);
      for(var i = 0; i < text.length; i++)
      {
         var ch = text.charAt(i);
         if(ch == '"') result += slash + '"';
         else if(ch == slash) result += slash + slash;
         else if(ch == String.fromCharCode(10)) result += slash + 'n';
         else if(ch == String.fromCharCode(13)) result += slash + 'r';
         else if(ch == String.fromCharCode(9)) result += slash + 't';
         else if(text.charCodeAt(i) < 32)
         {
            var code = text.charCodeAt(i).toString(16);
            while(code.length < 4) code = '0' + code;
            result += slash + 'u' + code;
         }
         else result += ch;
      }
      return result + '"';
   }
   function VRDiagnosticJsonNumber(value)
   {
      return typeof value == "number" && !isNaN(value) && Math.abs(value) < 1e308 ? String(value) : "null";
   }
   function PublishVRDiagnosticSnapshotJson()
   {
      var data = this.vrDiagnosticSnapshot;
      var result = '{"schema":1,"status":' + this.VRDiagnosticJsonQuote(data.status) +
         ',"generation":' + this.VRDiagnosticJsonNumber(data.generation) +
         ',"mode":' + this.VRDiagnosticJsonNumber(data.mode) +
         ',"allCoversEverySlider":' + String(data.allCoversEverySlider) + ',"sliders":[';
      for(var i = 0; i < data.sliders.length; i++)
      {
         var s = data.sliders[i];
         if(i > 0) result += ',';
         result += '{"slot":' + this.VRDiagnosticJsonNumber(s.slot) +
            ',"id":' + this.VRDiagnosticJsonNumber(s.id) +
            ',"name":' + this.VRDiagnosticJsonQuote(s.name) +
            ',"callback":' + this.VRDiagnosticJsonQuote(s.callback) +
            ',"minimum":' + this.VRDiagnosticJsonNumber(s.minimum) +
            ',"maximum":' + this.VRDiagnosticJsonNumber(s.maximum) +
            ',"step":' + this.VRDiagnosticJsonNumber(s.step) +
            ',"value":' + this.VRDiagnosticJsonNumber(s.value) +
            ',"enabled":' + String(s.enabled) +
            ',"categoryMask":' + this.VRDiagnosticJsonNumber(s.categoryMask) +
            ',"inAll":' + String(s.inAll) +
            ',"action":' + this.VRDiagnosticJsonQuote(s.action) + '}';
      }
      result += '],"races":[';
      for(var i = 0; i < data.races.length; i++)
      {
         var race = data.races[i];
         if(i > 0) result += ',';
         result += '{"slot":' + this.VRDiagnosticJsonNumber(race.slot) +
            ',"id":' + this.VRDiagnosticJsonNumber(race.id) +
            ',"name":' + this.VRDiagnosticJsonQuote(race.name) +
            ',"active":' + String(race.active) +
            ',"enabled":' + String(race.enabled) + '}';
      }
      this.vrDiagnosticSnapshotJson = result + ']}';
   }
   function PublishVRDiagnosticResultJson()
   {
      var result = this.vrDiagnosticResult;
      this.vrDiagnosticResultJson = '{"ok":' + String(result.ok) +
         ',"code":' + this.VRDiagnosticJsonQuote(result.code) +
         ',"message":' + this.VRDiagnosticJsonQuote(result.message) +
         ',"generation":' + this.VRDiagnosticJsonNumber(result.generation) +
         ',"requestGeneration":' + this.VRDiagnosticJsonNumber(this.vrDiagnosticRequest.generation) +
         ',"slot":' + this.VRDiagnosticJsonNumber(this.vrDiagnosticRequest.slot) +
         ',"value":' + this.VRDiagnosticJsonNumber(this.vrDiagnosticRequest.value) +
         ',"raceId":' + this.VRDiagnosticJsonNumber(this.vrDiagnosticRequest.raceId) +
         ',"sliderId":' + this.VRDiagnosticJsonNumber(this.vrDiagnosticRequest.sliderId) +
         ',"callback":' + this.VRDiagnosticJsonQuote(this.vrDiagnosticRequest.callback) + '}';
   }
   function VRDiagnosticAvailability()
   {
      if(!_global.skse.IsVR()) return "not-vr";
      if(this.modeSelect == undefined || this.modeSelect.getMode == undefined ||
         this.itemList == undefined || this.categoryList == undefined ||
         this.categoryList.entryList == undefined || this.categoryList.entryList.length == 0 ||
         this.racePanel == undefined)
         return "menu-not-ready";
      if(this.modeSelect.getMode() != 0) return "sliders-tab-inactive";
      if(this.bRaceChanging) return "race-change-pending";
      if(!this.bMenuInitialized || this.vrDiagnosticRebuilding) return "sliders-rebuilding";
      if(!this.racePanel._visible || this.itemList.entryList == undefined) return "sliders-unavailable";
      if(this.colorField._visible || this.makeupPanel._visible || this.textEntry._visible)
         return "modal-open";
      if(this.itemList.disableInput || this.itemList.disableSelection) return "slider-input-disabled";
      return "ready";
   }
   function RefreshVRDiagnosticControls()
   {
      // Each query publishes a new token, even if a previous query was ready.
      this.InvalidateVRDiagnosticControls();
      var state = this.VRDiagnosticAvailability();
      var snapshot = {schema:1,status:state,generation:this.vrDiagnosticGeneration,
                      mode:this.modeSelect != undefined && this.modeSelect.getMode != undefined ? this.modeSelect.getMode() : -1,
                      sliders:[],races:[],allCoversEverySlider:true};
      this.vrDiagnosticSnapshot = snapshot;
      if(state != "ready")
      {
         this.PublishVRDiagnosticSnapshotJson();
         return 0;
      }
      var entries = this.itemList.entryList;
      this.vrDiagnosticIdentity = {entries:this.VRDiagnosticCaptureIdentity(entries),
         categories:this.VRDiagnosticCaptureIdentity(this.categoryList.entryList)};
      var allMask = this.categoryList.entryList[0].flag;
      var raceEnabled = this.VRDiagnosticRaceCategoryEnabled();
      for(var i = 0; i < entries.length; i++)
      {
         var entry = entries[i];
         if(entry.type == RaceMenuDefines.ENTRY_TYPE_SLIDER && this.VRDiagnosticSliderOffered(entry))
         {
            var enabled = Boolean(entry.enabled);
            if(entry.hasColor != undefined && entry.hasColor() &&
               entry.isColorEnabled != undefined && !entry.isColorEnabled()) enabled = false;
            var inAll = (entry.filterFlag & allMask) != 0;
            if(!inAll) snapshot.allCoversEverySlider = false;
            snapshot.sliders.push({slot:i,id:entry.sliderID,name:entry.text,
               callback:entry.callbackName,minimum:entry.sliderMin,maximum:entry.sliderMax,
               step:entry.interval,value:entry.position,enabled:enabled,
               categoryMask:entry.filterFlag,inAll:inAll,
               action:this.VRDiagnosticIsSexSlider(entry) ? "select-sex" : "set-slider"});
         }
         else if(entry.type == RaceMenuDefines.ENTRY_TYPE_RACE)
         {
            snapshot.races.push({slot:i,id:entry.raceID,name:entry.text,
               active:entry == this.itemList.listState.activeEntry,enabled:raceEnabled && Boolean(entry.enabled)});
         }
      }
      this.PublishVRDiagnosticSnapshotJson();
      return snapshot.sliders.length;
   }
   function VRDiagnosticRaceCategoryEnabled()
   {
      if(this.bLimitedMenu) return false;
      for(var i = 0; i < this.categoryList.entryList.length; i++)
      {
         var category = this.categoryList.entryList[i];
         if(category.flag == RaceMenuDefines.CATEGORY_RACE)
            return Boolean(category.enabled) && category.filterFlag != 0;
      }
      return false;
   }
   function VRDiagnosticCaptureIdentity(entries)
   {
      // Freeze routing, range and availability values separately from the live
      // objects. Object identity alone does not detect in-place list updates.
      var fields = ["type","sliderID","raceID","text","callbackName","vrNativeExtension",
         "vrProvider","vrControl","sliderMin","sliderMax","interval","position",
         "enabled","filterFlag","flag","hasColor","isColorEnabled"];
      var captured = [];
      for(var i = 0; i < entries.length; i++)
      {
         var values = {};
         for(var j = 0; j < fields.length; j++) values[fields[j]] = entries[i][fields[j]];
         captured.push({entry:entries[i],values:values});
      }
      return captured;
   }
   function VRDiagnosticIdentityMatches(captured, entries)
   {
      if(captured == undefined || captured.length != entries.length) return false;
      for(var i = 0; i < captured.length; i++)
      {
         if(captured[i].entry !== entries[i]) return false;
         for(var field in captured[i].values)
            if(captured[i].values[field] !== entries[i][field]) return false;
      }
      return true;
   }
   function VRDiagnosticSliderOffered(entry)
   {
      // Include All: hidden extension sections deliberately keep their
      // controls reachable through the All category.
      for(var i = 0; i < this.categoryList.entryList.length; i++)
      {
         var category = this.categoryList.entryList[i];
         if(category.enabled && category.filterFlag != 0 &&
            (entry.filterFlag & category.flag) != 0) return true;
      }
      return false;
   }
   function VRDiagnosticReject(code, message)
   {
      this.vrDiagnosticResult = {ok:false,code:code,message:message,
                                 generation:this.vrDiagnosticGeneration};
      this.PublishVRDiagnosticResultJson();
      return false;
   }
   function VRDiagnosticValidateRequest(generation)
   {
      var state = this.VRDiagnosticAvailability();
      if(state != "ready") return this.VRDiagnosticReject(state,"RaceMenu sliders are not currently available: " + state);
      if(typeof generation != "number" || generation != this.vrDiagnosticGeneration)
         return this.VRDiagnosticReject("stale-generation","The live menu or slider set changed; query controls again");
      if(this.vrDiagnosticSnapshot == undefined || this.vrDiagnosticSnapshot.status != "ready" ||
         this.vrDiagnosticSnapshot.generation != generation || this.vrDiagnosticIdentity == undefined)
         return this.VRDiagnosticReject("snapshot-required","Query the ready live controls before submitting an action");
      if(!this.VRDiagnosticIdentityMatches(this.vrDiagnosticIdentity.entries,this.itemList.entryList) ||
         !this.VRDiagnosticIdentityMatches(this.vrDiagnosticIdentity.categories,this.categoryList.entryList))
      {
         this.InvalidateVRDiagnosticControls();
         return this.VRDiagnosticReject("slider-identity-changed","The published controls changed or reordered; query controls again");
      }
      return true;
   }
   function VRDiagnosticFindEntry(slot, type)
   {
      if(typeof slot != "number" || isNaN(slot) || slot < 0 || Math.floor(slot) != slot ||
         slot >= this.itemList.entryList.length) return null;
      var entry = this.itemList.entryList[slot];
      return entry != undefined && entry.type == type ? entry : null;
   }
   function VRDiagnosticValidValue(entry, value)
   {
      if(typeof value != "number" || isNaN(value) || Math.abs(value) >= 1e308 ||
         typeof entry.sliderMin != "number" || typeof entry.sliderMax != "number" ||
         isNaN(entry.sliderMin) || isNaN(entry.sliderMax) ||
         Math.abs(entry.sliderMin) >= 1e308 || Math.abs(entry.sliderMax) >= 1e308 ||
         value < entry.sliderMin || value > entry.sliderMax) return false;
      var step = Number(entry.interval);
      if(isNaN(step) || Math.abs(step) >= 1e308 || step < 0) return false;
      if(step > 0)
      {
         var units = (value - entry.sliderMin) / step;
         if(Math.abs(units - Math.round(units)) > 0.0001) return false;
      }
      return true;
   }
   function VRDiagnosticApplySlider(generation, slot, value, allowSex)
   {
      this.vrDiagnosticRequest = {generation:generation,slot:slot,value:value};
      if(!this.VRDiagnosticValidateRequest(generation)) return false;
      var entry = this.VRDiagnosticFindEntry(slot,RaceMenuDefines.ENTRY_TYPE_SLIDER);
      if(entry == null) return this.VRDiagnosticReject("slider-not-live","That slot is not a live slider; query controls again");
      this.vrDiagnosticRequest.sliderId = entry.sliderID;
      this.vrDiagnosticRequest.callback = entry.callbackName;
      var request = this.vrDiagnosticRequest;
      if(!this.VRDiagnosticSliderOffered(entry))
         return this.VRDiagnosticReject("slider-not-offered","The live menu offers no category for this slider");
      if(this.VRDiagnosticIsSexSlider(entry) && !allowSex)
         return this.VRDiagnosticReject("use-select-sex","Sex changes require SelectVRDiagnosticSex");
      if(!this.VRDiagnosticIsSexSlider(entry) && allowSex)
         return this.VRDiagnosticReject("not-sex-slider","That slot is not the live sex selector");
      if(!entry.enabled || entry.hasColor != undefined && entry.hasColor() &&
         entry.isColorEnabled != undefined && !entry.isColorEnabled())
         return this.VRDiagnosticReject("slider-disabled","The live menu has disabled this slider");
      if(!this.VRDiagnosticValidValue(entry,value))
         return this.VRDiagnosticReject("invalid-value","Value must lie within the live slider range and step");
      if(entry.vrNativeExtension)
      {
         var api = _global.skse.plugins.CharGen;
         if(api.SetMenuExtensionValue == undefined)
            return this.VRDiagnosticReject("callback-unavailable","The extension menu callback is unavailable");
         this.InvalidateVRDiagnosticControls();
         // This GFx callback is asynchronous and returns no acknowledgement.
         api.SetMenuExtensionValue(entry.vrProvider,entry.vrControl,value);
      }
      else
      {
         if(entry.callbackName == undefined || entry.callbackName == "")
            return this.VRDiagnosticReject("callback-unavailable","This slider has no active menu callback");
         this.InvalidateVRDiagnosticControls();
         // This is the same GameDelegate callback and argument pair used by
         // SliderListEntry.changedCallback, not a direct engine-data write.
         gfx.io.GameDelegate.call(entry.callbackName,[value,entry.sliderID]);
         gfx.io.GameDelegate.call("PlaySound",["UIMenuFocus"]);
         skse.SendModEvent(_global.eventPrefix + "SliderChange",entry.callbackName,value);
      }
      entry.position = value;
      if(!entry.vrNativeExtension && entry.sliderID != undefined &&
         _global.skse.plugins.CharGen.GetSliderData != undefined)
         entry.extraData = _global.skse.plugins.CharGen.GetSliderData(entry.sliderID,value);
      this.itemList.requestUpdate();
      if(this.itemList.listState.selectedEntry == entry) this.updateItemDescriptor();
      this.vrDiagnosticRequest = request;
      this.vrDiagnosticResult = {ok:true,code:"dispatched",message:"Menu callback dispatched; observe the resulting menu state",
                                 generation:this.vrDiagnosticGeneration,slot:slot,value:value};
      this.PublishVRDiagnosticResultJson();
      return true;
   }
   function SetVRDiagnosticSlider(generation, slot, value)
   {
      return this.VRDiagnosticApplySlider(generation,slot,value,false);
   }
   function SelectVRDiagnosticSex(generation, slot, value)
   {
      return this.VRDiagnosticApplySlider(generation,slot,value,true);
   }
   function SelectVRDiagnosticRace(generation, raceId)
   {
      this.vrDiagnosticRequest = {generation:generation,raceId:raceId};
      var request = this.vrDiagnosticRequest;
      if(!this.VRDiagnosticValidateRequest(generation)) return false;
      if(typeof raceId != "number" || isNaN(raceId) || Math.floor(raceId) != raceId)
         return this.VRDiagnosticReject("invalid-race","Race ID must be one of the enumerated live choices");
      if(!this.VRDiagnosticRaceCategoryEnabled())
         return this.VRDiagnosticReject("race-disabled","The live menu does not permit race selection");
      for(var i = 0; i < this.itemList.entryList.length; i++)
      {
         var entry = this.itemList.entryList[i];
         if(entry.type != RaceMenuDefines.ENTRY_TYPE_RACE || entry.raceID != raceId) continue;
         if(!entry.enabled) return this.VRDiagnosticReject("race-disabled","The selected race is disabled");
         if(entry == this.itemList.listState.activeEntry)
            return this.VRDiagnosticReject("already-selected","That race is already active");
         // onItemPress is the physical race-row path: it sets the active row,
         // opens the loading lifecycle and invokes Skyrim's ChangeRace callback.
         this.InvalidateVRDiagnosticControls();
         this.onItemPress({index:i});
         this.vrDiagnosticRequest = request;
         this.vrDiagnosticResult = {ok:true,code:"dispatched",message:"Race selection dispatched; wait for the menu to rebuild",
                                    generation:this.vrDiagnosticGeneration,raceId:raceId};
         this.PublishVRDiagnosticResultJson();
         return true;
      }
      return this.VRDiagnosticReject("race-not-live","That race is not offered by the live menu");
   }
