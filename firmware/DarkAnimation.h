#ifndef DARKANIMATION_H
#define DARKANIMATION_H

#pragma once

#include "Animation.h"

/// @brief "Go Dark" mode: clears the canvas and just idles. There's nothing
/// to animate, so this deliberately runs at a very low frame rate.
class DarkAnimation : public Animation {
   public:
    DarkAnimation(Canvas* canvas) : Animation(canvas, nullptr) { fps = 2; }

    void init() override {
        canvas->clear();
        canvas->show();
    }

    bool step() override { return true; }
};

#endif
