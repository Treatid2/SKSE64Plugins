   // Share two normal list entries by category mask, not by cloning widgets.
   // Stock list selection, slider loading and callbacks own their input.
   function IsVRViewControl(entry)
   {
      return entry.type == RaceMenuDefines.ENTRY_TYPE_SLIDER && entry.vrNativeExtension &&
         entry.vrViewBaseFilter != undefined &&
         entry.vrProvider == "RaceMenuVR2" &&
         (entry.vrControl == "avatarYaw" || entry.vrControl == "viewYaw");
   }
   function RefreshVRViewCategoryControls()
   {
      var items = this.itemList.entryList;
      var categories = this.categoryList.entryList;
      var present = false;
      for(var i = 0; i < items.length; i++) if(this.IsVRViewControl(items[i])) present = true;
      if(!present) return false;
      var mask = 0;
      // All already includes the extension's own flag. Only actual enabled
      // categories contribute; Race must remain a separate race-choice list.
      for(var i = 1; i < categories.length; i++)
      {
         var category = categories[i];
         if(category.enabled != false && category.filterFlag != 0 &&
            !(category.flag & RaceMenuDefines.CATEGORY_RACE)) mask |= category.flag;
      }
      var minimum = 0;
      for(var i = 0; i < items.length; i++)
      {
         var entry = items[i];
         if(entry.type == RaceMenuDefines.ENTRY_TYPE_SLIDER && !this.IsVRViewControl(entry) &&
            entry.priority != undefined && !isNaN(entry.priority) && Math.abs(entry.priority) < 1e308)
            minimum = Math.min(minimum,entry.priority);
      }
      var avatarPriority = minimum-2;
      var viewPriority = minimum-1;
      if(!(avatarPriority < viewPriority && viewPriority < minimum))
      {
         this.vrViewCategoryFault = "view controls cannot precede category priorities";
         return false;
      }
      var changed = false;
      for(var i = 0; i < items.length; i++)
      {
         var entry = items[i];
         if(entry.type != RaceMenuDefines.ENTRY_TYPE_SLIDER) continue;
         var view = this.IsVRViewControl(entry);
         // Neutral defaults allow numeric sorting; preserve explicit priorities
         // and backing-array order (including diagnostic slot identities).
         var priority = view ? (entry.vrControl == "avatarYaw" ? avatarPriority : viewPriority) : entry.priority;
         if(priority == undefined) priority = 0;
         var flags = view ? ((entry.vrViewBaseFilter | mask) & ~RaceMenuDefines.CATEGORY_RACE) : entry.filterFlag;
         if(entry.priority != priority || entry.filterFlag != flags)
         {
            if(!changed) this.InvalidateVRDiagnosticControls();
            changed = true;
            entry.priority = priority;
            entry.filterFlag = flags;
         }
      }
      if(changed) this.itemList.requestInvalidate();
      return changed;
   }
