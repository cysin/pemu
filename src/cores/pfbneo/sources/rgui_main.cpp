//
// RGUI main menu controller
//

#include "rgui_main.h"
#include "skeleton/pemu.h"
#include "pfbneo_config.h"
#include "pfbneo_utility.h"
#include "burner.h"
#include "rgui_turbo.h"
#include "rgui_cheats.h"

using namespace c2d;
using namespace pemu;

extern char szAppRomPath[];

RguiMain::RguiMain(UiMain *ui)
    : RectangleShape(ui->getSize()), m_ui(ui), m_renderer(ui) {
    RectangleShape::setFillColor(Color::Transparent);

    // create sub-menus
    Font *font = m_ui->getSkin()->getFont();
    m_main_menu = new RguiMenu(m_renderer, font, "pFBNeo", {});
    m_settings_menu = new RguiMenu(m_renderer, font, "Settings", {});
    m_filebrowser = new RguiFileBrowser(m_renderer, font, szAppRomPath,
                                         m_ui->getConfig()->getCoreSupportedExt());
    m_state_menu = new RguiStateMenu(m_renderer, font, m_ui);
    m_cheats_menu = new RguiCheats(m_renderer, font);
    m_turbo_menu = new RguiTurbo(m_renderer, font);

    RectangleShape::setVisibility(Visibility::Hidden);
}

RguiMain::~RguiMain() {
    delete m_main_menu;
    delete m_settings_menu;
    delete m_filebrowser;
    delete m_state_menu;
    delete m_cheats_menu;
    delete m_turbo_menu;
}

void RguiMain::buildMainMenu() {
    std::vector<RguiMenuItem> items;

    if (m_in_game) {
        items.push_back({"Resume Game", "", ID_RESUME, false});
    }

    items.push_back({"Load ROM", "", ID_LOAD_ROM, true});

    if (m_in_game) {
        items.push_back({"Save State", "", ID_SAVE_STATE, true});
        items.push_back({"Load State", "", ID_LOAD_STATE, true});
        items.push_back({"Cheats", "", ID_CHEATS, true});
    }

    items.push_back({"Turbo Fire", "", ID_TURBO, true});
    items.push_back({"Settings", "", ID_SETTINGS, true});
    items.push_back({"Quit", "", ID_QUIT, false});

    m_main_menu->setItems(items);
    m_main_menu->setTitle(m_in_game ? "pFBNeo - In Game" : "pFBNeo");
}

void RguiMain::buildSettingsMenu() {
    std::vector<RguiMenuItem> items;
    auto *cfg = m_ui->getConfig();

    items.push_back({"Show FPS", getOptionValue(PEMUConfig::OptId::EMU_SHOW_FPS),
                      PEMUConfig::OptId::EMU_SHOW_FPS, false});
    items.push_back({"Scaling", getOptionValue(PEMUConfig::OptId::EMU_SCALING),
                      PEMUConfig::OptId::EMU_SCALING, false});
    items.push_back({"Scaling Mode", getOptionValue(PEMUConfig::OptId::EMU_SCALING_MODE),
                      PEMUConfig::OptId::EMU_SCALING_MODE, false});
    items.push_back({"Filtering", getOptionValue(PEMUConfig::OptId::EMU_FILTER),
                      PEMUConfig::OptId::EMU_FILTER, false});
#ifdef __PFBA_ARM__
    items.push_back({"Frameskip", getOptionValue(PEMUConfig::OptId::EMU_FRAMESKIP),
                      PEMUConfig::OptId::EMU_FRAMESKIP, false});
#endif
#ifdef __VITA__
    items.push_back({"Wait Rendering", getOptionValue(PEMUConfig::OptId::EMU_WAIT_RENDERING),
                      PEMUConfig::OptId::EMU_WAIT_RENDERING, false});
#endif
    items.push_back({"Audio Frequency", getOptionValue(PEMUConfig::OptId::EMU_AUDIO_FREQ),
                      PEMUConfig::OptId::EMU_AUDIO_FREQ, false});
    items.push_back({"Audio Interpolation", getOptionValue(PEMUConfig::OptId::EMU_AUDIO_INTERPOLATION),
                      PEMUConfig::OptId::EMU_AUDIO_INTERPOLATION, false});
    items.push_back({"FM Interpolation", getOptionValue(PEMUConfig::OptId::EMU_AUDIO_FMINTERPOLATION),
                      PEMUConfig::OptId::EMU_AUDIO_FMINTERPOLATION, false});
    items.push_back({"Force 60Hz", getOptionValue(PEMUConfig::OptId::EMU_FORCE_60HZ),
                      PEMUConfig::OptId::EMU_FORCE_60HZ, false});
    items.push_back({"Rotation", getOptionValue(PEMUConfig::OptId::EMU_ROTATION),
                      PEMUConfig::OptId::EMU_ROTATION, false});
    items.push_back({"Neo-Geo BIOS", getOptionValue(PEMUConfig::OptId::EMU_NEOBIOS),
                      PEMUConfig::OptId::EMU_NEOBIOS, false});

    m_settings_menu->setItems(items);
}

