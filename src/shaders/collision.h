// ---------------------------------------------------------------------------------------------------------------------------
// Struct & Helpers 
// ---------------------------------------------------------------------------------------------------------------------------

struct aabb
{
    float2 min;
    float2 max;
};

aabb aabb_grow(aabb box, float2 amount)
{
    return (aabb) {.min = box.min - amount, .max = box.max + amount};
}

float2 aabb_get_extents(aabb box) {return box.max - box.min;}
float2 aabb_get_center(aabb box) {return (box.min + box.max) * .5f;}
aabb aabb_scale(aabb box, float scale)
{
    float2 half_extents = aabb_get_extents(box) * .5f;
    float2 center = aabb_get_center(box);

    aabb output;
    output.min = center - half_extents * scale;
    output.max = center + half_extents * scale;
    return output;
}

template <class T> T square(T value) {return value*value;}

// ---------------------------------------------------------------------------------------------------------------------------
float3 edge_init(float2 a, float2 b)
{
    float3 edge;
    edge.x = a.y - b.y;
    edge.y = b.x - a.x;
    edge.z = a.x * b.y - a.y * b.x;
    return edge;
}

// ---------------------------------------------------------------------------------------------------------------------------
float edge_distance(float3 e, float2 p)
{
    return e.x * p.x + e.y * p.y + e.z;
}

// ---------------------------------------------------------------------------------------------------------------------------
float edge_sign(float2 p, float2 e0, float2 e1)
{
    return (p.x - e1.x) * (e0.y - e1.y) - (e0.x - e1.x) * (p.y - e1.y);
}

struct obb
{
    float2 axis_i;
    float2 axis_j;
    float2 center;
    float2 extents;
};

// ---------------------------------------------------------------------------------------------------------------------------
obb compute_obb(float2 p0, float2 p1, float width)
{
    obb result;
    result.center = (p0 + p1) * .5f;
    result.axis_j = (p1 - result.center);
    result.extents.y = length(result.axis_j);
    result.axis_j /= result.extents.y;
    result.axis_i = skew(result.axis_j);
    result.extents.x = width * .5f;
    return result;
}

// ---------------------------------------------------------------------------------------------------------------------------
float2 obb_transform(obb obox, float2 point)
{
    point = point - obox.center;
    return float2(abs(dot(obox.axis_i, point)), abs(dot(obox.axis_j, point)));
}

// ---------------------------------------------------------------------------------------------------------------------------
// Intersections functions
// ---------------------------------------------------------------------------------------------------------------------------

// ---------------------------------------------------------------------------------------------------------------------------
// slab test
bool intersection_aabb_ray(aabb box, float2 origin, float2 direction)
{
    float tmin = 0.f;
    float tmax = 1e10f;
    for (int i = 0; i < 2; i++)
    {
        float inv_dir = 1.f / direction[i];
        float t1 = (box.min[i] - origin[i]) * inv_dir;
        float t2 = (box.max[i] - origin[i]) * inv_dir;

        if (t1 > t2)
        {
            float temp = t1;
            t1 = t2;
            t2 = temp;
        }

        tmin = max(tmin, t1);
        tmax = min(tmax, t2);

        if (tmin > tmax)
            return false;
    }
    return true;
}

// ---------------------------------------------------------------------------------------------------------------------------
bool intersection_aabb_disc(aabb box, float2 center, float radius)
{
    float2 nearest_point = clamp(center.xy, box.min, box.max);
    return distance_squared(nearest_point, center) < square(radius);
}

