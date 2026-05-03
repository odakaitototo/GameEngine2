#include "Engine/Core/Application.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    Application app;

    // 1280x720ÇÃÉTÉCÉYÇ≈èâä˙âª
    if (app.Initialize(hInstance, 1280, 720)) {
        app.Run();
    }

    app.Terminate();
    return 0;
}