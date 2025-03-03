// This shader has been automatically generated from ToodeeSculpt (https://github.com/Geolm/ToodeeSculpt)
// MIT License

// Copyright (c) 2024 Geolm

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.



//----------------------------------------------------------------------------------------------------------------------------------------------------------
// Signed Distance Field functions
//----------------------------------------------------------------------------------------------------------------------------------------------------------


vec2 skew(vec2 v) {return vec2(-v.y, v.x);}
float cross2(vec2 a, vec2 b ) {return a.x*b.y - a.y*b.x;}

//-----------------------------------------------------------------------------
float sd_disc(vec2 position, vec2 center, float radius)
{
    return length(center-position) - radius;
}

//-----------------------------------------------------------------------------
float sd_segment(vec2 position, vec2 a, vec2 b)
{
    vec2 pa = position-a, ba = b-a;
    float h = clamp( dot(pa,ba)/dot(ba,ba), 0.0, 1.0);
    return length(pa - ba*h);
}

//-----------------------------------------------------------------------------
float sd_oriented_box(vec2 position, vec2 a, vec2 b, float width)
{
    float l = length(b-a);
    vec2  d = (b-a)/l;
    vec2  q = (position-(a+b)*0.5);
    q = mat2(d.x,-d.y,d.y,d.x)*q;
    q = abs(q)-vec2(l,width)*0.5;
    return length(max(q,0.0)) + min(max(q.x,q.y),0.0);
}

//-----------------------------------------------------------------------------
float sd_triangle(vec2 p, vec2 p0, vec2 p1, vec2 p2 )
{
    vec2 e0 = p1 - p0;
    vec2 e1 = p2 - p1;
    vec2 e2 = p0 - p2;

    vec2 v0 = p - p0;
    vec2 v1 = p - p1;
    vec2 v2 = p - p2;

    vec2 pq0 = v0 - e0*clamp(dot(v0,e0)/dot(e0,e0), 0.0, 1.0);
    vec2 pq1 = v1 - e1*clamp(dot(v1,e1)/dot(e1,e1), 0.0, 1.0);
    vec2 pq2 = v2 - e2*clamp(dot(v2,e2)/dot(e2,e2), 0.0, 1.0);
    
    float s = e0.x*e2.y - e0.y*e2.x;
    vec2 d = min(min(vec2(dot(pq0, pq0 ), s*(v0.x*e0.y-v0.y*e0.x)),
                       vec2(dot(pq1, pq1 ), s*(v1.x*e1.y-v1.y*e1.x))),
                       vec2(dot(pq2, pq2 ), s*(v2.x*e2.y-v2.y*e2.x)));

    return -sqrt(d.x)*sign(d.y);
}

//-----------------------------------------------------------------------------
// based on https://www.shadertoy.com/view/tt3yz7
float sd_ellipse(vec2 p, vec2 e)
{
    vec2 pAbs = abs(p);
    vec2 ei = 1.f / e;
    vec2 e2 = e*e;
    vec2 ve = ei * vec2(e2.x - e2.y, e2.y - e2.x);
    
    vec2 t = vec2(0.70710678118654752, 0.70710678118654752);

    // hopefully unroll by the compiler
    for (int i = 0; i < 3; i++) 
    {
        vec2 v = ve*t*t*t;
        vec2 u = normalize(pAbs - v) * length(t * e - v);
        vec2 w = ei * (v + u);
        t = normalize(clamp(w, vec2(0.0), vec2(1.0)));
    }
    
    vec2 nearestAbs = t * e;
    float dist = length(pAbs - nearestAbs);
    return dot(pAbs, pAbs) < dot(nearestAbs, nearestAbs) ? -dist : dist;
}

