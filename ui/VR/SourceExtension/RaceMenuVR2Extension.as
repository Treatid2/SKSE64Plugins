// Standalone side-loaded movie; no upstream/SkyUI implementation is compiled.
class RaceMenuVR2Extension extends MovieClip
{
   #include "Lifecycle.as.inc"

   static function main(movie:MovieClip):Void
   {
      // MTASC's -main entry receives the loaded movie root.
      // Inherit MovieClip APIs and keep ownership local to this child movie.
      movie.__proto__ = RaceMenuVR2Extension.prototype;
      movie.SourceExtensionStart();
   }
}