// ---------------------------------------------------------------------------------------------------------------------------
bool intersection_aabb_circle(aabb box, float2 center, float radius, float half_width)
{
    if (!intersection_aabb_disc(box, center, radius + half_width))
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

// ---------------------------------------------------------------------------------------------------------------------------
bool intersection_aabb_pie(aabb box, float2 center, float2 direction, float2 aperture, float radius)
{
    if (!intersection_aabb_disc(box, center, radius))
        return false;

    float2 aabb_vertices[4]; 
    aabb_vertices[0] = box.min;
    aabb_vertices[1] = box.max;
    aabb_vertices[2] = float2(box.min.x, box.max.y);
    aabb_vertices[3] = float2(box.max.x, box.min.y);

    for(int i=0; i<4; ++i)
    {
        float2 center_vertex = normalize(aabb_vertices[i] - center);
        if (dot(center_vertex, direction) > aperture.y)
            return true;
    }
    return intersection_aabb_ray(box, center, direction);
}

// ---------------------------------------------------------------------------------------------------------------------------
bool intersection_aabb_arc(aabb box, float2 center, float2 direction, float2 aperture, float radius, float thickness)
{
    float half_thickness = thickness * .5f;
    if (!intersection_aabb_pie(box, center, direction, aperture, radius + half_thickness))
        return false;

    return intersection_aabb_circle(box, center, radius, half_thickness);
}

// ---------------------------------------------------------------------------------------------------------------------------
bool intersection_box_unevencapsule(aabb box, float2 p0, float2 p1, float radius0, float radius1)
{
    if (intersection_aabb_disc(box, p0, radius0))
        return true;

    if (intersection_aabb_disc(box, p1, radius1))
        return true;

    float2 direction = normalize(p1 - p0);
    float2 normal = skew(direction);

    float2 v[4];
    v[0] = p0 + normal * radius0;
    v[1] = p0 - normal * radius0;
    v[2] = p1 - normal * radius1;
    v[3] = p1 + normal * radius1;

    if (v[0].x > box.max.x && v[1].x > box.max.x && v[2].x > box.max.x && v[3].x > box.max.x)
        return false;

    if (v[0].x < box.min.x && v[1].x < box.min.x && v[2].x < box.min.x && v[3].x < box.min.x)
        return false;

    if (v[0].y < box.min.y && v[1].y < box.min.y && v[2].y < box.min.y && v[3].y < box.min.y)
        return false;

    if (v[0].y > box.max.y && v[1].y > box.max.y && v[2].y > box.max.y && v[3].y > box.max.y)
        return false;

    for(uint32_t i=0; i<4; ++i)
    {
        float3 edge = edge_init(v[i], v[(i+1)%4]);
        float4 vertices_distance = float4(edge_distance(edge, box.min), edge_distance(edge, float2(box.min.x, box.max.y)),
                                          edge_distance(edge, float2(box.max.x, box.min.y)), edge_distance(edge, box.max));

        if (all(vertices_distance < 0.f))
            return false;
    }

    return true;
}

// ---------------------------------------------------------------------------------------------------------------------------
bool intersection_aabb_obb(aabb box, float2 p0, float2 p1, float radius0, float radius1)
{
    float2 dir = normalize(p1 - p0);
    float2 normal = skew(dir);
    
    float2 v[4];
    v[0] = p0 + normal * radius0;
    v[1] = p0 - normal * radius0;
    v[2] = p1 - normal * radius1;
    v[3] = p1 + normal * radius1;

    if (v[0].x > box.max.x && v[1].x > box.max.x && v[2].x > box.max.x && v[3].x > box.max.x)
        return false;

    if (v[0].x < box.min.x && v[1].x < box.min.x && v[2].x < box.min.x && v[3].x < box.min.x)
        return false;

    if (v[0].y < box.min.y && v[1].y < box.min.y && v[2].y < box.min.y && v[3].y < box.min.y)
        return false;

    if (v[0].y > box.max.y && v[1].y > box.max.y && v[2].y > box.max.y && v[3].y > box.max.y)
        return false;

    for(uint32_t i=0; i<4; ++i)
    {
        float3 edge = edge_init(v[i], v[(i+1)%4]);
        float4 vertices_distance = float4(edge_distance(edge, box.min), edge_distance(edge, float2(box.min.x, box.max.y)),
                                          edge_distance(edge, float2(box.max.x, box.min.y)), edge_distance(edge, box.max));

        if (all(vertices_distance < 0.f))
            return false;
    }

    return true;
}


// ---------------------------------------------------------------------------------------------------------------------------
bool point_in_triangle(float2 p0, float2 p1, float2 p2, float2 point)
{
    float d1, d2, d3;
    bool has_neg, has_pos;

    d1 = edge_sign(point, p0, p1);
    d2 = edge_sign(point, p1, p2);
    d3 = edge_sign(point, p2, p0);

    has_neg = (d1 < 0) || (d2 < 0) || (d3 < 0);
    has_pos = (d1 > 0) || (d2 > 0) || (d3 > 0);

    return !(has_neg && has_pos);
}

// ---------------------------------------------------------------------------------------------------------------------------
bool point_in_ellipse(float2 p0, float2 p1, float width, float2 point)
{
    obb obox = compute_obb(p0, p1, width);
    point = obb_transform(obox, point);
    float distance =  square(point.x) / square(obox.extents.x) + square(point.y) / square(obox.extents.y);

    return (distance <= 1.f);
}

// ---------------------------------------------------------------------------------------------------------------------------
bool point_in_pie(float2 center, float2 direction, float radius, float cos_aperture, float2 point)
{
    if (distance_squared(center, point) > square(radius))
        return false;

    float2 to_point = normalize(point - center);
    return dot(to_point, direction) > cos_aperture;
}

// ---------------------------------------------------------------------------------------------------------------------------
bool intersection_ellipse_circle(float2 p0, float2 p1, float width, float2 center, float radius)
{
    obb obox = compute_obb(p0, p1, width);
    center = obb_transform(obox, center);
    
    float2 transformed_center = center / obox.extents;
    float scaled_radius = radius / min(obox.extents.x, obox.extents.y);
    float squared_distance = dot(transformed_center, transformed_center);

    return (squared_distance <= square(1.f + scaled_radius));
}

// ---------------------------------------------------------------------------------------------------------------------------
bool is_aabb_inside_ellipse(float2 p0, float2 p1, float width, aabb box)
{
    float2 aabb_vertices[4]; 
    aabb_vertices[0] = box.min;
    aabb_vertices[1] = box.max;
    aabb_vertices[2] = float2(box.min.x, box.max.y);
    aabb_vertices[3] = float2(box.max.x, box.min.y);

    obb obox = compute_obb(p0, p1, width);

    // transform each vertex in ellipse space and test all are in the ellipse
    for(int i=0; i<4; ++i)
    {
        float2 vertex_ellipse_space = obb_transform(obox, aabb_vertices[i]);
        float distance =  square(vertex_ellipse_space.x) / square(obox.extents.x) + square(vertex_ellipse_space.y) / square(obox.extents.y);
        if (distance>1.f)
            return false;
    }
    return true;
}

// ---------------------------------------------------------------------------------------------------------------------------
bool is_aabb_inside_triangle(float2 p0, float2 p1, float2 p2, aabb box)
{
    float2 aabb_vertices[4]; 
    aabb_vertices[0] = box.min;
    aabb_vertices[1] = box.max;
    aabb_vertices[2] = float2(box.min.x, box.max.y);
    aabb_vertices[3] = float2(box.max.x, box.min.y);

    float3 edge0 = edge_init(p0, p1);
    float3 edge1 = edge_init(p1, p2);
    float3 edge2 = edge_init(p2, p0);

    for(int i=0; i<4; ++i)
    {
        float d0 = edge_distance(edge0, aabb_vertices[i]);
        float d1 = edge_distance(edge1, aabb_vertices[i]);
        float d2 = edge_distance(edge2, aabb_vertices[i]);

        bool has_neg = (d1 < 0) || (d2 < 0) || (d0 < 0);
        bool has_pos = (d1 > 0) || (d2 > 0) || (d0 > 0);

        if (has_neg&&has_pos)
            return false;
    }
    return true;
}

// ---------------------------------------------------------------------------------------------------------------------------
bool is_aabb_inside_obb(float2 p0, float2 p1, float width, aabb box)
{
    float2 aabb_vertices[4]; 
    aabb_vertices[0] = box.min;
    aabb_vertices[1] = box.max;
    aabb_vertices[2] = float2(box.min.x, box.max.y);
    aabb_vertices[3] = float2(box.max.x, box.min.y);

    obb obox = compute_obb(p0, p1, width);

    for(int i=0; i<4; ++i)
    {
        float2 point = obb_transform(obox, aabb_vertices[i]);
        if (any(abs(point) > obox.extents))
            return false;
    }
    return true;
}

// ---------------------------------------------------------------------------------------------------------------------------
bool is_aabb_inside_pie(float2 center, float2 direction, float2 aperture, float radius, aabb box)
{
    float2 aabb_vertices[4]; 
    aabb_vertices[0] = box.min;
    aabb_vertices[1] = box.max;
    aabb_vertices[2] = float2(box.min.x, box.max.y);
    aabb_vertices[3] = float2(box.max.x, box.min.y);

    for(int i=0; i<4; ++i)
    {
        if (!point_in_pie(center, direction, radius, aperture.y, aabb_vertices[i]))
            return false;
    }
    return true;
}