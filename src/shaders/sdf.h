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
float sd_oriented_box(float2 position, float2 a, float2 b, float height)
{
    float l = length(b-a);
    float2  d = (b-a)/l;
    float2  q = (position-(a+b)*0.5);
    q = float2x2(d.x,-d.y,d.y,d.x)*q;
    q = abs(q)-float2(l,height)*0.5;
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