std::string RguiMain::getOptionValue(int optId) {
    auto *opt = m_ui->getConfig()->get(optId, m_in_game);
    if (!opt) return "N/A";
    return opt->getString();
}

void RguiMain::cycleOption(int optId, int direction) {
    auto *opt = m_ui->getConfig()->get(optId, m_in_game);
    if (!opt) return;

    int idx = opt->getArrayIndex();
    int count = (int)opt->getArray().size();
    if (count <= 0) return;

    idx += direction;
    if (idx >= count) idx = 0;
    if (idx < 0) idx = count - 1;

    opt->setArrayIndex(idx);

    // apply immediate changes
    if (optId == PEMUConfig::OptId::EMU_SHOW_FPS) {
        // FPS display is checked every frame, no action needed
    } else if (optId == PEMUConfig::OptId::EMU_FILTER) {
        auto *emu = m_ui->getUiEmu();
        if (emu && emu->getVideo()) {
            emu->getVideo()->setFilter(
                (Texture::Filter)opt->getArrayIndex());
        }
    } else if (optId == PEMUConfig::OptId::EMU_SCALING ||
               optId == PEMUConfig::OptId::EMU_SCALING_MODE) {
        auto *emu = m_ui->getUiEmu();
        if (emu && emu->getVideo()) {
            emu->getVideo()->updateScaling();
        }
    }
#ifdef __VITA__
    else if (optId == PEMUConfig::OptId::EMU_WAIT_RENDERING) {
        ((PSP2Renderer *)m_ui)->setWaitRendering(opt->getInteger());
    }
#endif

    // save
    if (m_in_game) {
        m_ui->getConfig()->saveGame();
    } else {
        m_ui->getConfig()->save();
    }
}

void RguiMain::show(bool in_game) {
    m_in_game = in_game;
    m_screen = SCREEN_MAIN;
    m_visible = true;
    buildMainMenu();
    RectangleShape::setVisibility(Visibility::Visible);
    m_ui->getInput()->setRepeatDelay(INPUT_DELAY);
}

void RguiMain::hide() {
    m_visible = false;
    RectangleShape::setVisibility(Visibility::Hidden);
}

bool RguiMain::isVisible() const {
    return m_visible;
}

void RguiMain::handleMainAction(RguiMenu::Action action) {
    if (action == RguiMenu::CONFIRM) {
        int id = m_main_menu->getSelectedId();
        switch (id) {
            case ID_RESUME:
                hide();
                m_ui->getUiEmu()->resume();
                break;
            case ID_LOAD_ROM:
                m_screen = SCREEN_FILEBROWSER;
                // remember last browsed path (RetroArch-style)
                if (m_last_browse_path.empty()) {
                    m_last_browse_path = "ux0:/";
                }
                m_filebrowser->setPath(m_last_browse_path);
                break;
            case ID_SAVE_STATE:
                m_screen = SCREEN_SAVESTATE;
                m_state_menu->setMode(RguiStateMenu::SAVE);
                break;
            case ID_LOAD_STATE:
                m_screen = SCREEN_LOADSTATE;
                m_state_menu->setMode(RguiStateMenu::LOAD);
                break;
            case ID_CHEATS:
                m_screen = SCREEN_CHEATS;
                m_cheats_menu->refresh();
                break;
            case ID_TURBO:
                m_screen = SCREEN_TURBO;
                m_turbo_menu->refresh();
                break;
            case ID_SETTINGS:
                m_screen = SCREEN_SETTINGS;
                buildSettingsMenu();
                break;
            case ID_QUIT:
                if (m_in_game) {
                    m_ui->getUiEmu()->stop();
                    m_in_game = false;
                    m_screen = SCREEN_MAIN;
                    buildMainMenu();
                    m_ui->getInput()->clear();
                } else {
                    hide();
                    m_ui->done = true;
                }
                break;
        }
    } else if (action == RguiMenu::CANCEL) {
        if (m_in_game) {
            hide();
            m_ui->getUiEmu()->resume();
        }
    }
}

