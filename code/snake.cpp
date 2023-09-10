#include <SDL.h>
#include <glad/glad.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define HANDMADE_MATH_USE_DEGREES
#include "HandmadeMath.h"

#include <stdlib.h>
#include <stdio.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t   s8;
typedef int16_t  s16;
typedef int32_t  s32;
typedef int64_t  s64;
typedef float  f32;
typedef double f64;
typedef s32 b32;

struct Input {
    b32 up;
    b32 down;
    b32 left;
    b32 right;
};
Input input{};
   
enum Dir {
    Left = 1,
    Right,
    Up,
    Down,
};

struct Cell {
    int x;
    int y;
    Dir dir;

    Cell() {
        x = 0;
        y = 0;
        dir = (Dir)0;
    }
    
    Cell(int x, int y) {
        this->x = x;
        this->y = y;
        dir = (Dir)0;
    }

    Cell(int x, int y, Dir dir) {
        this->x = x;
        this->y = y;
        this->dir = dir;
    }
};


#define WIDTH 1280
#define HEIGHT 720

#define ARRLEN(X) (sizeof(X) / sizeof((X)[0]))

u32 quad_shader;
u32 quad_vao;

u32 gl_texture_create(const char *texture_path) {
    stbi_set_flip_vertically_on_load(true);
    int tex_width, tex_height, n;
    unsigned char *tex_data = stbi_load(texture_path, &tex_width, &tex_height, &n, 4);
    if (tex_data == NULL) {
        printf("Failed to load texture: %s\n", texture_path);
    }

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex_width, tex_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, (void *)tex_data);

    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(tex_data);
    return texture;
}

u32 gl_shader_create(const char *vertex_src, const char *frag_src) {
    u32 shader = glCreateProgram();
    int status = 0;
    int n;
    char log[512] = {};

    u32 vshader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vshader, 1, &vertex_src, nullptr);
    glCompileShader(vshader);
    glGetShaderiv(vshader, GL_COMPILE_STATUS, &status);
    if (!status) {
        printf("Failed to compile vertex shader!\n");
    }

    glGetShaderInfoLog(vshader, 512, &n, log);
    if (n > 0) {
        printf("Error in vertex shader!\n");
        printf(log);
    }

    u32 fshader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fshader, 1, &frag_src, nullptr);
    glCompileShader(fshader);
    glGetShaderiv(vshader, GL_COMPILE_STATUS, &status);
    if (!status) {
        printf("Failed to compile fragment shader!\n");
    }
    
    glGetShaderInfoLog(fshader, 512, &n, log);
    if (n > 0) {
        printf("Error in fragment shader!\n");
        printf(log);
    }

    glAttachShader(shader, vshader);
    glAttachShader(shader, fshader);
    glLinkProgram(shader);
    glDeleteShader(vshader);
    glDeleteShader(fshader);

    return shader;
}

u32 gl_shader_create_file(const char *vertex_name, const char *frag_name) {
    char *vertex_src = nullptr;
    char *frag_src = nullptr;
    FILE *file = fopen(vertex_name, "rb");
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    vertex_src = (char *)malloc(size + 1);
    fread(vertex_src, 1, size, file);
    vertex_src[size] = '\0';
    fclose(file);
    file = fopen(frag_name, "rb");
    fseek(file, 0, SEEK_END);
    size = ftell(file);
    fseek(file, 0, SEEK_SET);
    frag_src = (char *)malloc(size + 1);
    fread(frag_src, 1, size, file);
    frag_src[size] = '\0';
    fclose(file);

    u32 result = gl_shader_create(vertex_src, frag_src);

    free(vertex_src);
    free(frag_src);
    return result;
}

