//
// Created by cpasjuste on 19/09/23.
//

#include "main.h"

#ifdef __PFBA__
#include "rgui_main.h"
#endif

using namespace c2d;
using namespace pemu;

PEMUUiMain *pemu_ui;

#ifdef __PFBA__
RguiMain *g_rgui = nullptr;
#endif

int main(int argc, char **argv) {
    // command line game info
    Game game;

    // custom io
    const auto io = new PEMUIo();

    // create main ui/renderer
    pemu_ui = new PEMUUiMain(Vector2f{0, 0});
    pemu_ui->setIo(io);

    // load configuration
    constexpr int version = (__PEMU_VERSION_MAJOR__ * 100) + __PEMU_VERSION_MINOR__;
    const auto cfg = new PEMUConfig(pemu_ui, version);
    pemu_ui->setConfig(cfg);

    // load skin configuration
    const auto skin = new PEMUSkin(pemu_ui);
    pemu_ui->setSkin(skin);

    // parse command line
    if (argc > 1) {
        if (io->exist(argv[1])) {
            game.path = Utility::baseName(argv[1]);
            game.name = Utility::removeExt(game.path);
            game.romsPath = Utility::remove(argv[1], game.path);
        } else {
            printf("main: file provided as console argument does not exist (%s)\n", argv[1]);
            delete (skin);
            delete (cfg);
            delete (pemu_ui);
            return 1;
        }
    }

    // ui - still need the old components for UiMain::init to work
    const auto romList = new PEMURomList(pemu_ui, cfg->getCoreVersion(), cfg->getCoreSupportedExt());
    if (game.path.empty()) {
        romList->build();
        romList->initFav();
    } else {
        delete (romList->rect);
    }
    const auto uiRomList = new PEMUUiRomList(pemu_ui, romList, pemu_ui->getSize());
    const auto uiMenu = new PEMUUiMenu(pemu_ui);
    const auto uiEmu = new PEMUUiEmu(pemu_ui);
    const auto uiState = new PEMUUiMenuState(pemu_ui);
    pemu_ui->init(uiRomList, uiMenu, uiEmu, uiState);

#ifdef __PFBA__
    // create RGUI menu overlay and add to renderer
    g_rgui = new RguiMain(pemu_ui);
    g_rgui->setLayer(100);
    pemu_ui->add(g_rgui);

    // hide old rom list UI
    uiRomList->setVisibility(Visibility::Hidden);
    if (uiRomList->getBlur()) {
        uiRomList->getBlur()->setVisibility(Visibility::Hidden);
    }
#endif

    // load specified game from command line if requested
    if (!game.path.empty()) {
        uiRomList->setVisibility(Visibility::Hidden);
        uiRomList->setGames({game});
        cfg->loadGame(game);
        uiEmu->setExitOnStop(true);
        uiEmu->load(game);
    }
#ifdef __PFBA__
    else {
        // show RGUI main menu on startup
        g_rgui->show(false);
    }
#endif

    while (!pemu_ui->done) {
        pemu_ui->flip();
    }

#ifdef __PFBA__
    delete g_rgui;
    g_rgui = nullptr;
#endif

    delete (skin);
    delete (cfg);
    delete (pemu_ui);

#ifdef  __PS4__
    sceSystemServiceLoadExec((char *) "exit", nullptr);
    while (true) {}
#endif

    return 0;
}
