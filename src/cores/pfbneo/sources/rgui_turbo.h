//
// RGUI turbo fire settings
//

#ifndef PFBNEO_RGUI_TURBO_H
#define PFBNEO_RGUI_TURBO_H

#include "rgui_menu.h"
#include "cross2d/c2d.h"

#define TURBO_MAX_PLAYERS 2
#define TURBO_MAX_BUTTONS 8

struct TurboConfig {
    bool enabled[TURBO_MAX_PLAYERS][TURBO_MAX_BUTTONS]; // A,B,C,D,X,Y,Z,L
    int speed; // toggle every N frames (default 2)
};

extern TurboConfig g_turbo;
extern int g_turbo_frame_counter;

class RguiTurbo {
public:
    RguiTurbo(c2d::Renderer *renderer, c2d::Font *font);
    ~RguiTurbo();

    void refresh();

    // returns: 0=done, 1=cancelled, -1=navigating
    int handleInput(c2d::Input *input);

    void draw(c2d::Transform &t);

    static void save(const char *driverName);
    static void load(const char *driverName);

    static const char *getButtonName(int btn);

private:
    c2d::Renderer *m_renderer;
    RguiMenu *m_menu;
};

#endif // PFBNEO_RGUI_TURBO_H
