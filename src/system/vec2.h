#ifndef __VEC2__
#define __VEC2__

#include <math.h>
#include <stdbool.h>

//-----------------------------------------------------------------------------
 #define VEC2_PI     (3.14159265f)
 #define VEC2_PI_2   (1.57079632f)
 #define VEC2_PI_4   (0.78539816f)
 #define VEC2_TAU    (6.28318530f)
 #define VEC2_SQR2   (1.41421356237f)

//-----------------------------------------------------------------------------
static inline float float_sign(float f) {if (f>0.f) return 1.f; if (f<0.f) return -1.f; return 0.f;}
static inline float float_clamp(float f, float a, float b) {if (f<a) return a; if (f>b) return b; return f;}
static inline float float_square(float f) {return f*f;}
static inline float float_min(float a, float b) {return (a<b) ? a : b;}
static inline float float_max(float a, float b) {return (a>b) ? a : b;}
static inline float float_lerp(float a, float b, float t) {return fmaf(a , (1.f - t), (b * t));}

typedef struct {float x, y;} vec2;

//-----------------------------------------------------------------------------
static inline vec2 vec2_splat(float value) {return (vec2) {value, value};}
static inline vec2 vec2_set(float x, float y) {return (vec2) {x, y};}
static inline vec2 vec2_zero(void) {return (vec2) {0.f, 0.f};}
static inline vec2 vec2_one(void) {return (vec2) {1.f, 1.f};}
static inline vec2 vec2_angle(float angle) {return (vec2) {cosf(angle), sinf(angle)};}
static inline vec2 vec2_add(vec2 a, vec2 b) {return (vec2) {a.x + b.x, a.y + b.y};}
static inline vec2 vec2_sub(vec2 a, vec2 b) {return (vec2) {a.x - b.x, a.y - b.y};}
static inline vec2 vec2_scale(vec2 a, float f) {return (vec2) {a.x * f, a.y * f};}
static inline vec2 vec2_skew(vec2 v) {return (vec2) {-v.y, v.x};}
static inline vec2 vec2_mul(vec2 a, vec2 b) {return (vec2){a.x * b.x, a.y * b.y};}
static inline vec2 vec2_div(vec2 a, vec2 b) {return (vec2){a.x / b.x, a.y / b.y};}
static inline float vec2_dot(vec2 a, vec2 b) {return fmaf(a.x, b.x, a.y * b.y);}
static inline float vec2_sq_length(vec2 v) {return vec2_dot(v, v);}
static inline float vec2_length(vec2 v) {return sqrtf(vec2_sq_length(v));}
static inline float vec2_sq_distance(vec2 a, vec2 b) {return vec2_sq_length(vec2_sub(b, a));}
static inline float vec2_distance(vec2 a, vec2 b) {return vec2_length(vec2_sub(b, a));}
static inline vec2 vec2_reflect(vec2 v, vec2 normal) {return vec2_sub(v, vec2_scale(normal, vec2_dot(v, normal) * 2.f));}
static inline vec2 vec2_min(vec2 v, vec2 op) {return (vec2) {.x = (v.x < op.x) ? v.x : op.x, .y = (v.y < op.y) ? v.y : op.y};}
static inline vec2 vec2_min3(vec2 a, vec2 b, vec2 c) {return vec2_min(a, vec2_min(b, c));}
static inline vec2 vec2_min4(vec2 a, vec2 b, vec2 c, vec2 d) {return vec2_min(a, vec2_min3(b, c, d));}
static inline vec2 vec2_max(vec2 v, vec2 op) {return (vec2) {.x = (v.x > op.x) ? v.x : op.x, .y = (v.y > op.y) ? v.y : op.y};}
static inline vec2 vec2_max3(vec2 a, vec2 b, vec2 c) {return vec2_max(a, vec2_max(b, c));}
static inline vec2 vec2_max4(vec2 a, vec2 b, vec2 c, vec2 d) {return vec2_max(a, vec2_max3(b, c, d));}
static inline vec2 vec2_clamp(vec2 v, vec2 lower_bound, vec2 higher_bound) {return vec2_min(vec2_max(v, lower_bound), higher_bound);}
static inline vec2 vec2_saturate(vec2 v) {return vec2_clamp(v, vec2_splat(0.f), vec2_splat(1.f));}
static inline bool vec2_equal(vec2 a, vec2 b) {return (a.x == b.x && a.y == b.y);}
static inline bool vec2_similar(vec2 a, vec2 b, float epsilon) {return (fabsf(a.x - b.x) < epsilon) && (fabsf(a.y - b.y) < epsilon);}
static inline vec2 vec2_neg(vec2 a) {return  (vec2){-a.x, -a.y};}
static inline vec2 vec2_abs(vec2 a) {return (vec2) {.x = fabsf(a.x), .y = fabsf(a.y)};}
static inline bool vec2_all_less(vec2 a, vec2 b) {return (a.x < b.x && a.y < b.y);}
static inline bool vec2_any_less(vec2 a, vec2 b) {return (a.x < b.x || a.y < b.y);}
static inline bool vec2_all_greater(vec2 a, vec2 b) {return (a.x > b.x && a.y > b.y);}
static inline bool vec2_any_greater(vec2 a, vec2 b) {return (a.x > b.x || a.y > b.y);}
static inline float vec2_cross(vec2 a, vec2 b) {return fmaf(a.x, b.y, - a.y * b.x);}
static inline vec2 vec2_sign(vec2 a) {return (vec2) {float_sign(a.x), float_sign(a.y)};}
static inline vec2 vec2_pow(vec2 a, vec2 b) {return (vec2) {powf(a.x, b.x), powf(a.y, b.y)};}
static inline float vec2_atan2(vec2 v) {return atan2f(v.y, v.x);}
static inline void vec2_swap(vec2* a, vec2* b) {vec2 tmp = *a; *a = *b; *b = tmp;}
static inline vec2 vec2_rotate(vec2 point, vec2 rotation) {return (vec2) {fmaf(point.x, rotation.x, -point.y * rotation.y), fmaf(point.y, rotation.x, point.x * rotation.y)};}
static inline vec2 vec2_floor(vec2 a) {return (vec2) {floorf(a.x), floorf(a.y)};}
static inline vec2 vec2_lerp(vec2 a, vec2 b, float t) 
{
    float one_minus_t = 1.f - t;
    return (vec2) {.x = fmaf(a.x , one_minus_t, b.x * t), .y = fmaf(a.y , one_minus_t, b.y * t)};
}
static inline vec2 vec2_quadratic_bezier(vec2 p0, vec2 p1, vec2 p2, float t) 
{
    float omt = 1.f-t;
    float c0 = omt * omt;
    float c1 = 2.f * omt * t;
    float c2 = t * t;
    return (vec2)
    {
        .x = fmaf(p0.x, c0, fmaf(p1.x, c1, p2.x * c2)),
        .y = fmaf(p0.y, c0, fmaf(p1.y, c1, p2.y * c2))
    };
}

