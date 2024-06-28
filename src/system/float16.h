#ifndef __FLOAT16__H__
#define __FLOAT16__H__

#include <stdint.h>
#include <arm_neon.h>

typedef uint16_t float16;

static inline float16 float_to_half(float f)
{
    return vget_lane_u16(vcvt_f16_f32(vdupq_n_f32(f)), 0);
}

static inline float half_to_float(float16 h)
{
    return vgetq_lane_f32(vcvt_f32_f16(vreinterpret_f16_u16(vdup_n_u16(h))), 0);
}


#endif // 