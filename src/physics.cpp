
void distribute(vec3* points, int count)
{
    float offset = 2.0 / count;
    float increment = M_PI * (3.0 - sqrt(5.0));
    for (int i = 0; i < count; i++)
    {
        float y = ((i * offset) - 1) + (offset / 2);
        float r = sqrt(1 - pow(y, 2));
        float phi = fmod(i + 1.0, count) * increment;
        float x = cos(phi) * r;
        float z = sin(phi) * r;
        points[i] = { x, y, z };
    }
}

void add_thing(Thing thing)
{
    for (auto& thing : things)
        for (auto& line : thing.lines)
            line.points.resize(1);

    int line_count = (int) ceil(fabs(thing.q * 8.0));
    if (line_count < 8) line_count = 8;
    if (line_count > 40) line_count = 40;

    vec3 points[line_count];
    distribute(points, line_count);
    for (int i = 0; i < line_count; i++)
    {
        Line line;
        line.points.push_back(thing.p + points[i] * 0.1f);
        thing.lines.push_back(line);
    }

    things.push_back(thing);
}

vec3 get_force_in_point(vec3 where)
{
    static const double K = 8.987552e9;

    vec3 vector = { 0.0f, 0.0f, 0.0f };
    for (auto& thing : things)
    {
        vec3 delta = where - thing.p;
        float radius = length(delta);
        vec3 normal = normalize(delta);
        float force = K * thing.q / (radius * radius);
        vector += normal * force;
    }
    return vector;
}

void update_physics()
{
    static const int MAX_LINE_POINTS = 300;
    static const float LINE_MOVE = 0.05;

    for (auto& thing : things)
    {
        for (auto& line : thing.lines)
        {
            if (line.points.size() > MAX_LINE_POINTS) continue;
            vec3 point = *line.points.rbegin();
            vec3 force = get_force_in_point(point);
            vec3 move = normalize(force) * LINE_MOVE;
            if (thing.q < 0) move = -move;
            line.points.push_back(point + move);
        }
    }
}

void render_physics()
{
    for (auto& thing : things)
    {
        vec3 color = { 0.5f, 0.5f, 0.5f };
        if (thing.q < 0) color = { 1.0f, 1.0f, 1.0f };
        sphere(0.3, thing.p, color);
    }

    glBegin(GL_LINES);
    for (auto& thing : things)
    {
        for (auto& line : thing.lines)
        {
            for (int i = 1; i < line.points.size(); i++)
            {
                vec3 a = line.points[i - 1];
                vec3 b = line.points[i];

                float distance = length(b - camera_position);
                vec3 color = { 0.6f, 0.6f, 1.0f };
                color *= (dot(camera_position, b - a) + 1.0) / 2.0;
                // color *= 1.0f - log(distance) * 0.2f;
                glColor3f(color.x, color.y, color.z);

                glVertex3f(a.x, a.y, a.z);
                glVertex3f(b.x, b.y, b.z);
            }
        }
    }
    glEnd();
}