void RguiMain::handleSettingsAction(RguiMenu::Action action) {
    if (action == RguiMenu::CANCEL) {
        m_screen = SCREEN_MAIN;
        buildMainMenu();
        return;
    }

    if (action == RguiMenu::LEFT || action == RguiMenu::RIGHT || action == RguiMenu::CONFIRM) {
        int optId = m_settings_menu->getSelectedId();
        int dir = (action == RguiMenu::LEFT) ? -1 : 1;
        int sel = m_settings_menu->getSelectedIndex();
        cycleOption(optId, dir);
        buildSettingsMenu();
        m_settings_menu->setSelectedIndex(sel);
    }
}

bool RguiMain::onInput(Input::Player *players) {
    if (!m_visible) return false;

    Input *input = m_ui->getInput();

    switch (m_screen) {
        case SCREEN_MAIN: {
            auto action = m_main_menu->handleInput(input);
            if (action != RguiMenu::NONE) {
                handleMainAction(action);
            }
            break;
        }
        case SCREEN_SETTINGS: {
            auto action = m_settings_menu->handleInput(input);
            if (action != RguiMenu::NONE) {
                handleSettingsAction(action);
            }
            break;
        }
        case SCREEN_FILEBROWSER: {
            int result = m_filebrowser->handleInput(input);
            if (result == 0) {
                // file selected - remember directory for next time
                m_last_browse_path = m_filebrowser->getCurrentPath();
                // load ROM
                std::string path = m_filebrowser->getSelectedPath();
                if (!path.empty()) {
                    ss_api::Game game;
                    game.path = Utility::baseName(path);
                    game.name = Utility::removeExt(game.path);
                    game.romsPath = Utility::remove(path, game.path);

                    // stop current game if running
                    if (m_in_game) {
                        m_ui->getConfig()->saveGame();
                        m_ui->getUiEmu()->stop();
                    }

                    hide();
                    m_ui->getConfig()->loadGame(game);
                    m_ui->getUiEmu()->load(game);
                }
            } else if (result == 1) {
                // remember directory even on cancel
                m_last_browse_path = m_filebrowser->getCurrentPath();
                m_screen = SCREEN_MAIN;
                buildMainMenu();
            }
            break;
        }
        case SCREEN_SAVESTATE:
        case SCREEN_LOADSTATE: {
            int result = m_state_menu->handleInput(input);
            if (result == 0) {
                // action done
                hide();
                m_ui->getUiEmu()->resume();
            } else if (result == 1) {
                m_screen = SCREEN_MAIN;
                buildMainMenu();
            }
            break;
        }
        case SCREEN_CHEATS: {
            int result = m_cheats_menu->handleInput(input);
            if (result == 1) {
                if (m_in_game) {
                    RguiCheats::saveState(BurnDrvGetTextA(DRV_NAME));
                }
                m_screen = SCREEN_MAIN;
                buildMainMenu();
            }
            break;
        }
        case SCREEN_TURBO: {
            int result = m_turbo_menu->handleInput(input);
            if (result == 1) {
                if (m_in_game) {
                    RguiTurbo::save(BurnDrvGetTextA(DRV_NAME));
                }
                m_screen = SCREEN_MAIN;
                buildMainMenu();
            }
            break;
        }
    }

    return true;
}

void RguiMain::onDraw(Transform &t, bool draw) {
    if (!m_visible || !draw) return;

    switch (m_screen) {
        case SCREEN_MAIN:
            m_main_menu->onDraw(t, true);
            break;
        case SCREEN_SETTINGS:
            m_settings_menu->onDraw(t, true);
            break;
        case SCREEN_FILEBROWSER:
            m_filebrowser->draw(t);
            break;
        case SCREEN_SAVESTATE:
        case SCREEN_LOADSTATE:
            m_state_menu->draw(t);
            break;
        case SCREEN_CHEATS:
            m_cheats_menu->draw(t);
            break;
        case SCREEN_TURBO:
            m_turbo_menu->draw(t);
            break;
    }

    RectangleShape::onDraw(t, draw);
}
