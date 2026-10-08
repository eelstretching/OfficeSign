#ifndef CLOCKWEATHERANIMATION_H
#define CLOCKWEATHERANIMATION_H

#pragma once

#include "Animation.h"
#include "SignMessage.h"
#include "Font.h"
#include "Xpm.h"

/// @brief Info cached from the most recent WEATHER message.
struct WeatherInfo {
    WeatherCondition condition = WeatherCondition::Sunny;
    int16_t currentTempF = 0;
    int16_t highTempF = 0;
    int16_t lowTempF = 0;
    bool valid = false;
};

/// @brief The sign's default "Clock Mode" display: current date/time (from
/// the RTC, as set by the most recent SET_TIME message), optionally with the
/// most recently received weather overlaid. Evolution of the CampSign
/// example's TimeAnimation/DataAnimation, updated for the current SimpleFont
/// API. Persists forever once started -- step() always returns true, so the
/// wrapping Animator never rotates it out on its own.
class ClockWeatherAnimation : public Animation {
   public:
    ClockWeatherAnimation(Canvas* canvas, Font* font);

    void init() override;
    bool step() override;

    void setWeather(const WeatherInfo& info);
    void setShowWeather(bool show);

   private:
    Font* font;
    WeatherInfo weather;
    bool showWeather = true;
    uint64_t lastRenderUs = 0;

    char timeBuf[24];
    char weatherBuf[24];

    // One Xpm per WeatherCondition, parsed once up front (see
    // WeatherIcons.h) rather than re-parsed every render() call.
    Xpm sunIcon;
    Xpm cloudyIcon;
    Xpm partlyCloudyIcon;
    Xpm rainIcon;
    Xpm snowIcon;

    void render();
    Xpm* iconFor(WeatherCondition condition);
};

#endif
