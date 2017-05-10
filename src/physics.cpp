
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

void add_object(Object object)
{
    if (object.kind == OBJECT_POINT)
    {
        object.s = { 0, 0, 0 };

        Thing thing;
        thing.p = object.p;
        thing.q = object.q;
        object.things.push_back(thing);
    }
    else
    {
        int div = 3;
        int cx = (int)(object.s.x + 0.5) * div;
        int cy = (int)(object.s.y + 0.5) * div;
        int cz = (int)(object.s.z + 0.5) * div;
        Thing thing;
        thing.q = object.q / (float)(cx * cy * cz);
        for (int x = 0; x < cx; x++)
        {
            float ox = (x + 0.5f) / (float) cx;
            for (int y = 0; y < cy; y++)
            {
                float oy = (y + 0.5f) / (float) cy;
                for (int z = 0; z < cz; z++)
                {
                    float oz = (z + 0.5f) / (float) cz;
                    vec3 o = { ox, oy, oz };
                    thing.p = object.p + o * object.s;
                    object.things.push_back(thing);
                }
            }
        }
    }

    for (auto& object : objects)
        for (auto& line : object.lines)
            line.points.resize(1);

    int line_count = (int) ceil(fabs(object.q * 8.0));
    if (line_count < 8) line_count = 8;
    if (line_count > 40) line_count = 40;

    vec3 points[line_count];
    distribute(points, line_count);
    for (int i = 0; i < line_count; i++)
    {
        if (object.kind == OBJECT_BOX)
        {
            float len = length(object.s / 2.0f);
            points[i] *= len;
            if (fabs(points[i].x) > object.s.x / 2.0) points[i] *= fabs(object.s.x / 2.0 / points[i].x);
            if (fabs(points[i].y) > object.s.y / 2.0) points[i] *= fabs(object.s.y / 2.0 / points[i].y);
            if (fabs(points[i].z) > object.s.z / 2.0) points[i] *= fabs(object.s.z / 2.0 / points[i].z);
        }
        else
        {
            points[i] *= 0.1f;
        }
        Line line;
        line.points.push_back(object.p + object.s * 0.5f + points[i]);
        object.lines.push_back(line);
    }

    objects.push_back(object);
}

vec3 get_force_in_point(vec3 where)
{
    static const double K = 8.987552e9;

    vec3 vector = { 0.0f, 0.0f, 0.0f };
    for (auto& object : objects)
        for (auto& thing : object.things)
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
    static const float LINE_MOVE = 0.1;

    for (auto& object : objects)
        for (auto& line : object.lines)
        {
            if (line.points.size() > MAX_LINE_POINTS) continue;
            vec3 point = *line.points.rbegin();
            vec3 force = get_force_in_point(point);
            vec3 move = normalize(force) * LINE_MOVE;
            if (object.q < 0) move = -move;
            line.points.push_back(point + move);
        }
}

void render_physics()
{
    for (auto& object : objects)
    {
        vec3 color = { 0.5f, 0.5f, 0.5f };
        if (object.q < 0) color = { 1.0f, 1.0f, 1.0f };
        if (object.kind == OBJECT_POINT)
            sphere(0.3, object.p, color);
        else
            box(object.p, object.s, color);
    }

    glLineWidth(2);
    glBegin(GL_LINES);
    for (auto& object : objects)
        for (auto& line : object.lines)
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
    glEnd();
    glLineWidth(1);
}
