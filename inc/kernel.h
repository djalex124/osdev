#pragma once

#define AQUA_VER_MAJOR 0
#define AQUA_VER_MINOR 0
#define AQUA_VER_REV   0
#define AQUA_VER_STR   alpha

#define string2(s) #s
#define string1(s) string2(s)

#ifdef AQUA_DEBUG
#define AQUA_VER_STRING string1(AQUA_VER_MAJOR) "." string1(AQUA_VER_MINOR) "." string1(AQUA_VER_REV) "-" string1(AQUA_VER_STR) "+" string1(AQUA_VER_BUILD) ".debug"
#else
#define AQUA_VER_STRING string1(AQUA_VER_MAJOR) "." string1(AQUA_VER_MINOR) "." string1(AQUA_VER_REV) "-" string1(AQUA_VER_STR) "+" string1(AQUA_VER_BUILD)
#endif