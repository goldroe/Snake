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

#include "snake.h"

#define WIDTH 1280
#define HEIGHT 720
#define CELL_Y 20
#define MAX_CELLS 512

u32 quad_shader;
u32 quad_vao;
u32 grid_vao;
u32 text_vao;

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
    HMM_Mat4 model = HMM_M4D(1.0f);
    model = model * HMM_Translate(HMM_V3(translation.X, translation.Y, 0.0f));

    model = model * HMM_Translate(HMM_V3(0.5f * scale.X, 0.5f * scale.Y, 0.0f));
    model = model * HMM_Rotate_LH(rotation, HMM_V3(0.0f, 0.0f, 1.0f));
    model = model * HMM_Translate(HMM_V3(-0.5f * scale.X, -0.5f * scale.Y, 0.0f));

    model = model * HMM_Scale(HMM_V3(scale.X, scale.Y, 1.0));
    model = projection * model;

    glBindVertexArray(quad_vao);
    glUseProgram(quad_shader);
    glUniformMatrix4fv(glGetUniformLocation(quad_shader, "wvp"), 1, false, (f32 *)&model);
    glBindTexture(GL_TEXTURE_2D, texture);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void draw_grid(HMM_Vec2 window_dim, f32 cell_size, u32 texture, HMM_Mat4 projection) {

    f32 cell_x = (window_dim.Width / cell_size) / 2.0f;
    f32 cell_y = (window_dim.Height / cell_size) / 2.0f;

    f32 grid_verts[] = {
        0.0f, 0.0f,                            0.0f, 0.0f,
        0.0f, window_dim.Height,               0.0f, cell_y,
        window_dim.Width, window_dim.Height,   cell_x, cell_y,

        0.0f, 0.0f,                            0.0f, 0.0f,
        window_dim.Width, window_dim.Height,   cell_x, cell_y,
        window_dim.Width, 0.0f,                cell_x, 0.0f
    };

    glBindVertexArray(grid_vao);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(grid_verts), (void *)grid_verts);
    glUniformMatrix4fv(glGetUniformLocation(quad_shader, "wvp"), 1, false, (float *)&projection);
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

