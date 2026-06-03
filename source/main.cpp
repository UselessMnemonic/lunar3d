#include <3ds.h>

#include <stdio.h>

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    gfxInitDefault();
    consoleInit(GFX_TOP, NULL);

    printf("Lunar3D\n");
    printf("Press START to exit.\n");

    while (aptMainLoop()) {
        hidScanInput();

        if (hidKeysDown() & KEY_START) {
            break;
        }

        gfxFlushBuffers();
        gfxSwapBuffers();
        gspWaitForVBlank();
    }

    gfxExit();
    return 0;
}
