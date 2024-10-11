struct aabb
{
    float2 min;
    float2 max;
};

aabb aabb_grow(aabb box, float2 amount)
{
    return (aabb) {.min = box.min - amount, .max = box.max + amount};
}

float2 skew(float2 v) {return float2(-v.y, v.x);}

template <class T> T square(T value) {return value*value;}

// ---------------------------------------------------------------------------------------------------------------------------
bool intersection_aabb_disc(aabb box, float2 center, float sq_radius)
{
    float2 nearest_point = clamp(center.xy, box.min, box.max);
    return distance_squared(nearest_point, center) < sq_radius;
}

// ---------------------------------------------------------------------------------------------------------------------------
bool intersection_aabb_circle(aabb box, float2 center, float radius, float half_width)
{
    if (!intersection_aabb_disc(box, center, square(radius + half_width)))
        return false;

    float2 candidate0 = abs(center.xy - box.min);
    float2 candidate1 = abs(center.xy - box.max);
    float2 furthest_point = max(candidate0, candidate1);

    return length_squared(furthest_point) > square(radius - half_width);
}

// ---------------------------------------------------------------------------------------------------------------------------
bool intersection_aabb_aabb(aabb box0, aabb box1)
{
    return !(box1.max.x < box0.min.x || box0.max.x < box1.min.x || box1.max.y < box0.min.y || box0.max.y < box1.min.y);
}

// ---------------------------------------------------------------------------------------------------------------------------
bool intersection_aabb_obb(aabb box, float2 p0, float2 p1, float width)
{
    float2 dir = p1 - p0;
    float2 center = (p0 + p1) * .5f;
    float height = length(dir);
    float2 axis_j = dir / height; 
    float2 axis_i = skew(axis_j);

    // generate obb vertices
    float2 v[4];
    float2 extent_i = axis_i * width;
    float2 extent_j = axis_j * height;
    v[0] = center + extent_i + extent_j;
    v[1] = center - extent_i + extent_j;
    v[2] = center + extent_i - extent_j;
    v[3] = center - extent_i - extent_j;

    // sat : obb vertices vs aabb axis
    if (v[0].x > box.max.x && v[1].x > box.max.x && v[2].x > box.max.x && v[3].x > box.max.x)
        return false;

    if (v[0].x < box.min.x && v[1].x < box.min.x && v[2].x < box.min.x && v[3].x < box.min.x)
        return false;

    if (v[0].y < box.min.y && v[1].y < box.min.y && v[2].y < box.min.y && v[3].y < box.min.y)
        return false;

    if (v[0].y > box.max.y && v[1].y > box.max.y && v[2].y > box.max.y && v[3].y > box.max.y)
        return false;

    // generate aabb vertices
    v[0] = box.min;
    v[1] = float2(box.min.x, box.max.y);
    v[2] = float2(box.max.x, box.min.y);
    v[3] = box.max;

    // sat : aabb vertices vs obb axis
    float4 distances = float4(dot(axis_i, v[0]), dot(axis_i, v[1]), dot(axis_i, v[2]), dot(axis_i, v[3]));
    distances -= dot(center, axis_i);

    float threshold = width * .5f;
    if (all(distances > threshold) || all(distances < -threshold))
        return false;
    
    distances = float4(dot(axis_j, v[0]), dot(axis_j, v[1]), dot(axis_j, v[2]), dot(axis_j, v[3]));
    distances -= dot(center, axis_j);

    threshold = height * .5f;
    if (all(distances > threshold) || all(distances < -threshold))
        return false;

    return true;
}

// ---------------------------------------------------------------------------------------------------------------------------
float3 edge_init(float2 a, float2 b)
{
    float3 edge;
    edge.x = a.y - b.y;
    edge.y = b.x - a.x;
    edge.z = a.x * b.y - a.y * b.x;
    return edge;
}

float edge_distance(float3 e, float2 p)
{
    return e.x * p.x + e.y * p.y + e.z;
}

// ---------------------------------------------------------------------------------------------------------------------------
bool intersection_aabb_triangle(aabb box, float2 p0, float2 p1, float2 p2)
{
    // first axis : aabb's axis
    if (p0.x < box.min.x && p1.x < box.min.x && p2.x < box.min.x)
        return false;

    if (p0.x > box.max.x && p1.x > box.max.x && p2.x > box.max.x)
        return false;
    
    if (p0.y < box.min.y && p1.y < box.min.y && p2.y < box.min.y)
        return false;

    if (p0.y > box.max.y && p1.y > box.max.y && p2.y > box.max.y)
        return false;
    
    float2 v[4];
    v[0] = box.min;
    v[1] = box.max;
    v[2] = (float2) {box.min.x, box.max.y};
    v[3] = (float2) {box.max.x, box.min.y};

    // we can't assume any winding order for the triangle, so we check distance to the aabb's vertices sign against 
    // distance to the other vertex of the triangle sign. If all aabb's vertices have a opposite sign, it's a separate axis.
    float3 e = edge_init(p0, p1);
    float4 vertices_distance = float4(edge_distance(e, v[0]), edge_distance(e, v[1]), edge_distance(e, v[2]), edge_distance(e, v[3]));
    if (all(sign(vertices_distance) != sign(edge_distance(e, p2))))
        return false;

    e = edge_init(p1, p2);
    vertices_distance = float4(edge_distance(e, v[0]), edge_distance(e, v[1]), edge_distance(e, v[2]), edge_distance(e, v[3]));
    if (all(sign(vertices_distance) != sign(edge_distance(e, p0))))
        return false;

    e = edge_init(p2, p0);
    vertices_distance = float4(edge_distance(e, v[0]), edge_distance(e, v[1]), edge_distance(e, v[2]), edge_distance(e, v[3]));
    if (all(sign(vertices_distance) != sign(edge_distance(e, p1))))
        return false;

    return true;

}