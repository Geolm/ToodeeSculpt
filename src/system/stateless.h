#ifndef __STATELESS_HELPERS__
#define __STATELESS_HELPERS__


#include <math.h>

 #define SL_TAU    (6.28318530f)


static inline float sl_loop(float time_in_second, float loop_in_second)
{
    time_in_second /= loop_in_second;
    return time_in_second - floorf(time_in_second);
}

static inline float sl_wave(float pos_in_loop)
{
    return .5f + .5f * cosf(pos_in_loop * SL_TAU);
}

static inline float sl_wave_base(float pos_in_loop, float base_value, float amplitude)
{
    return base_value + sl_wave(pos_in_loop) * amplitude;
}

// value in [0; 1]
static inline float sl_impulse(float value)
{
    return value * value * value * value;
}

// pseudo-random
// returns a value in [0; 1]
static inline float sl_random(float value)
{
    value = cosf(value * 91.3458f) * 47453.5453f;
    return value - floorf(value);
}

#endif