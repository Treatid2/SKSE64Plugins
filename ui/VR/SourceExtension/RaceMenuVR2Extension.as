// Standalone side-loaded movie; no upstream/SkyUI implementation is compiled.
dynamic class RaceMenuVR2Extension extends MovieClip
{
// BEGIN LIFECYCLE
// Original AS2 extension code. Upstream RaceMenu classes are not imported.
function SourceExtensionInvalidate(owner)
{
   // Optional diagnostics must neither suppress original callbacks nor recurse.
   if(owner == undefined || owner.sourceExtensionInvalidating) return;
   owner.sourceExtensionInvalidating = true;
   try
   {
      owner.sourceExtensionRevision = Number(owner.sourceExtensionRevision || 0) + 1;
      if(typeof owner.InvalidateDiagnosticControls == "function")
         owner.InvalidateDiagnosticControls();
   }
   catch(error)
   {
      owner.sourceExtensionNotificationStatus = "invalidation-failed";
   }
   finally
   {
      delete owner.sourceExtensionInvalidating;
   }
}
function SourceExtensionAttach(owner)
{
   if(this.sourceExtensionAttaching || this.sourceExtensionDetaching) return false;
   if(owner == undefined || typeof owner.SetSliders != "function" ||
      typeof owner.SetCategoriesList != "function" ||
      typeof owner.onItemPress != "function" || owner.modeSelect == undefined ||
      typeof owner.hasOwnProperty != "function")
      return false;
   // One child owns at most one lease; changing parents requires detach first.
   if(this.sourceExtensionLease != undefined)
      return this.sourceExtensionLease.owner === owner &&
         owner.sourceExtensionLease === this.sourceExtensionLease;
   if(owner.sourceExtensionLease != undefined)
      return false;
   var lease = {module:this,owner:owner,active:true,states:{},wrappers:{}};
   var methods = ["SetSliders","SetCategoriesList","SetRaceList","onItemPress"];
   for(var i = 0; i < methods.length; i++)
   {
      var name = methods[i];
      if(typeof owner[name] != "function") continue;
      var state = {active:true,lease:lease,module:this,owner:owner,
         original:owner[name],hadOwn:owner.hasOwnProperty(name)};
      var wrapper = this.SourceExtensionWrap(state);
      lease.states[name] = state;
      lease.wrappers[name] = wrapper;
      owner[name] = wrapper;
   }
   owner.sourceExtensionLease = lease;
   owner.sourceExtensionStatus = "attached";
   owner.sourceExtensionSchema = 1;
   this.sourceExtensionLease = lease;
   // A notification cannot re-attach this child while attachment is publishing.
   this.sourceExtensionAttaching = true;
   try { this.SourceExtensionInvalidate(owner); }
   finally { delete this.sourceExtensionAttaching; }
   // Optional code may have detached synchronously during notification.
   return this.sourceExtensionLease === lease && owner.sourceExtensionLease === lease;
}
function SourceExtensionWrap(state)
{
   // Capture only this per-method state, not the child or full lease directly.
   return function()
   {
      // A detached wrapper retained by another component must still delegate.
      if(state.active && state.lease.active &&
         state.owner.sourceExtensionLease === state.lease)
         state.module.SourceExtensionInvalidate(state.owner);
      return state.original.apply(this, arguments);
   };
}
function SourceExtensionDetach()
{
   var lease = this.sourceExtensionLease;
   if(lease == undefined || this.sourceExtensionDetaching) return;
   this.sourceExtensionDetaching = true;
   var owner = lease.owner;
   var owned = owner.sourceExtensionLease === lease;
   lease.active = false;
   // Never overwrite a wrapper subsequently installed by another extension.
   for(var name in lease.wrappers)
   {
      var state = lease.states[name];
      state.active = false;
      if(owner[name] === lease.wrappers[name])
      {
         if(state.hadOwn) owner[name] = state.original;
         else delete owner[name];
      }
      // Later wrappers may retain state; only original delegation is needed.
      delete state.module;
      delete state.owner;
      delete state.lease;
   }
   if(owned)
   {
      delete owner.sourceExtensionLease;
      owner.sourceExtensionStatus = "detached";
   }
   delete this.sourceExtensionLease;
   delete lease.module;
   delete lease.owner;
   delete lease.states;
   delete lease.wrappers;
   // Cleanup is complete before optional code runs; re-entry cannot reacquire.
   try { if(owned) this.SourceExtensionInvalidate(owner); }
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
