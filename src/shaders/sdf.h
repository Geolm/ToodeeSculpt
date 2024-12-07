float2 skew(float2 v) {return float2(-v.y, v.x);}

//-----------------------------------------------------------------------------
float sd_disc(float2 position, float2 center, float radius)
{
    return length(center-position) - radius;
}

//-----------------------------------------------------------------------------
float sd_segment(float2 position, float2 a, float2 b)
{
    float2 pa = position-a, ba = b-a;
    float h = clamp( dot(pa,ba)/dot(ba,ba), 0.f, 1.f);
    return length(pa - ba*h);
}

//-----------------------------------------------------------------------------
float sd_oriented_box(float2 position, float2 a, float2 b, float width)
{
    float l = length(b-a);
    float2  d = (b-a)/l;
    float2  q = (position-(a+b)*0.5);
    q = float2x2(d.x,-d.y,d.y,d.x)*q;
    q = abs(q)-float2(l,width)*0.5;
    return length(max(q,0.0)) + min(max(q.x,q.y),0.0);
}

//-----------------------------------------------------------------------------
float sd_triangle(float2 p, float2 p0, float2 p1, float2 p2 )
{
    float2 e0 = p1 - p0;
    float2 e1 = p2 - p1;
    float2 e2 = p0 - p2;

    float2 v0 = p - p0;
    float2 v1 = p - p1;
    float2 v2 = p - p2;

    float2 pq0 = v0 - e0*saturate(dot(v0,e0)/dot(e0,e0));
    float2 pq1 = v1 - e1*saturate(dot(v1,e1)/dot(e1,e1));
    float2 pq2 = v2 - e2*saturate(dot(v2,e2)/dot(e2,e2));
    
    float s = e0.x*e2.y - e0.y*e2.x;
    float2 d = min(min(float2(dot(pq0, pq0 ), s*(v0.x*e0.y-v0.y*e0.x)),
                       float2(dot(pq1, pq1 ), s*(v1.x*e1.y-v1.y*e1.x))),
                       float2(dot(pq2, pq2 ), s*(v2.x*e2.y-v2.y*e2.x)));

    return -sqrt(d.x)*sign(d.y);
}

//-----------------------------------------------------------------------------
// based on https://www.shadertoy.com/view/tt3yz7
float sd_ellipse(float2 p, float2 e)
{
    float2 pAbs = abs(p);
    float2 ei = 1.f / e;
    float2 e2 = e*e;
    float2 ve = ei * float2(e2.x - e2.y, e2.y - e2.x);
    
    float2 t = float2(0.70710678118654752f, 0.70710678118654752f);

    // hopefully unroll by the compiler
    for (int i = 0; i < 3; i++) 
    {
        float2 v = ve*t*t*t;
        float2 u = normalize(pAbs - v) * length(t * e - v);
        float2 w = ei * (v + u);
        t = normalize(saturate(w));
    }
    
    float2 nearestAbs = t * e;
    float dist = length(pAbs - nearestAbs);
    return dot(pAbs, pAbs) < dot(nearestAbs, nearestAbs) ? -dist : dist;
}

//-----------------------------------------------------------------------------
float sd_oriented_ellipse(float2 position, float2 a, float2 b, float width)
{
    float height = length(b-a);
    float2  axis = (b-a)/height;
    float2  position_translated = (position-(a+b)*.5f);
    float2 position_boxspace = float2x2(axis.x,-axis.y, axis.y, axis.x)*position_translated;
    return sd_ellipse(position_boxspace, float2(height * .5f, width * .5f));
}

//-----------------------------------------------------------------------------
float sd_oriented_pie(float2 position, float2 center, float2 direction, float2 aperture, float radius)
{
    direction = -skew(direction);
    position -= center;
    position = float2x2(direction.x,-direction.y, direction.y, direction.x) * position;
    position.x = abs(position.x);
    float l = length(position) - radius;
	float m = length(position - aperture*clamp(dot(position,aperture),0.f,radius));
    return max(l,m*sign(aperture.y*position.x - aperture.x*position.y));
}