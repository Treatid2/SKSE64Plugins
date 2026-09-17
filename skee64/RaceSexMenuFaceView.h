#pragma once
namespace RE { class GFxMovie; class GFxValue; }
namespace SKEE::FaceView
{
    void Configure(bool enabled, float distance, float eyeHeight = 5);
    bool Supported();
    unsigned Current();
    bool Request(unsigned view);
    void Restore();
    void Register(RE::GFxMovie* movie, RE::GFxValue* root);
}
