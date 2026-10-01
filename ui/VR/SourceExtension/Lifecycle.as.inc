// Original AS2 extension code. Upstream RaceMenu classes are not imported.
function SourceExtensionInvalidate(owner)
{
   if(typeof owner.InvalidateDiagnosticControls == "function")
      owner.InvalidateDiagnosticControls();
   owner.sourceExtensionRevision = Number(owner.sourceExtensionRevision || 0) + 1;
}
function SourceExtensionAttach(owner)
{
   if(owner == undefined || typeof owner.SetSliders != "function" ||
      typeof owner.SetCategoriesList != "function" ||
      typeof owner.onItemPress != "function" || owner.modeSelect == undefined)
      return false;
   if(owner.sourceExtensionLease != undefined)
      return owner.sourceExtensionLease.module === this;
   var module = this;
   var lease = {module:module,owner:owner,originals:{},wrappers:{}};
   var methods = ["SetSliders","SetCategoriesList","SetRaceList","onItemPress"];
   for(var i = 0; i < methods.length; i++)
   {
      var name = methods[i];
      if(typeof owner[name] != "function") continue;
      var wrapper = this.SourceExtensionWrap(lease, name, owner[name]);
      lease.originals[name] = owner[name];
      lease.wrappers[name] = wrapper;
      owner[name] = wrapper;
   }
   owner.sourceExtensionLease = lease;
   owner.sourceExtensionStatus = "attached";
   owner.sourceExtensionSchema = 1;
   this.sourceExtensionLease = lease;
   this.SourceExtensionInvalidate(owner);
   return true;
}
function SourceExtensionWrap(lease, name, original)
{
   var module = this;
   return function()
   {
      // A detached wrapper retained by another component must still delegate.
      if(lease.owner.sourceExtensionLease === lease)
         module.SourceExtensionInvalidate(lease.owner);
      return original.apply(this, arguments);
   };
}
function SourceExtensionDetach()
{
   var lease = this.sourceExtensionLease;
   if(lease == undefined) return;
   var owner = lease.owner;
   // Never overwrite a wrapper subsequently installed by another extension.
   for(var name in lease.wrappers)
      if(owner[name] === lease.wrappers[name]) owner[name] = lease.originals[name];
   if(owner.sourceExtensionLease === lease)
   {
      this.SourceExtensionInvalidate(owner);
      delete owner.sourceExtensionLease;
      owner.sourceExtensionStatus = "detached";
   }
   delete this.sourceExtensionLease;
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
