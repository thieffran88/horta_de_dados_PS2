/* Horta de Dados PS2 - ponto de entrada no console. */
#include "plat.h"
#include "gfx.h"
#include "ui.h"

static Ui ui;

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;
    plat_init();
    ui_init(&ui, plat_seed());
    for (;;) {
        ui_update(&ui, plat_buttons());
        plat_frame_begin(COL_BG);
        ui_draw(&ui);
        plat_frame_end();
    }
    return 0;
}
