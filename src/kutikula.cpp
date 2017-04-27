#include <windows.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <vector>
using namespace std;

#include "GL/glew.h"
#include "GL/gl.h"
#include "GL/glu.h"
#include "glm/glm.hpp"
#include "glm/gtx/transform.hpp"
using namespace glm;

#include "SDL2/SDL.h"
#include "SDL2/SDL_mixer.h"
#undef main

#define STB_RECT_PACK_IMPLEMENTATION
#include "stb/stb_rect_pack.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb/stb_truetype.h"

namespace kutikula
{

#define PI  3.14159265359
#define TAU 6.28318530718

#define K 8.987552e9

#define PIXELS_TO_METERS 1.0 / 100.0
#define METERS_TO_PIXELS 100.0

struct Point
{
    vec2 location;
    float q;
    int qq;
};

struct Line
{
    vector<vec2> points;
    bool rev;
};

vector<Point> points;
vector<Line> lines;

bool window_should_close = false;
int window_width;
int window_height;

SDL_Window* the_window = NULL;
SDL_GLContext the_context = NULL;

#define MAX_Q 50

bool update_logic = false;
int current_q = 1;

#define WINDOW_TITLE    "Kutikula"
#define WINDOW_WIDTH    1200
#define WINDOW_HEIGHT   675

GLuint font_texture;
stbtt_bakedchar cdata[96];

void report(const char* message)
{
    MessageBox(NULL, message, "Error", MB_ICONERROR);
    exit(EXIT_FAILURE);
}

void init()
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO))
    {
        report("SDL2 failed to initialize!");
    }
    atexit(SDL_Quit);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    auto window_flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
    the_window = SDL_CreateWindow(WINDOW_TITLE, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, window_flags);
    if (!the_window)
    {
        report("SDL2 failed to create a window!");
    }
    SDL_ShowWindow(the_window);

    the_context = SDL_GL_CreateContext(the_window);
    if (!the_context)
    {
        report("Failed to create a OpenGL context!");
    }

    SDL_GL_SetSwapInterval(1);

    glewInit();

    auto buffer_size = 1 << 20;
    unsigned char* ttf_buffer = (unsigned char*) malloc(buffer_size);
    FILE* file = fopen("segoeui.ttf", "rb");
    fread(ttf_buffer, 1, buffer_size, file);
    fclose(file);

    auto alphamap = (unsigned char*) malloc(512 * 512);
    stbtt_BakeFontBitmap(ttf_buffer, 0, 64.0, alphamap, 512, 512, 32, 96, &cdata[0]);
    free(ttf_buffer);

    auto bitmap = (unsigned char*) malloc(512 * 512 * 4);
    for (int y = 0; y < 512; y++)
    {
        for (int x = 0; x < 512; x++)
        {
            int ia = y * 512 + x;
            int ib = ia * 4;
            bitmap[ib] = bitmap[ib + 1] = bitmap[ib + 2] = 255;
            bitmap[ib + 3] = alphamap[ia];
        }
    }
    free(alphamap);

    glGenTextures(1, &font_texture);
    glBindTexture(GL_TEXTURE_2D, font_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 512, 512, 0, GL_RGBA, GL_UNSIGNED_BYTE, bitmap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    free(bitmap);
}

void on_left_mouse(float x, float y)
{
    if (current_q == 0) return;

    Point point;
    point.location = vec2(x, y);
    point.q = 1e-7;
    point.q *= current_q;
    point.qq = current_q;

    int count_lines = 4 * abs(current_q); // cast(int32)(lerp(4, 80, cast(float32)(log(cast(float32) current_q) / log(cast(float32) MAX_Q))) + 0.5);
    for (int i = 0; i < count_lines; i++)
    {
        float angle = i / (float) count_lines * TAU;
        float x = point.location.x + cos(angle) * PIXELS_TO_METERS * 12;
        float y = point.location.y + sin(angle) * PIXELS_TO_METERS * 12;
        Line line;
        line.points.push_back(vec2(x, y));
        line.rev = current_q < 0;
        lines.push_back(line);
    }

    points.push_back(point);
    update_logic = true;
}

void handle_events()
{
    SDL_GetWindowSize(the_window, &window_width, &window_height);
    glViewport(0, 0, window_width, window_height);

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        case SDL_QUIT:
        {
            window_should_close = true;
        } break;
        case SDL_MOUSEBUTTONDOWN:
        {
            if (event.button.button == SDL_BUTTON_LEFT)
            {
                on_left_mouse(event.button.x * PIXELS_TO_METERS, (window_height - event.button.y - 1) * PIXELS_TO_METERS);
            }
        } break;
        case SDL_KEYDOWN:
        {
            SDL_Scancode digit_codes[] = { SDL_SCANCODE_0, SDL_SCANCODE_1, SDL_SCANCODE_2, SDL_SCANCODE_3, SDL_SCANCODE_4,
                                           SDL_SCANCODE_5, SDL_SCANCODE_6, SDL_SCANCODE_7, SDL_SCANCODE_8, SDL_SCANCODE_9 };
            auto scancode = event.key.keysym.scancode;
            if (scancode == SDL_SCANCODE_SPACE)
            {
                points.clear();
                lines.clear();
            }
            else if (scancode == SDL_SCANCODE_BACKSPACE)
            {
                current_q /= 10;
            }
            else if (scancode == SDL_SCANCODE_MINUS)
            {
                current_q = -current_q;
            }
            else
            {
                int digit = 0;
                while ((digit < 10) && (digit_codes[digit] != scancode))
                    digit += 1;
                if (digit < 10)
                {
                    current_q *= 10;
                    current_q += digit;
                    if (current_q > MAX_Q)
                        current_q = MAX_Q;
                }
            }
        } break;
        }
    }
}

