// Standalone side-loaded movie; no upstream/SkyUI implementation is compiled.
dynamic class RaceMenuVR2Extension extends MovieClip
{
// BEGIN LIFECYCLE
// Original AS2 extension code. Upstream RaceMenu classes are not imported.
function SourceExtensionInvalidate(owner)
{
   // Optional diagnostics must neither suppress original callbacks nor recurse.
   if(owner == undefined || this.sourceExtensionNotifying) return;
   this.sourceExtensionNotifying = true;
   try
   {
      owner.sourceExtensionRevision = Number(owner.sourceExtensionRevision || 0) + 1;
      if(typeof owner.InvalidateDiagnosticControls == "function")
         owner.InvalidateDiagnosticControls();
   }
   catch(error)
   {
      this.sourceExtensionNotificationStatus = "invalidation-failed";
      try { owner.sourceExtensionNotificationStatus = "invalidation-failed"; }
      catch(statusError) { /* Optional owner reporting may also fail. */ }
   }
   finally
   {
      delete this.sourceExtensionNotifying;
   }
}
function SourceExtensionAttach(owner)
{
   if(this.sourceExtensionAttaching || this.sourceExtensionDetaching) return false;
   // Child-owned guard precedes even owner getters/watch handlers.
   this.sourceExtensionAttaching = true;
   var lease;
   var success = false;
   try
   {
      if(owner == undefined || typeof owner.SetSliders != "function" ||
         typeof owner.SetCategoriesList != "function" ||
         typeof owner.onItemPress != "function" || owner.modeSelect == undefined ||
         typeof owner.hasOwnProperty != "function") return false;
      // One child owns at most one lease; changing parents requires detach first.
      if(this.sourceExtensionLease != undefined)
         return this.sourceExtensionLease.owner === owner &&
            owner.sourceExtensionLease === this.sourceExtensionLease;
      if(owner.sourceExtensionLease != undefined) return false;
      lease = {module:this,owner:owner,active:false,states:{},wrappers:{},metadata:{}};
      var methods = ["SetSliders","SetCategoriesList","SetRaceList","onItemPress"];
      for(var i = 0; i < methods.length; i++)
      {
         var name = methods[i];
         var original = owner[name];
         if(typeof original != "function") continue;
         var state = {active:false,lease:lease,module:this,owner:owner,
            original:original,hadOwn:owner.hasOwnProperty(name)};
         lease.states[name] = state;
         lease.wrappers[name] = this.SourceExtensionWrap(state);
      }
      var fields = ["sourceExtensionLease","sourceExtensionStatus","sourceExtensionSchema"];
      var values = [lease,"attached",1];
      for(var j = 0; j < fields.length; j++)
      {
         name = fields[j];
         lease.metadata[name] = {original:owner[name],hadOwn:owner.hasOwnProperty(name),value:values[j]};
      }
      // All snapshots exist before the first mutation. Detach during setup
      // requests cancellation; it cannot retire state while setters are running.
      this.sourceExtensionLease = lease;
      for(var method in lease.wrappers)
      {
         owner[method] = lease.wrappers[method];
         if(owner[method] !== lease.wrappers[method] || lease.cancelled)
            throw "wrapper-publication-failed";
      }
      for(j = 0; j < fields.length; j++)
      {
         name = fields[j];
         owner[name] = lease.metadata[name].value;
         if(owner[name] !== lease.metadata[name].value || lease.cancelled)
            throw "lease-publication-failed";
      }
      // A setter for a later field may have changed an earlier field.
      for(method in lease.wrappers)
         if(owner[method] !== lease.wrappers[method]) throw "wrapper-changed";
      for(j = 0; j < fields.length; j++)
         if(owner[fields[j]] !== lease.metadata[fields[j]].value) throw "lease-changed";
      if(lease.cancelled) throw "attachment-cancelled";
      lease.active = true;
      for(method in lease.states) lease.states[method].active = true;
      this.SourceExtensionInvalidate(owner);
      // Optional notification may detach synchronously after commit.
      success = lease.active && this.sourceExtensionLease === lease &&
         owner.sourceExtensionLease === lease;
      if(success) this.sourceExtensionStatus = "attached";
      return success;
   }
   catch(error)
   {
      this.sourceExtensionStatus = "attachment-failed";
      return false;
   }
   finally
   {
      try
      {
         if(lease != undefined && !success && this.sourceExtensionLease === lease)
         {
            var clean = this.SourceExtensionRelease(lease, true);
            this.sourceExtensionStatus = clean ? "attachment-failed" : "attachment-rollback-incomplete";
         }
      }
      finally { delete this.sourceExtensionAttaching; }
   }
}
function SourceExtensionWrap(state)
{
   // Capture only this per-method state, not the child or full lease directly.
   return function()
   {
      // A detached wrapper retained by another component must still delegate.
      try
      {
         if(state.active && state.lease.active &&
            state.owner.sourceExtensionLease === state.lease)
            state.module.SourceExtensionInvalidate(state.owner);
      }
      catch(notificationError) { /* Owner getters cannot suppress the original. */ }
      return state.original.apply(this, arguments);
   };
}
function SourceExtensionRestore(owner, name, expected, state)
{
   try
   {
      // Never overwrite a later third-party wrapper or lease.
      if(owner[name] !== expected) return true;
      if(state.hadOwn)
      {
         owner[name] = state.original;
         return owner[name] === state.original && owner.hasOwnProperty(name);
      }
      delete owner[name];
      return !owner.hasOwnProperty(name) && owner[name] !== expected;
   }
   catch(error) { return false; }
}
function SourceExtensionRelease(lease, rollback)
{
   var owner = lease.owner;
   var clean = true;
   lease.active = false;
   for(var name in lease.states)
   {
      var state = lease.states[name];
      state.active = false;
      if(!this.SourceExtensionRestore(owner, name, lease.wrappers[name], state)) clean = false;
      // Later wrappers may retain state; only original delegation is needed.
      delete state.module;
      delete state.owner;
      delete state.lease;
   }
   var fields = rollback ? ["sourceExtensionSchema","sourceExtensionStatus","sourceExtensionLease"] : ["sourceExtensionLease"];
   for(var i = 0; i < fields.length; i++)
   {
      name = fields[i];
      if(!this.SourceExtensionRestore(owner, name, lease.metadata[name].value, lease.metadata[name])) clean = false;
   }
   if(this.sourceExtensionLease === lease) delete this.sourceExtensionLease;
   delete lease.module;
   delete lease.owner;
   delete lease.states;
   delete lease.wrappers;
   delete lease.metadata;
   return clean;
}
function SourceExtensionDetach()
{
   var lease = this.sourceExtensionLease;
   if(lease == undefined || this.sourceExtensionDetaching) return;
   if(this.sourceExtensionAttaching && !lease.active)
   {
      lease.cancelled = true;
      return;
   }
   this.sourceExtensionDetaching = true;
   var owner = lease.owner;
   var owned = false;
   var clean = true;
   try
   {
      try { owned = owner.sourceExtensionLease === lease; }
      catch(ownerError) { clean = false; }
      if(!this.SourceExtensionRelease(lease, false)) clean = false;
      if(owned)
      {
         try
         {
            owner.sourceExtensionStatus = clean ? "detached" : "detach-incomplete";
            if(owner.sourceExtensionStatus !== (clean ? "detached" : "detach-incomplete")) clean = false;
         }
         catch(statusError) { clean = false; }
      }
      this.sourceExtensionStatus = clean ? "detached" : "detach-incomplete";
   // Cleanup is complete before optional code runs; re-entry cannot reacquire.
      if(owned) this.SourceExtensionInvalidate(owner);
   }
   finally { delete this.sourceExtensionDetaching; }
}
function SourceExtensionTick()
{
   if(this.SourceExtensionAttach(this._parent))
   {
      delete this.onEnterFrame;
      return;
   }
   this.sourceExtensionAttempts++;
   if(this.sourceExtensionAttempts >= 120)
   {
      this.sourceExtensionStatus = "unsupported-owner";
      delete this.onEnterFrame;
   }
}
function SourceExtensionStart()
{
   this.sourceExtensionAttempts = 0;
   this.sourceExtensionStatus = "waiting-for-owner";
   this.onEnterFrame = this.SourceExtensionTick;
   this.onUnload = this.SourceExtensionDetach;
}
// END LIFECYCLE

   static function main(movie:MovieClip):Void
   {
      // MTASC's -main entry receives the loaded movie root.
      // Inherit MovieClip APIs and keep ownership local to this child movie.
      movie.__proto__ = RaceMenuVR2Extension.prototype;
      movie.SourceExtensionStart();
   }
}