void draw_quad(HMM_Vec2 translation, HMM_Vec2 scale, f32 rotation, HMM_Mat4 projection, u32 texture) {
    glBindVertexArray(quad_vao);
    glUseProgram(quad_shader);

    HMM_Mat4 model = HMM_M4D(1.0f);
    model = model * HMM_Translate(HMM_V3(translation.X, translation.Y, 0.0f));

    model = model * HMM_Translate(HMM_V3(0.5f * scale.X, 0.5f * scale.Y, 0.0f));
    model = model * HMM_Rotate_LH(rotation, HMM_V3(0.0f, 0.0f, 1.0f));
    model = model * HMM_Translate(HMM_V3(-0.5f * scale.X, -0.5f * scale.Y, 0.0f));

    model = model * HMM_Scale(HMM_V3(scale.X, scale.Y, 1.0));
    model = projection * model;

    glUniformMatrix4fv(glGetUniformLocation(quad_shader, "wvp"), 1, false, (f32 *)&model);
    glBindTexture(GL_TEXTURE_2D, texture);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

HMM_Vec2 direction_vector(Dir dir) {
    HMM_Vec2 result{};
    switch (dir) {
    case Left:
        result.X = -1.0f;
        break;
    case Right:
        result.X = 1.0f;
        break;
    case Up:
        result.Y = 1.0f;
        break;
    case Down:
        result.Y = -1.0f;
        break;
    }
    return result;
}

f32 direction_rotation(Dir dir) {
    f32 rot = 0.0f;
    switch (dir) {
    case Left:
        rot = 180.0f;
        break;
    case Right:
        rot = 0.0f;
        break;
    case Up:
        rot = 90.0f;
        break;
    case Down:
        rot = 270.0f;
        break;
    }

    return rot;
}


void snake_step(Cell *snake_cells, int snake_length) {
    Dir dir = snake_cells[0].dir;
    snake_cells[0].x += (dir==Left) ? -1 : (dir==Right) ? 1 : 0;
    snake_cells[0].y += (dir==Down) ? -1 : (dir==Up) ? 1 : 0;

    for (int i = snake_length - 1; i > 0; i--) {
        Dir dir = snake_cells[i].dir;
        snake_cells[i].x += (dir==Left) ? -1 : (dir==Right) ? 1 : 0;
        snake_cells[i].y += (dir==Down) ? -1 : (dir==Up) ? 1 : 0;
        snake_cells[i].dir = snake_cells[i - 1].dir;
    }
}

void grow_snake(Cell *snake_cells, int snake_length) {
    Cell tail = snake_cells[snake_length - 1];
    Cell new_cell = tail;
    switch (tail.dir) {
    case Left:
        new_cell.x += 1;
        break;
    case Right:
        new_cell.x -= 1;
        break;
    case Up:
        new_cell.y -= 1;
        break;
    case Down:
        new_cell.y += 1;
        break;
    }
    snake_cells[snake_length] = new_cell;
}

Cell spawn_apple(Cell *snake_cells, int snake_length, int grid_x, int grid_y) {
    Cell result{};
    for (;;) {
        srand(SDL_GetTicks());
        int x = rand() % grid_x;
        int y = rand() % grid_y;
        b32 collision = false;
        for (int i = 0; i < snake_length; i++) {
            if (x == snake_cells[i].x || y == snake_cells[i].y) {
                collision = true;
            }
        }

        if (!collision) {
            result.x = x;
            result.y = y;
            break;
        }
    }
    return result;
}

int main(int argc, char **argv) {
    if (SDL_Init(SDL_INIT_VIDEO)) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "SDL Error", "Failed to initialize SDL Video", NULL);
        return -1;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 16 );
    SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 ); 

    SDL_Window *window = SDL_CreateWindow("Snake", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WIDTH, HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL);

    if (window == nullptr) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "SDL Error", "Failed to create SDL window", NULL);
        return -1;
    }
    
    SDL_GLContext context = SDL_GL_CreateContext(window);
    gladLoadGLLoader(SDL_GL_GetProcAddress);

    const char *quad_vshader = "#version 330 core\n"
        "layout (location = 0) in vec2 in_pos;\n"
        "layout (location = 1) in vec2 in_uv;\n"
        "uniform mat4 wvp;\n"
        "out vec2 uv;\n"
        "void main() {\n"
        "gl_Position = wvp * vec4(in_pos, 0.0, 1.0);\n"
        "uv = in_uv;\n"
        "}\0";
    const char *quad_fshader = "#version 330 core\n"
        "in vec2 uv;\n"
        "uniform sampler2D quad_tex;\n"
        "out vec4 out_color;\n"
        "void main() {\n"
        "out_color = texture(quad_tex, uv);\n"
        "}\0";

    quad_shader = gl_shader_create(quad_vshader, quad_fshader);

    f32 quad_verts[] = {
        0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 1.0f,
        1.0f, 1.0f, 1.0f, 1.0f,

        0.0f, 0.0f, 0.0f, 0.0f,
        1.0f, 1.0f, 1.0f, 1.0f,
        1.0f, 0.0f, 1.0f, 0.0f,
    };

    u32 cell_texture = gl_texture_create("data/cell.png");
    u32 grid_texture = gl_texture_create("data/grid.png");
    u32 apple_texture = gl_texture_create("data/apple.png");

    u32 quad_vbo;
    glGenBuffers(1, &quad_vbo);
    glGenVertexArrays(1, &quad_vao);
    
    glBindVertexArray(quad_vao);
    glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad_verts), quad_verts, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, false, 4 * sizeof(f32), (void *)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, false, 4 * sizeof(f32), (void *)(2 * sizeof(f32)));

