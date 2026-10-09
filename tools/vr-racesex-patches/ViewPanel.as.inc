   // Original VR panel glue; the stock renderer/artwork remain upstream assets.
   // Reuse the exact SliderListEntry linkage already present in the pinned movie.
   function VRViewPanelAvailable()
   {
      return this.bMenuInitialized && !this.bRaceChanging && !this.vrDiagnosticRebuilding &&
         this.modeSelect.getMode() == 0 && this.racePanel._visible &&
         !this.itemList.disableInput && !this.itemList.disableSelection &&
         !this.bTextEntryMode && !this.vrTextInputActive &&
         !this.colorField._visible && !this.makeupPanel._visible && !this.textEntry._visible;
   }
   function ReleaseVRViewPanel()
   {
      var panel = this.vrViewPanel;
      if(panel == undefined) return undefined;
      // Invalidate callbacks before removing clips. Do not leave an owned timer.
      this.vrViewPanel = undefined;
      for(var i = 0; i < panel.rows.length; i++)
      {
         var row = panel.rows[i];
         row.vrOwner = undefined;
         clearInterval(row.rateInterval);
         row.SliderInstance.disabled = true;
         row.removeMovieClip();
      }
      panel.removeMovieClip();
   }
   function VRViewPanelSection(sections)
   {
      for(var i = 0; i < sections.length; i++)
         if(sections[i].provider == "RaceMenuVR2" && sections[i].id == "view") return sections[i];
      return undefined;
   }
   function VRViewPanelControls(section)
   {
      if(section == undefined) return undefined;
      var result = [];
      var ids = ["avatarYaw","viewYaw"];
      for(var i = 0; i < ids.length; i++)
      {
         var found = undefined;
         for(var j = 0; j < section.controls.length; j++)
            if(section.controls[j].id == ids[i]) found = section.controls[j];
         if(found == undefined || isNaN(found.minimum) || isNaN(found.maximum) ||
            isNaN(found.step) || isNaN(found.value) || found.minimum >= found.maximum ||
            Math.abs(found.minimum) >= 1e308 || Math.abs(found.maximum) >= 1e308 ||
            found.step <= 0 || found.step > found.maximum-found.minimum ||
            found.value < found.minimum || found.value > found.maximum) return undefined;
         result.push(found);
      }
      return result;
   }
   function RefreshVRViewPanel(sections, revision)
   {
      // Non-consolidated profiles retain the normal category placement; do not
      // place a second panel at an unqualified origin or cover legacy controls.
      var controls = this.vrMenuProfile != undefined && this.vrMenuProfile.consolidate == 1 ?
         this.VRViewPanelControls(this.VRViewPanelSection(sections)) : undefined;
      if(controls == undefined)
      {
         if(this.vrViewPanel != undefined) { this.ReleaseVRViewPanel(); this.LayoutVRPanel(); }
         return false;
      }
      var panel = this.vrViewPanel;
      if(panel == undefined)
      {
         panel = this.createEmptyMovieClip("vrViewPanel",this.getNextHighestDepth());
         if(panel == undefined) return false;
         this.vrViewPanel = panel;
         panel.rows = [];
         for(var i = 0; i < controls.length; i++)
         {
            var control = controls[i];
            var row = panel.attachMovie("SliderListEntry","row"+i,panel.getNextHighestDepth());
            if(row == undefined || row.setEntry == undefined || row.SliderInstance == undefined)
            {
               if(row != undefined) row.removeMovieClip();
               this.vrViewPanelFault = "stock view slider renderer unavailable";
               this.ReleaseVRViewPanel();
               return false;
            }
            panel.rows.push(row);
            row.vrOwner = this;
            // No list-selection handlers: these rows live outside the scroll list.
            row.itemIndex = undefined;
            var entry = {type:RaceMenuDefines.ENTRY_TYPE_SLIDER,text:control.label,callbackName:"",
               sliderMin:control.minimum,sliderMax:control.maximum,sliderID:0,
               position:control.value,interval:control.step,enabled:true,
               vrNativeExtension:true,vrProvider:"RaceMenuVR2",vrControl:control.id};
            entry.GetTextureList = function() { return null; };
            entry.isColorEnabled = entry.hasColor = entry.hasGlow = function() { return false; };
            entry.internalCallback = function()
            {
               var row = this.entryClip;
               var owner = row.vrOwner;
               var api = _global.skse.plugins.CharGen;
               if(owner == undefined || owner.vrViewPanel != row._parent ||
                  !owner.VRViewPanelAvailable() || row.vrRevision != Number(api.GetMenuExtensionsRevision())) return undefined;
               api.SetMenuExtensionValue(this.entryObject.vrProvider,this.entryObject.vrControl,this.position);
            };
            row.vrEntry = entry;
            // Stock setEntry owns widget-load completion and the change listener.
            // Call once, not on every poll (which could interrupt a held drag).
            row.setEntry(entry,{list:{selectedEntry:null},activeEntry:null,focusEntry:null});
         }
         this.LayoutVRPanel();
      }
      var ready = this.VRViewPanelAvailable();
      panel._visible = this.modeSelect.getMode() == 0 && this.racePanel._visible &&
         !this.colorField._visible && !this.makeupPanel._visible && !this.textEntry._visible && !this.vrTextInputActive;
      panel._alpha = ready ? 100 : 40;
      for(var i = 0; i < panel.rows.length; i++)
      {
         var row = panel.rows[i];
         var control = controls[i];
         // The built-in ranges are fixed. A changed contract uses the category
         // fallback instead of quietly rebinding a live drag to a new range.
         if(row.vrEntry.sliderMin != control.minimum || row.vrEntry.sliderMax != control.maximum ||
            row.vrEntry.interval != control.step)
         {
            this.vrViewPanelFault = "view slider range changed";
            this.ReleaseVRViewPanel(); this.LayoutVRPanel(); return false;
         }
         row.vrRevision = revision;
         row.vrEntry.position = control.value;
         row.SliderInstance.disabled = !ready;
         if(row.SliderInstance.initialized && row.SliderInstance.position != control.value)
         {
            // Synchronisation is display-only: never changedCallback/SetValue.
            row.SliderInstance.position = control.value;
            row.valueField.SetText(((control.value*100)|0)/100);
         }
      }
      return true;
   }
   function LayoutVRViewPanel(panelBounds)
   {
      var panel = this.vrViewPanel;
      if(panel == undefined) return undefined;
      panel._x = panelBounds.xMin+12;
      panel._y = panelBounds.yMax-108;
      var width = panelBounds.xMax-panelBounds.xMin-24;
      for(var i = 0; i < panel.rows.length; i++)
      {
         var row = panel.rows[i];
         row._xscale = row._yscale = 100;
         var bounds = row.getBounds(row);
         var naturalWidth = bounds.xMax-bounds.xMin;
         if(naturalWidth > 0)
         {
            var naturalHeight = bounds.yMax-bounds.yMin;
            var scale = Math.min(100,width/naturalWidth*100,44/Math.max(1,naturalHeight)*100);
            row._xscale = row._yscale = scale;
            row._x = -bounds.xMin*scale/100;
            row._y = i*48-bounds.yMin*scale/100;
         }
      }
   }
