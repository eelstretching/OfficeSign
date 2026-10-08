#include "ClockWeatherAnimation.h"

#include "WeatherIcons.h"
#include "pico/aon_timer.h"
#include "pico/printf.h"
#include "pico/time.h"

#define S_IN_US 1000000ull

ClockWeatherAnimation::ClockWeatherAnimation(Canvas* canvas, Font* font)
    : Animation(canvas, nullptr),
      font(font),
      sunIcon(sun_xpm),
      cloudyIcon(cloudy_xpm),
      partlyCloudyIcon(partly_cloudy_xpm),
      rainIcon(rain_xpm),
      snowIcon(snow_xpm) {}

void ClockWeatherAnimation::init() {
    lastRenderUs = time_us_64();
    render();
}

bool ClockWeatherAnimation::step() {
    uint64_t now = time_us_64();
    if (now - lastRenderUs >= S_IN_US) {
        lastRenderUs = now;
        render();
    }
    return true;
}

void ClockWeatherAnimation::setWeather(const WeatherInfo& info) {
    weather = info;
    render();
}

void ClockWeatherAnimation::setShowWeather(bool show) {
    showWeather = show;
    render();
}

Xpm* ClockWeatherAnimation::iconFor(WeatherCondition condition) {
    switch (condition) {
        case WeatherCondition::Sunny:
            return &sunIcon;
        case WeatherCondition::Cloudy:
            return &cloudyIcon;
        case WeatherCondition::PartlyCloudy:
            return &partlyCloudyIcon;
        case WeatherCondition::Rain:
            return &rainIcon;
        case WeatherCondition::Snow:
            return &snowIcon;
    }
    return &sunIcon;
}

// Layout, planned for the eventual 128x32 sign: a weather icon in a fixed
// column on the left (vertically centered, blank when there's no weather to
// show), with the clock and weather text stacked to its right so the text's
// left edge doesn't jump around as weather comes and goes.
void ClockWeatherAnimation::render() {
    struct tm tm = {};
    aon_timer_get_time_calendar(&tm);
    snprintf(timeBuf, sizeof(timeBuf), "%02d/%02d  %02d:%02d:%02d", tm.tm_mon + 1, tm.tm_mday,
             tm.tm_hour, tm.tm_min, tm.tm_sec);

    canvas->clear();

    constexpr int kIconSize = 16;
    constexpr int kIconMargin = 2;
    constexpr int kTextGap = 4;
    constexpr int kLineGap = 2;
    constexpr int kTopMargin = 2;

    int iconX = kIconMargin;
    int iconY = (static_cast<int>(canvas->getHeight()) - kIconSize) / 2;
    int textX = kIconMargin + kIconSize + kTextGap;

    uint lineHeight = font->getBoundingBox(timeBuf).second;
    int timeY = static_cast<int>(canvas->getHeight()) - kTopMargin - static_cast<int>(lineHeight);
    int weatherY = timeY - static_cast<int>(lineHeight) - kLineGap;

    font->render(canvas, timeBuf, textX, timeY, RGB::White);

    if (showWeather && weather.valid) {
        iconFor(weather.condition)->render(canvas, iconX, iconY);

        snprintf(weatherBuf, sizeof(weatherBuf), "%dF (H%d L%d)", weather.currentTempF,
                 weather.highTempF, weather.lowTempF);
        font->render(canvas, weatherBuf, textX, weatherY, RGB::Cyan);
    }
}
