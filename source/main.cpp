#include <3ds.h>
#include <citro2d.h>

#include "activities/main_activity.hpp"
#include "ui/lifecycle.hpp"

namespace {

constexpr float StereoSliderThreshold = 0.05f;

struct RuntimeContext {
    C3D_RenderTarget* topLeft = nullptr;
    C3D_RenderTarget* topRight = nullptr;
    C3D_RenderTarget* bottom = nullptr;
    aptHookCookie aptCookie = {};
    lunar3d::ui::ActivityManager activityManager;
    bool stereoEnabled = false;
    bool skipFrame = false;
};

void updateStereoEnabled(RuntimeContext& runtime) {
    const bool shouldEnable = osGet3DSliderState() > StereoSliderThreshold;
    if (runtime.stereoEnabled == shouldEnable) {
        return;
    }

    gfxSet3D(shouldEnable);
    runtime.stereoEnabled = shouldEnable;
}

void onAptHook(APT_HookType hook, void* param) {
    RuntimeContext& runtime = *static_cast<RuntimeContext*>(param);

    if (hook == APTHOOK_ONSUSPEND || hook == APTHOOK_ONSLEEP) {
        runtime.activityManager.stop();
    } else if (hook == APTHOOK_ONRESTORE) {
        runtime.activityManager.restart();
        runtime.skipFrame = true;
    } else if (hook == APTHOOK_ONWAKEUP) {
        runtime.activityManager.restart();
        runtime.skipFrame = true;
    }
}

void renderActivityTarget(const lunar3d::ui::Activity& activity, C3D_RenderTarget& target) {
    C2D_TargetClear(&target, activity.backgroundColor(target.screen));
    C2D_SceneBegin(&target);
    activity.render(target);
}

} // namespace

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    // Initialize runtime
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();
    const bool psInitialized = R_SUCCEEDED(psInit());
    const bool httpcInitialized = R_SUCCEEDED(httpcInit(0));

    RuntimeContext runtime;
    lunar3d::activities::MainActivity mainActivity(runtime.activityManager);
    runtime.topLeft = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    runtime.topRight = C2D_CreateScreenTarget(GFX_TOP, GFX_RIGHT);
    runtime.bottom = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
    updateStereoEnabled(runtime);

    // Main loop
    u64 lastTick = 0;
    runtime.activityManager.launch(mainActivity);
    aptHook(&runtime.aptCookie, onAptHook, &runtime);

    runtime.skipFrame = true;
    while (aptMainLoop() && runtime.activityManager.active() != nullptr) {
        // after resume or for first tick
        if (runtime.skipFrame) {
            runtime.skipFrame = false;
            lastTick = osGetTime();
            gspWaitForVBlank();
            continue;
        }

        // Calculate delta
        const u64 now = osGetTime();
        const u64 deltaMilli = now - lastTick;

        hidScanInput();

        lunar3d::ui::HidInput input;
        input.keysDown = hidKeysDown();
        input.keysHeld = hidKeysHeld();
        input.keysUp = hidKeysUp();
        hidTouchRead(&input.touch);
        hidCircleRead(&input.circlePad);

        if (!runtime.activityManager.update(input, deltaMilli)) {
            continue;
        }

        updateStereoEnabled(runtime);

        const lunar3d::ui::Activity& activity = *runtime.activityManager.active();

        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        renderActivityTarget(activity, *runtime.topLeft);
        if (runtime.stereoEnabled) {
            renderActivityTarget(activity, *runtime.topRight);
        }
        renderActivityTarget(activity, *runtime.bottom);
        C3D_FrameEnd(0);

        lastTick = now;
    }

    // Shutdown runtime
    aptUnhook(&runtime.aptCookie);
    runtime.activityManager.destroyAll();
    C2D_Fini();
    C3D_Fini();

    if (httpcInitialized) {
        httpcExit();
    }
    if (psInitialized) {
        psExit();
    }

    gfxExit();
    return 0;
}
