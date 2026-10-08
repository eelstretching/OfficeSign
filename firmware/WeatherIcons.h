#ifndef WEATHERICONS_H
#define WEATHERICONS_H

#pragma once

// 16x16 weather pictograms, one per WeatherCondition (see SignMessage.h).
// Format matches include/Xpm.h: the color whose line reads "<char> c B" is
// transparent (rendered as the canvas background), everything else is a
// "<char> c #RRGGBB" line, same convention as include/Ghost.h's sprites.

static const char *sun_xpm[] = {
"16 16 3 1",
". c B",
"o c #ff9900",
"y c #ffd200",
"................",
"......oooo......",
"....o..oo..o....",
"...oo..oo..oo...",
"..oooo.oo.oooo..",
"....ooyyyyoo....",
".o...yyyyyy...o.",
".ooooyyyyyyoooo.",
".ooooyyyyyyoooo.",
".o...yyyyyy...o.",
"....ooyyyyoo....",
"..oooo.oo.oooo..",
"...oo..oo..oo...",
"....o..oo..o....",
"......oooo......",
"................"
};

static const char *cloudy_xpm[] = {
"16 16 4 1",
". c B",
"# c #000000",
"a c #808080",
"b c #ffffff",
"................",
"................",
"................",
".####aaaaaa####.",
".###abbbbbba###.",
".###abbbbbba###.",
".##aabbbbbbaa##.",
".#abbabbbbabba#.",
".abbbbbbbbbbbba.",
".abbbbbbbbbbbba.",
".abbbbbbbbbbbba.",
".#abbbbbbbbbba#.",
".##aaaaaaaaaa##.",
"................",
"................",
"................"
};

static const char *partly_cloudy_xpm[] = {
"16 16 6 1",
". c B",
"o c #ff9900",
"y c #ffd200",
"# c #000000",
"a c #808080",
"b c #ffffff",
"................",
"..........o.o.o.",
"...........ooo..",
"..........ooyoo.",
"...........ooo..",
"..........o.o.o.",
"................",
"................",
"................",
".####aa####.....",
"##abbbbbba##....",
"#abbbbbbbba#....",
"abbbbbbbbbba....",
"abbbbbbbbbba....",
"#abbbbbbbba#....",
"##aaaaaaaa##...."
};

static const char *rain_xpm[] = {
"16 16 5 1",
". c B",
"# c #000000",
"a c #808080",
"b c #ffffff",
"r c #3fa9ff",
"................",
".####aaaaaa####.",
".###abbbbbba###.",
".###abbbbbba###.",
".##aabbbbbbaa##.",
".#abbabbbbabba#.",
".abbbbbbbbbbbba.",
".abbbbbbbbbbbba.",
".abbbbbbbbbbbba.",
".#abbbbbbbbbba#.",
".##aaaaaaaaaa##.",
"................",
"..r...r...r...r.",
".r...r...r...r..",
"..r...r...r...r.",
".r...r...r...r.."
};

static const char *snow_xpm[] = {
"16 16 5 1",
". c B",
"# c #000000",
"a c #808080",
"b c #ffffff",
"s c #d6f2ff",
"................",
".####aaaaaa####.",
".###abbbbbba###.",
".###abbbbbba###.",
".##aabbbbbbaa##.",
".#abbabbbbabba#.",
".abbbbbbbbbbbba.",
".abbbbbbbbbbbba.",
".abbbbbbbbbbbba.",
".#abbbbbbbbbba#.",
".##aaaaaaaaaa##.",
"................",
"..s.....s....s..",
".....s.....s....",
"..s.....s....s..",
".....s.....s...."
};

#endif