void update()
{
    if (!update_logic) return;
    update_logic = false;

    #define FIELD_DENSITY 0.5
    int field_width = (int)(window_width / FIELD_DENSITY);
    int field_height = (int)(window_height / FIELD_DENSITY);
    vec2* field = new vec2[field_width * field_height];

    for (int x = 0; x < field_width; x++)
        for (int y = 0; y < field_height; y++)
        {
            vec2 where = vec2(x, y) * (float)(FIELD_DENSITY * PIXELS_TO_METERS);
            vec2 vector = vec2(0, 0);
            for (auto point : points)
            {
                vec2 delta = where - point.location;
                float radius = length(delta);
                vec2 normal = normalize(delta);
                float force = K * point.q / (radius * radius);
                vector += normal * force;
            }
            field[y * field_width + x] = vector;
        }

    for (auto& line : lines)
    {
        line.points.resize(1);

        for (int i = 0; i < 5000; i++)
        {
            vec2 last = *line.points.rbegin();
            int x = (int)(last.x * METERS_TO_PIXELS / FIELD_DENSITY + 0.5);
            int y = (int)(last.y * METERS_TO_PIXELS / FIELD_DENSITY + 0.5);
            if (x < 0 || x >= field_width || y < 0 || y >= field_height) break;
            vec2 vector = normalize(field[y * field_width + x]) * (float)(FIELD_DENSITY * PIXELS_TO_METERS);
            vec2 next = last;
            if (line.rev)
                next -= vector;
            else
                next += vector;
            line.points.push_back(next);
        }
    }

    delete[] field;
}

void render_string(float x, float y, float scale, float center, char *text)
{
    auto t = text;
    float tx, ty;
    tx = ty = 0;
    while (*text)
    {
        if (*text >= 32 && *text < 128)
        {
            stbtt_aligned_quad q;
            stbtt_GetBakedQuad(cdata, 512, 512, *text - 32, &tx, &ty, &q, 1);
        }
        text++;
    }
    text = t;
    float width = tx;
    x -= width * scale * center;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, font_texture);
    glBegin(GL_QUADS);
    tx = ty = 0;
    while (*text)
    {
        if (*text >= 32 && *text < 128)
        {
            stbtt_aligned_quad q;
            stbtt_GetBakedQuad(cdata, 512, 512, *text - 32, &tx, &ty, &q, 1);
            glTexCoord2f(q.s0, q.t0); glVertex2f(x + scale * q.x0, y - scale * q.y0);
            glTexCoord2f(q.s1, q.t0); glVertex2f(x + scale * q.x1, y - scale * q.y0);
            glTexCoord2f(q.s1, q.t1); glVertex2f(x + scale * q.x1, y - scale * q.y1);
            glTexCoord2f(q.s0, q.t1); glVertex2f(x + scale * q.x0, y - scale * q.y1);
        }
        text++;
    }
    glEnd();
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
}

void render()
{
    char buffer[128];

    glClearColor(36.0/255.0, 147.0/255.0, 68.0/255.0, 255.0/255.0);
    glClear(GL_COLOR_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, window_width, 0, window_height, -1, 1);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glColor3f(1, 1, 1);
    for (auto point : points)
    {
        float x = point.location.x * METERS_TO_PIXELS;
        float y = point.location.y * METERS_TO_PIXELS;
        float radius = log(fabs(point.q)) / log(10) * 3 + 25;
        glBegin(GL_POLYGON);
        for (int i = 0; i < 16; i++)
        {
            float ox = x + cos(i / 16.0 * TAU) * radius;
            float oy = y + sin(i / 16.0 * TAU) * radius;
            glVertex2f(ox, oy);
        }
        glEnd();
    }

    glColor3f(0, 0, 1);
    for (auto& line : lines)
    {
        glBegin(GL_LINE_STRIP);
        for (auto point : line.points)
        {
            float x = point.x * METERS_TO_PIXELS;
            float y = point.y * METERS_TO_PIXELS;
            glVertex2f(x, y);
        }
        glEnd();
    }

    glColor3f(1, 1, 1);
    for (auto point : points)
    {
        float x = point.location.x * METERS_TO_PIXELS;
        float y = point.location.y * METERS_TO_PIXELS;
        float radius = log(fabs(point.q)) / log(10) * 3 + 25;
        sprintf(buffer, "%d nC", point.qq);
        render_string(x, y + radius + 5, 0.3, 0.5, buffer);
    }

    glColor3f(1, 1, 1);
    sprintf(buffer, "q: %d nC", current_q);
    render_string(50, 50, 0.5, 0, buffer);
}

void entry()
{
    init();
    while (!window_should_close)
    {
        handle_events();
        update();
        render();
        SDL_GL_SwapWindow(the_window);
    }
}

}

// int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
int main()
{
    kutikula::entry();
    return EXIT_SUCCESS;
}