static inline vec2 vec2_quadratic_bezier_tangent(vec2 p0, vec2 p1, vec2 p2, float t)
{
    float omt = 1.f-t;
    return vec2_add(vec2_scale(vec2_sub(p1, p0), omt * 2.f), vec2_scale(vec2_sub(p2, p1), 2.f * t));
}

static inline float vec2_normalize(vec2* v)
{
    float norm = vec2_length(*v);
    if (norm == 0.f)
        return 0.f;

    *v = vec2_scale(*v, 1.f / norm);
    return norm;
}

static inline vec2 vec2_normalized(vec2 v)
{
    return vec2_scale(v, 1.f / vec2_length(v));
}

static inline float vec2_relative_epsilon(vec2 a, float epsilon)
{
    return float_max(a.x, a.y) * epsilon;
}

static inline float vec2_signed_area(vec2 a, vec2 b, vec2 c)
{
    return vec2_cross(vec2_sub(b, a), vec2_sub(c, b));
}

static inline bool vec2_colinear(vec2 a, vec2 b, vec2 c, float epsilon)
{
    return fabsf(vec2_signed_area(a, b, c)) < vec2_relative_epsilon(vec2_max3(a, b, c), epsilon);
}

#ifdef __cplusplus

static inline void operator+= (vec2& a, vec2 b) {a = vec2_add(a, b);}
static inline void operator-= (vec2& a, vec2 b) {a = vec2_sub(a, b);}
static inline void operator*= (vec2& a, vec2 b) {a = vec2_mul(a, b);}
static inline void operator*= (vec2& a, float scale) {a = vec2_scale(a, scale);}
static inline vec2 operator+ (vec2 a, vec2 b) {return vec2_add(a, b);}
static inline vec2 operator- (vec2 a, vec2 b) {return vec2_sub(a, b);}
static inline vec2 operator* (vec2 a, vec2 b) {return vec2_mul(a, b);}
static inline vec2 operator* (vec2 a, float scale) {return vec2_scale(a, scale);}
static inline bool operator!= (vec2 a, vec2 b) {return !vec2_equal(a, b);}

#endif // __cplusplus

#endif 