#define GRID_X 16
#define GRID_Y 9
#define MAX_CELLS 100

    f32 cell_width = ((f32)WIDTH / (f32)GRID_X);
    f32 cell_height = ((f32)HEIGHT / (f32)GRID_Y);

    Dir selected_dir = Right;
    
    int snake_length = 1;
    Cell *snake_cells = (Cell *)calloc(MAX_CELLS, sizeof(Cell));
    snake_cells[0] = Cell(0, 4, Right);

    Cell apple = spawn_apple(snake_cells, snake_length, GRID_X, GRID_Y);

    u32 start_time = SDL_GetTicks();
    bool window_should_close = false; 
    while (!window_should_close) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) { 
            switch (event.type) {
            case SDL_WINDOWEVENT_SIZE_CHANGED: {
                glViewport(0, 0, event.window.data1, event.window.data2);
            } break;
            case SDL_KEYDOWN:
            case SDL_KEYUP: {
                b32 is_down = event.key.state == SDL_PRESSED;
                switch (event.key.keysym.sym) {
                case SDLK_UP:
                    input.up = is_down;
                    break;
                case SDLK_DOWN:
                    input.down = is_down;
                    break;
                case SDLK_LEFT:
                    input.left = is_down;
                    break;
                case SDLK_RIGHT:
                    input.right = is_down;
                    break;
                }
            } break;
            case SDL_QUIT:
                window_should_close = true;
                break;
            }
        }
      
        Dir dir = snake_cells[0].dir;
        if (input.left && dir != Right) {
            selected_dir = Left;
        }
        else if (input.right && dir != Left) {
            selected_dir = Right;
        }
        else if (input.up && dir != Down) {
            selected_dir = Up;
        }
        else if (input.down && dir != Up) {
            selected_dir = Down;
        }

        f32 time = (f32)(SDL_GetTicks() - start_time) / 1000.0f;
        if (time >= 0.2f) {
            snake_cells[0].dir = selected_dir;
            snake_step(snake_cells, snake_length);
            start_time = SDL_GetTicks();
        }

        if (snake_cells[0].x == apple.x && snake_cells[0].y == apple.y) {
            grow_snake(snake_cells, snake_length);
            snake_length++;
            apple = spawn_apple(snake_cells, snake_length, GRID_X, GRID_Y);
        }

        int width, height;
        SDL_GetWindowSize(window, &width, &height);
        HMM_Mat4 projection = HMM_Orthographic_RH_NO(0.0f, (float)width, 0.0f, (float)height, -1.0f, 1.0f);
        
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glClear(GL_COLOR_BUFFER_BIT);
        glClearColor(1.0f, 0.0f, 1.0f, 1.0f);

        draw_quad(HMM_V2(0.0f, 0.0f), HMM_V2((f32)width, (f32)height), 0.0f, projection, grid_texture);

        const HMM_Vec2 cell_size = HMM_V2(cell_width, cell_height);

        draw_quad(HMM_V2(apple.x * cell_width, apple.y * cell_height), cell_size, (f32)SDL_GetTicks() * 0.1f, projection, apple_texture);

        for (int i = 0; i < snake_length; i++) {
            f32 rot = 0.0f;
            HMM_Vec2 pos = HMM_V2(snake_cells[i].x * cell_width, snake_cells[i].y * cell_height);
            draw_quad(pos, cell_size, rot, projection, cell_texture);
        }

        SDL_GL_SwapWindow(window);
    }

    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