Cell spawn_apple(Cell *snake_cells, int snake_length, int cell_x, int cell_y) {
    Cell result{};
    srand(SDL_GetTicks());
    for (;;) {
        int x = rand() % cell_x;
        int y = rand() % cell_y;
        b32 collision = false;
        for (int i = 0; i < snake_length; i++) {
            if (x == snake_cells[i].x && y == snake_cells[i].y) {
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

void draw_text(const char *text, HMM_Vec2 start, f32 char_size, u32 texture, HMM_Mat4 projection) {
    int vert_count = 0;
    QuadV vertices[1024]{};

    HMM_Vec2 pos = start;
    f32 glyph_step = 8.0f / 472.0f;
    
    for (const char *ptr = text; *ptr; ptr++) {
        char ch = *ptr;
        if (ch == '\n') {
            pos.x = start.x;
            pos.y -= char_size;
            continue;
        }

        f32 char_off = (f32)(ch - ' ');
        f32 off_x = char_off * glyph_step;

        vertices[vert_count++] = {pos.x, pos.y,                          off_x, 0.0f};
        vertices[vert_count++] = {pos.x, pos.y + char_size,              off_x, 1.0f};
        vertices[vert_count++] = {pos.x + char_size, pos.y + char_size,  off_x + glyph_step, 1.0f};
        vertices[vert_count++] = {pos.x, pos.y,                          off_x, 0.0f};
        vertices[vert_count++] = {pos.x + char_size, pos.y + char_size,  off_x + glyph_step, 1.0f};
        vertices[vert_count++] = {pos.x + char_size, pos.y,              off_x + glyph_step, 0.0f};

        pos.x += char_size;
    }

    glBindVertexArray(text_vao);
    glBufferData(GL_ARRAY_BUFFER, vert_count * sizeof(QuadV), vertices, GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, false, 4 * sizeof(f32), (void *)0);
    glVertexAttribPointer(1, 2, GL_FLOAT, false, 4 * sizeof(f32), (void *)(2 * sizeof(f32)));

    glUseProgram(quad_shader);
    glUniformMatrix4fv(glGetUniformLocation(quad_shader, "wvp"), 1, false, (f32 *)&projection);
    glBindTexture(GL_TEXTURE_2D, texture);
    glDrawArrays(GL_TRIANGLES, 0, vert_count);
}

int main(int argc, char **argv) {
    if (SDL_Init(SDL_INIT_VIDEO)) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "SDL Error", "Failed to initialize SDL Video", NULL);
        return -1;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    // SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 16 );
    SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 ); 

    SDL_Window *window = SDL_CreateWindow("Snake", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WIDTH, HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);

    if (window == nullptr) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "SDL Error", "Failed to create SDL window", NULL);
        return -1;
    }
    
    SDL_GLContext context = SDL_GL_CreateContext(window);
    gladLoadGLLoader(SDL_GL_GetProcAddress);

    u32 arrow_texture = gl_texture_create("data/arrow.png");
    u32 cell_texture = gl_texture_create("data/cell.png");
    u32 apple_texture = gl_texture_create("data/apple.png");
    u32 font_texture = gl_texture_create("data/font.png");
    u32 grid_texture = gl_texture_create("data/grid.png");
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

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

    u32 quad_vbo;
    glGenBuffers(1, &quad_vbo);
    glGenVertexArrays(1, &quad_vao);
    glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad_verts), quad_verts, GL_STATIC_DRAW);
    glBindVertexArray(quad_vao);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, false, 4 * sizeof(f32), (void *)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, false, 4 * sizeof(f32), (void *)(2 * sizeof(f32)));
    glBindVertexArray(0);

    u32 text_vbo;
    glGenBuffers(1, &text_vbo);
    glGenVertexArrays(1, &text_vao);
    glBindBuffer(GL_ARRAY_BUFFER, text_vbo);
    glBufferData(GL_ARRAY_BUFFER, 0, NULL, GL_DYNAMIC_DRAW);
    glBindVertexArray(text_vao);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    u32 grid_vbo;
    glGenBuffers(1, &grid_vbo);
    glGenVertexArrays(1, &grid_vao);
    glBindBuffer(GL_ARRAY_BUFFER, grid_vbo);
    glBufferData(GL_ARRAY_BUFFER, 6 * 4 * sizeof(f32), 0, GL_STATIC_DRAW);
    glBindVertexArray(grid_vao);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, false, 4 * sizeof(f32), (void *)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, false, 4 * sizeof(f32), (void *)(2 * sizeof(f32)));
    glBindVertexArray(0);

    int window_width, window_height;
    SDL_GetWindowSize(window, &window_width, &window_height);
    
    int cell_y = CELL_Y;
    f32 cell_size = (f32)window_height / (f32)cell_y;
    int cell_x = (int)(window_width / cell_size);
   
    Dir selected_dir = Right;
    
    int snake_length = 1;
    Cell *snake_cells = (Cell *)calloc(MAX_CELLS, sizeof(Cell));
    snake_cells[0] = Cell(0, 4, Right);

    Cell apple = spawn_apple(snake_cells, snake_length, cell_x, cell_y);

    Input input{};
    bool window_should_close = false; 
    GameState game_state{};
    b32 start_selected = true;
    b32 exit_selected = false;

    u32 start_time = SDL_GetTicks();
    while (!window_should_close) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) { 
            switch (event.type) {
            case SDL_WINDOWEVENT: {
                if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                    glViewport(0, 0, event.window.data1, event.window.data2);
                }
            } break;
            case SDL_KEYDOWN:
            case SDL_KEYUP: {
                b32 is_down = event.key.state == SDL_PRESSED;
                switch (event.key.keysym.sym) {
                case SDLK_RETURN:
                    input.enter = is_down;
                    break;
                case SDLK_UP:
                case 'w':
                    input.up = is_down;
                    break;
                case SDLK_DOWN:
                case 's':
                    input.down = is_down;
                    break;
                case SDLK_LEFT:
                case 'a':
                    input.left = is_down;
                    break;
                case SDLK_RIGHT:
                case 'd':
                    input.right = is_down;
                    break;
                }
            } break;
            case SDL_QUIT:
                window_should_close = true;
                break;
            }
        }
      
        SDL_GetWindowSize(window, &window_width, &window_height);
        HMM_Mat4 projection = HMM_Orthographic_RH_NO(0.0f, (float)window_width, 0.0f, (float)window_height, -1.0f, 1.0f);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glClear(GL_COLOR_BUFFER_BIT);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

        if (game_state.game_mode == Mode_Start) {
            if (input.up) {
                if (exit_selected) {
                    start_selected = true;
                    exit_selected = false;
                }
            } else if (input.down) {
                if (start_selected) {
                    start_selected = false;
                    exit_selected = true;
                }
            }

            if (input.enter) {
                if (start_selected) {
                    game_state.game_mode = Mode_Play;
                } else if (exit_selected) {
                    window_should_close = true;
                }
            }
 
            draw_text("SNAKE 2D\nSTART\nEXIT", HMM_V2(400.0f, 600.0f), 50.0f, font_texture, projection);
           
            if (start_selected) {
                draw_quad(HMM_V2(400.0f - 50.0f, 550.0f), HMM_V2(50.0f, 50.0f), 0.0f, projection, arrow_texture);
            } else if (exit_selected) {
                draw_quad(HMM_V2(400.0f - 50.0f, 500.0f), HMM_V2(50.0f, 50.0f), 0.0f, projection, arrow_texture);
            }
        } else if (game_state.game_mode == Mode_Play) {
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
            if (time >= 0.1f) {
                snake_cells[0].dir = selected_dir;
                snake_step(snake_cells, snake_length);

                // Death
                Cell *head = snake_cells;
                if (head->x > cell_x || head->x < 0 || head->y < 0 || head->y > cell_y) {
                    game_state.game_mode = Mode_End;
                }
                for (int i = 1; i < snake_length + 1; i++) {
                    if (head->x == snake_cells[i].x && head->y == snake_cells[i].y) {
                        game_state.game_mode = Mode_End;
                    }
                }

                start_time = SDL_GetTicks();
            }

            if (snake_cells[0].x == apple.x && snake_cells[0].y == apple.y) {
                grow_snake(snake_cells, snake_length);
                snake_length++;
                apple = spawn_apple(snake_cells, snake_length, cell_x, cell_y);
            }
            draw_grid(HMM_V2((float)window_width, (float)window_height), cell_size, grid_texture, projection);

            HMM_Vec2 cell_dim = HMM_V2(cell_size, cell_size);

            draw_quad(HMM_V2(apple.x * cell_size, apple.y * cell_size), cell_dim, (f32)SDL_GetTicks() * 0.1f, projection, apple_texture);

            for (int i = 0; i < snake_length; i++) {
                f32 rot = 0.0f;
                HMM_Vec2 pos = HMM_V2(snake_cells[i].x * cell_size, snake_cells[i].y * cell_size);
                draw_quad(pos, cell_dim, rot, projection, cell_texture);
            }

            char buffer[12]{};
            sprintf(buffer, "%d", snake_length);
            draw_text((const char *)buffer, HMM_V2(0.0f, 0.0f), 30.0f, font_texture, projection);
        } else if (game_state.game_mode == Mode_End) {
            draw_text("GAME OVER", HMM_V2(400.0f, 600.0f), 40.0f, font_texture, projection);
        }

        SDL_GL_SwapWindow(window);
    }

    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