//-----------------------------------------------------------------------------
float sd_oriented_ellipse(vec2 position, vec2 a, vec2 b, float width)
{
    float height = length(b-a);
    vec2  axis = (b-a)/height;
    vec2  position_translated = (position-(a+b)* 0.5);
    vec2 position_boxspace = mat2(axis.x,-axis.y, axis.y, axis.x)*position_translated;
    return sd_ellipse(position_boxspace, vec2(height * 0.5, width * 0.5));
}

//-----------------------------------------------------------------------------
float sd_oriented_pie(vec2 position, vec2 center, vec2 direction, vec2 aperture, float radius)
{
    direction = -skew(direction);
    position -= center;
    position = mat2(direction.x,-direction.y, direction.y, direction.x) * position;
    position.x = abs(position.x);
    float l = length(position) - radius;
	float m = length(position - aperture*clamp(dot(position,aperture),0.0,radius));
    return max(l,m*sign(aperture.y*position.x - aperture.x*position.y));
}

//-----------------------------------------------------------------------------
float sd_oriented_ring(vec2 position, vec2 center, vec2 direction, vec2 aperture, float radius, float thickness)
{
    direction = -skew(direction);
    position -= center;
    position = mat2(direction.x,-direction.y, direction.y, direction.x) * position;
    position.x = abs(position.x);
    position = mat2(aperture.y,aperture.x,-aperture.x,aperture.y)*position;
    return max(abs(length(position)-radius)-thickness*0.5,length(vec2(position.x,max(0.0,abs(radius-position.y)-thickness*0.5)))*sign(position.x) );
}

//-----------------------------------------------------------------------------
float sd_uneven_capsule(vec2 p, vec2 pa, vec2 pb, float ra, float rb)
{
    p  -= pa;
    pb -= pa;
    float h = dot(pb,pb);
    vec2  q = vec2( dot(p,vec2(pb.y,-pb.x)), dot(p,pb) )/h;
    
    q.x = abs(q.x);
    float b = ra-rb;
    vec2  c = vec2(sqrt(h-b*b),b);
    
    float k = cross2(c,q);
    float m = dot(c,q);
    float n = dot(q,q);

         if( k < 0.0 ) return sqrt(h*(n            )) - ra;
    else if( k > c.x ) return sqrt(h*(n+1.0-2.0*q.y)) - rb;
                       return m                       - ra;
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------
// SDF Operator
//----------------------------------------------------------------------------------------------------------------------------------------------------------

vec2 smooth_minimum(float a, float b, float k)
{
    b = max(b, 0.f);    // a is always on top
    if (k>0.f)
    {
        float h = max( k-abs(a-b), 0.0 )/k;
        float m = h*h*h*0.5;
        float s = m*k*(1.0/3.0);
        return (a<b) ? vec2(a-s, 0.f) : vec2(b-s, 1.f - smoothstep(-k, 0.f, b-a));
    }
    else
    {
        // "hard" min
        return vec2(min(a, b),  (a<b) ? 0.0 : 1.0);
    }
}

// ---------------------------------------------------------------------------------------------------------------------------
float smooth_substraction(float d1, float d2, float k)
{
    if (k>0.f)
    {
        float h = clamp(0.5 - 0.5*(d2+d1)/k, 0.0, 1.0);
        return mix(d1, -d2, h) + k*h*(1.0-h);
    }
    else
    {
        return max(-d2, d1);
    }
}

// ---------------------------------------------------------------------------------------------------------------------------
float smooth_intersection(float d1, float d2, float k )
{
    if (k>0.f)
    {
        float h = clamp(0.5 - 0.5*(d2-d1)/k, 0.0, 1.0);
        return mix(d2, d1, h) + k*h*(1.0-h);
    }
    else
    {
        return max(d1, d2);
    }
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------
// SDF world
//----------------------------------------------------------------------------------------------------------------------------------------------------------
vec4 map(vec2 p)
{
    float d = 100000.0;
    vec2 blend = vec2(0.0);
    vec3 color = vec3(0.0);
    float pixel_size = length(dFdx(p) + dFdy(p));
