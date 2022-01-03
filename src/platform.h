#pragma once

// Interface used to communicate with examples

#include "types.h"

#include "camera.h"

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))
#define OFFSETOF(TYPE, MEMBER) __builtin_offsetof(TYPE, MEMBER)

struct platform_io
{
    union
    {
        int ScreenWidth; // Deprecated (Replace ScreenWidth by WindowWidth)
        int WindowWidth;
    };
    union
    {
        int ScreenHeight;
        int WindowHeight;
    };
    
    double Time;
    double DeltaTime;

    bool MouseCaptured;
    float DeltaMouseX;
    float DeltaMouseY;
    float MouseX;
    float MouseY;

    camera_inputs CameraInputs;

    // F1 to F11 keys
    bool DebugKeysDown[11];
    bool DebugKeysPressed[11];

    // Events
    bool WindowSizeChanged;
};
