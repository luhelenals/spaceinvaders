#include <string>
#include <iostream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <cstdint>
#include <vector>
#include "shaderFunctions.cpp"
#include "Items.cpp"

GLFWwindow* window = NULL;
int buffer_width = 224, buffer_height = 256;
size_t score = 0;
using namespace std;

#define GL_ERROR_CASE(glerror)\
    case glerror: snprintf(error, sizeof(error), "%s", #glerror)

inline void gl_debug(const char* file, int line) {
	GLenum err;
	while ((err = glGetError()) != GL_NO_ERROR) {
		char error[128];

		switch (err) {
			GL_ERROR_CASE(GL_INVALID_ENUM); break;
			GL_ERROR_CASE(GL_INVALID_VALUE); break;
			GL_ERROR_CASE(GL_INVALID_OPERATION); break;
			GL_ERROR_CASE(GL_INVALID_FRAMEBUFFER_OPERATION); break;
			GL_ERROR_CASE(GL_OUT_OF_MEMORY); break;
		default: snprintf(error, sizeof(error), "%s", "UNKNOWN_ERROR"); break;
		}

		fprintf(stderr, "%s - %s: %d\n", error, file, line);
	}
}

#undef GL_ERROR_CASE

void error_callback(int error, const char* description);
uint32_t rgb_to_uint32(uint8_t r, uint8_t g, uint8_t b);
void buffer_clear(Buffer* buffer, uint32_t color);
void buffer_draw_sprite(Buffer* buffer, const Sprite& sprite, size_t x, size_t y, uint32_t color);
bool validate_program(GLuint program);
void validate_shader(GLuint shader, const char* file);
void CreateTexture(GLuint &buffer_texture, Buffer buffer);
Sprite CreatePlayer();
Buffer CreateBuffer();
Game CreateGame();
SpriteAnimation* CreateAnimation(Sprite* alien_sprites);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
Sprite CreateBullet();
Sprite* CreateAlienSprites();
Sprite CreateDeathSprite();
bool sprite_overlap_check(const Sprite& sp_a, size_t x_a, size_t y_a, const Sprite& sp_b, size_t x_b, size_t y_b);
Sprite CreateTextSprite(char letter);

bool game_running = false;
int move_dir = 0;
bool fire_pressed = 0;

int main(void) {
    const size_t buffer_width = 224;
    const size_t buffer_height = 256;

    glfwSetErrorCallback(error_callback);

    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    /* Create a windowed mode window and its OpenGL context */
    GLFWwindow* window = glfwCreateWindow(2 * buffer_width, 2 * buffer_height, "Space Invaders", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    glfwSetKeyCallback(window, key_callback);

    glfwMakeContextCurrent(window);

    GLenum err = glewInit();
    if (err != GLEW_OK)
    {
        fprintf(stderr, "Error initializing GLEW.\n");
        glfwTerminate();
        return -1;
    }

    int glVersion[2] = { -1, 1 };
    glGetIntegerv(GL_MAJOR_VERSION, &glVersion[0]);
    glGetIntegerv(GL_MINOR_VERSION, &glVersion[1]);

    gl_debug(__FILE__, __LINE__);

    printf("Using OpenGL: %d.%d\n", glVersion[0], glVersion[1]);
    printf("Renderer used: %s\n", glGetString(GL_RENDERER));
    printf("Shading Language: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

    glfwSwapInterval(1);

    glClearColor(1.0, 0.0, 0.0, 1.0);

    // Create graphics buffer
    Buffer buffer;
    buffer.width = buffer_width;
    buffer.height = buffer_height;
    buffer.data = new uint32_t[buffer.width * buffer.height];

    buffer_clear(&buffer, 0);

    // Create texture for presenting buffer to OpenGL
    GLuint buffer_texture;
    glGenTextures(1, &buffer_texture);
    glBindTexture(GL_TEXTURE_2D, buffer_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, buffer.width, buffer.height, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, buffer.data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);


    // Create vao for generating fullscreen triangle
	GLuint fullscreen_triangle_vao;
	glGenVertexArrays(1, &fullscreen_triangle_vao);
	ShaderProgramSource source = ParseShader("C:/Users/luiza/source/repos/Space Invaders/shaders/Source.shader");

    // Create shader for displaying buffer
    unsigned int shader_id = createShader(source.VertexSource, source.FragmentSource);
    glLinkProgram(shader_id);

    if (!validate_program(shader_id)) {
        fprintf(stderr, "Error while validating shader.\n");
        glfwTerminate();
        glDeleteVertexArrays(1, &fullscreen_triangle_vao);
        delete[] buffer.data;
        return -1;
    }

    glUseProgram(shader_id);

    GLint location = glGetUniformLocation(shader_id, "buffer");
    glUniform1i(location, 0);

	// After linking the shader program, get the brightness uniform location
	GLint brightnessLocation = glGetUniformLocation(shader_id, "brightness");

	float brightness = 1.0f;  // Default brightness

    //OpenGL setup
    glDisable(GL_DEPTH_TEST);
    glActiveTexture(GL_TEXTURE0);

    glBindVertexArray(fullscreen_triangle_vao);

    // Prepare game
    Sprite *alien_sprites = CreateAlienSprites();

    Sprite alien_death_sprite = CreateDeathSprite();

    Sprite player_sprite = CreatePlayer();

    Sprite bullet_sprite = CreateBullet();

	Sprite text[5];
	text[0] = CreateTextSprite('S');
	text[1] = CreateTextSprite('C');
	text[2] = CreateTextSprite('O');
	text[3] = CreateTextSprite('R');
	text[4] = CreateTextSprite('E');

	SpriteAnimation *alien_animation = CreateAnimation(alien_sprites);

    Game game = CreateGame();

	GLuint vao, vbo;
	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);

    for (size_t yi = 0; yi < 5; ++yi)
    {
        for (size_t xi = 0; xi < 11; ++xi)
        {
            Alien& alien = game.aliens[yi * 11 + xi];
            alien.type = (5 - yi) / 2 + 1;

            const Sprite& sprite = alien_sprites[2 * (alien.type - 1)];

            alien.x = 16 * xi + 20 + (alien_death_sprite.width - sprite.width) / 2;
            alien.y = 17 * yi + 128;
        }
    }

    uint8_t* death_counters = new uint8_t[game.num_aliens];
    for (size_t i = 0; i < game.num_aliens; ++i)
    {
        death_counters[i] = 10;
    }

    uint32_t clear_color = rgb_to_uint32(0, 128, 0);

    game_running = true;

    int player_move_dir = 0;
    while (!glfwWindowShouldClose(window) && game_running)
    {
        buffer_clear(&buffer, clear_color);

        // Draw
		int text_size = 0;
		for (int i = 0; i < 5; i++)
		{
			text_size = 5 + i * (text[i].width + 1);
			buffer_draw_sprite(&buffer, text[i], text_size, buffer_height - 10, rgb_to_uint32(128, 0, 0));
		}

		string s = to_string(score);
		int len = s.length();
		for (int i = 0; i < len; i++) {
			Sprite scoreSprite = CreateTextSprite(s[i]);
			buffer_draw_sprite(&buffer, scoreSprite, text_size + 10 + i * (scoreSprite.width + 1), buffer_height - 10, rgb_to_uint32(128, 0, 0));
		}

        for (size_t ai = 0; ai < game.num_aliens; ++ai)
        {
            if (!death_counters[ai]) continue;

            const Alien& alien = game.aliens[ai];
            if (alien.type == ALIEN_DEAD)
            {
                buffer_draw_sprite(&buffer, alien_death_sprite, alien.x, alien.y, rgb_to_uint32(128, 0, 0));
            }
            else
            {
                const SpriteAnimation& animation = alien_animation[alien.type - 1];
                size_t current_frame = animation.time / animation.frame_duration;
                const Sprite& sprite = *animation.frames[current_frame];
                buffer_draw_sprite(&buffer, sprite, alien.x, alien.y, rgb_to_uint32(128, 0, 0));
            }
        }

        for (size_t bi = 0; bi < game.num_bullets; ++bi)
        {
            const Bullet& bullet = game.bullets[bi];
            const Sprite& sprite = bullet_sprite;
            buffer_draw_sprite(&buffer, sprite, bullet.x, bullet.y, rgb_to_uint32(128, 0, 0));
        }

        buffer_draw_sprite(&buffer, player_sprite, game.player.x, game.player.y, rgb_to_uint32(128, 0, 0));

        // Update animations
        for (size_t i = 0; i < 3; ++i)
        {
            ++alien_animation[i].time;
            if (alien_animation[i].time == alien_animation[i].num_frames * alien_animation[i].frame_duration)
            {
                alien_animation[i].time = 0;
            }
        }

        glTexSubImage2D(
            GL_TEXTURE_2D, 0, 0, 0,
            buffer.width, buffer.height,
            GL_RGBA, GL_UNSIGNED_INT_8_8_8_8,
            buffer.data
        );
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        glfwSwapBuffers(window);

        // Simulate aliens
        for (size_t ai = 0; ai < game.num_aliens; ++ai)
        {
            const Alien& alien = game.aliens[ai];
            if (alien.type == ALIEN_DEAD && death_counters[ai])
            {
                --death_counters[ai];
            }
        }

        // Simulate bullets
        for (size_t bi = 0; bi < game.num_bullets;)
        {
            game.bullets[bi].y += game.bullets[bi].dir;
            if (game.bullets[bi].y >= game.height || game.bullets[bi].y < bullet_sprite.height)
            {
                game.bullets[bi] = game.bullets[game.num_bullets - 1];
                --game.num_bullets;
                continue;
            }

            // Check hit
            for (size_t ai = 0; ai < game.num_aliens; ++ai)
            {
                const Alien& alien = game.aliens[ai];
                if (alien.type == ALIEN_DEAD) continue;

                const SpriteAnimation& animation = alien_animation[alien.type - 1];
                size_t current_frame = animation.time / animation.frame_duration;
                const Sprite& alien_sprite = *animation.frames[current_frame];
                bool overlap = sprite_overlap_check(
                    bullet_sprite, game.bullets[bi].x, game.bullets[bi].y,
                    alien_sprite, alien.x, alien.y
                );
                if (overlap)
                {
					score += ((4 - static_cast<int>(game.aliens[ai].type)) * 10);
                    game.aliens[ai].type = ALIEN_DEAD;
                    // NOTE: Hack to recenter death sprite
                    game.aliens[ai].x -= (alien_death_sprite.width - alien_sprite.width) / 2;
                    game.bullets[bi] = game.bullets[game.num_bullets - 1];
                    --game.num_bullets;
                    continue;
                }
            }

            ++bi;
        }

        // Simulate player
        player_move_dir = 2 * move_dir;

        if (player_move_dir != 0)
        {
            if (game.player.x + player_sprite.width + player_move_dir >= game.width)
            {
                game.player.x = game.width - player_sprite.width;
            }
            else if ((int)game.player.x + player_move_dir <= 0)
            {
                game.player.x = 0;
            }
            else game.player.x += player_move_dir;
        }

        // Process events
        if (fire_pressed && game.num_bullets < GAME_MAX_BULLETS)
        {
            game.bullets[game.num_bullets].x = game.player.x + player_sprite.width / 2;
            game.bullets[game.num_bullets].y = game.player.y + player_sprite.height;
            game.bullets[game.num_bullets].dir = 2;
            ++game.num_bullets;
        }
        fire_pressed = false;
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		if (score >= 990) {
			brightness -= 0.01f;  // Gradually darken the screen
			if (brightness < 0.3f) brightness = 0.3f;  // Clamp to 0.3
			glfwSetKeyCallback(window, NULL);
		}

		// Set the brightness uniform in the shader
		glUniform1f(brightnessLocation, brightness);

        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    glDeleteVertexArrays(1, &fullscreen_triangle_vao);

    for (size_t i = 0; i < 6; ++i)
    {
        delete[] alien_sprites[i].data;
    }

    delete[] alien_death_sprite.data;

    for (size_t i = 0; i < 3; ++i)
    {
        delete[] alien_animation[i].frames;
    }
    delete[] buffer.data;
    delete[] game.aliens;
    delete[] death_counters;

    return 0;
}

bool sprite_overlap_check(const Sprite& sp_a, size_t x_a, size_t y_a, const Sprite& sp_b, size_t x_b, size_t y_b)
{
	// NOTE: For simplicity we just check for overlap of the sprite
	// rectangles. Instead, if the rectangles overlap, we should
	// further check if any pixel of sprite A overlap with any of
	// sprite B.
	if (x_a < x_b + sp_b.width && x_a + sp_a.width > x_b &&
		y_a < y_b + sp_b.height && y_a + sp_a.height > y_b)
	{
		return true;
	}

	return false;
}

uint32_t rgb_to_uint32(uint8_t r, uint8_t g, uint8_t b) {
	return (r << 24) | (g << 16) | (b << 8) | 255;
}

void buffer_clear(Buffer* buffer, uint32_t color) {
	for (size_t i = 0; i < buffer->width * buffer->height; ++i)
	{
		buffer->data[i] = color;
	}
}

void buffer_draw_sprite(Buffer* buffer, const Sprite& sprite, size_t x, size_t y, uint32_t color)
{
	for (size_t xi = 0; xi < sprite.width; ++xi)
	{
		for (size_t yi = 0; yi < sprite.height; ++yi)
		{
			if (sprite.data[yi * sprite.width + xi] &&
				(sprite.height - 1 + y - yi) < buffer->height &&
				(x + xi) < buffer->width)
			{
				buffer->data[(sprite.height - 1 + y - yi) * buffer->width + (x + xi)] = color;
			}
		}
	}
}

void error_callback(int error, const char* description) {
	fprintf(stderr, "Error: %s\n", description);
}

void validate_shader(GLuint shader, const char* file) {
	static const unsigned int BUFFER_SIZE = 512;
	char buffer[BUFFER_SIZE];
	GLsizei length = 0;

	glGetShaderInfoLog(shader, BUFFER_SIZE, &length, buffer);

	if (length > 0) {
		printf("Shader %d(%s) compile error: %s\n", shader, (file ? file : ""), buffer);
	}
}

bool validate_program(GLuint program) {
	static const GLsizei BUFFER_SIZE = 512;
	GLchar buffer[BUFFER_SIZE];
	GLsizei length = 0;

	glGetProgramInfoLog(program, BUFFER_SIZE, &length, buffer);

	if (length > 0) {
		printf("Program %d link error: %s\n", program, buffer);
		return false;
	}

	return true;
}

Buffer CreateBuffer() {
	Buffer buffer;
	buffer.width = buffer_width;
	buffer.height = buffer_height;
	buffer.data = new uint32_t[buffer.width * buffer.height];
	return buffer;
}

void CreateTexture(GLuint &buffer_texture, Buffer buffer) {
	glGenTextures(1, &buffer_texture);
    glBindTexture(GL_TEXTURE_2D, buffer_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, buffer.width, buffer.height, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, buffer.data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

Sprite* CreateAlienSprites() {
    Sprite* alien_sprites = new Sprite[6];

	alien_sprites[0].width = 8;
	alien_sprites[0].height = 8;
	alien_sprites[0].data = new uint8_t[64]
	{
		0,0,0,1,1,0,0,0, // ...@@...
		0,0,1,1,1,1,0,0, // ..@@@@..
		0,1,1,1,1,1,1,0, // .@@@@@@.
		1,1,0,1,1,0,1,1, // @@.@@.@@
		1,1,1,1,1,1,1,1, // @@@@@@@@
		0,1,0,1,1,0,1,0, // .@.@@.@.
		1,0,0,0,0,0,0,1, // @......@
		0,1,0,0,0,0,1,0  // .@....@.
	};

	alien_sprites[1].width = 8;
	alien_sprites[1].height = 8;
	alien_sprites[1].data = new uint8_t[64]
	{
		0,0,0,1,1,0,0,0, // ...@@...
		0,0,1,1,1,1,0,0, // ..@@@@..
		0,1,1,1,1,1,1,0, // .@@@@@@.
		1,1,0,1,1,0,1,1, // @@.@@.@@
		1,1,1,1,1,1,1,1, // @@@@@@@@
		0,0,1,0,0,1,0,0, // ..@..@..
		0,1,0,1,1,0,1,0, // .@.@@.@.
		1,0,1,0,0,1,0,1  // @.@..@.@
	};

	alien_sprites[2].width = 11;
	alien_sprites[2].height = 8;
	alien_sprites[2].data = new uint8_t[88]
	{
		0,0,1,0,0,0,0,0,1,0,0, // ..@.....@..
		0,0,0,1,0,0,0,1,0,0,0, // ...@...@...
		0,0,1,1,1,1,1,1,1,0,0, // ..@@@@@@@..
		0,1,1,0,1,1,1,0,1,1,0, // .@@.@@@.@@.
		1,1,1,1,1,1,1,1,1,1,1, // @@@@@@@@@@@
		1,0,1,1,1,1,1,1,1,0,1, // @.@@@@@@@.@
		1,0,1,0,0,0,0,0,1,0,1, // @.@.....@.@
		0,0,0,1,1,0,1,1,0,0,0  // ...@@.@@...
	};

	alien_sprites[3].width = 11;
	alien_sprites[3].height = 8;
	alien_sprites[3].data = new uint8_t[88]
	{
		0,0,1,0,0,0,0,0,1,0,0, // ..@.....@..
		1,0,0,1,0,0,0,1,0,0,1, // @..@...@..@
		1,0,1,1,1,1,1,1,1,0,1, // @.@@@@@@@.@
		1,1,1,0,1,1,1,0,1,1,1, // @@@.@@@.@@@
		1,1,1,1,1,1,1,1,1,1,1, // @@@@@@@@@@@
		0,1,1,1,1,1,1,1,1,1,0, // .@@@@@@@@@.
		0,0,1,0,0,0,0,0,1,0,0, // ..@.....@..
		0,1,0,0,0,0,0,0,0,1,0  // .@.......@.
	};

	alien_sprites[4].width = 12;
	alien_sprites[4].height = 8;
	alien_sprites[4].data = new uint8_t[96]
	{
		0,0,0,0,1,1,1,1,0,0,0,0, // ....@@@@....
		0,1,1,1,1,1,1,1,1,1,1,0, // .@@@@@@@@@@.
		1,1,1,1,1,1,1,1,1,1,1,1, // @@@@@@@@@@@@
		1,1,1,0,0,1,1,0,0,1,1,1, // @@@..@@..@@@
		1,1,1,1,1,1,1,1,1,1,1,1, // @@@@@@@@@@@@
		0,0,0,1,1,0,0,1,1,0,0,0, // ...@@..@@...
		0,0,1,1,0,1,1,0,1,1,0,0, // ..@@.@@.@@..
		1,1,0,0,0,0,0,0,0,0,1,1  // @@........@@
	};


	alien_sprites[5].width = 12;
	alien_sprites[5].height = 8;
	alien_sprites[5].data = new uint8_t[96]
	{
		0,0,0,0,1,1,1,1,0,0,0,0, // ....@@@@....
		0,1,1,1,1,1,1,1,1,1,1,0, // .@@@@@@@@@@.
		1,1,1,1,1,1,1,1,1,1,1,1, // @@@@@@@@@@@@
		1,1,1,0,0,1,1,0,0,1,1,1, // @@@..@@..@@@
		1,1,1,1,1,1,1,1,1,1,1,1, // @@@@@@@@@@@@
		0,0,1,1,1,0,0,1,1,1,0,0, // ..@@@..@@@..
		0,1,1,0,0,1,1,0,0,1,1,0, // .@@..@@..@@.
		0,0,1,1,0,0,0,0,1,1,0,0  // ..@@....@@..
	};

	return alien_sprites;
}

Sprite CreateDeathSprite() {
	Sprite alien_death_sprite;
	alien_death_sprite.width = 13;
	alien_death_sprite.height = 7;
	alien_death_sprite.data = new uint8_t[91]
	{
		0,1,0,0,1,0,0,0,1,0,0,1,0, // .@..@...@..@.
		0,0,1,0,0,1,0,1,0,0,1,0,0, // ..@..@.@..@..
		0,0,0,1,0,0,0,0,0,1,0,0,0, // ...@.....@...
		1,1,0,0,0,0,0,0,0,0,0,1,1, // @@.........@@
		0,0,0,1,0,0,0,0,0,1,0,0,0, // ...@.....@...
		0,0,1,0,0,1,0,1,0,0,1,0,0, // ..@..@.@..@..
		0,1,0,0,1,0,0,0,1,0,0,1,0  // .@..@...@..@.
	};
	return alien_death_sprite;
}

Sprite CreatePlayer() {
	Sprite player_sprite;
	player_sprite.width = 11;
	player_sprite.height = 7;
	player_sprite.data = new uint8_t[player_sprite.width * player_sprite.height]
	{
		0,0,0,0,0,1,0,0,0,0,0, // .....@.....
		0,0,0,0,1,1,1,0,0,0,0, // ....@@@....
		0,0,0,0,1,1,1,0,0,0,0, // ....@@@....
		0,1,1,1,1,1,1,1,1,1,0, // .@@@@@@@@@.
		1,1,1,1,1,1,1,1,1,1,1, // @@@@@@@@@@@
		1,1,1,1,1,1,1,1,1,1,1, // @@@@@@@@@@@
		1,1,1,1,1,1,1,1,1,1,1, // @@@@@@@@@@@
	};
	return player_sprite;
}

Sprite CreateTextSprite(char letter) {
	Sprite textSprite;
	textSprite.width = 4;
	textSprite.height = 5;
	switch (letter) {
		case 'S':
			textSprite.data = new uint8_t[textSprite.width * textSprite.height]{
				0, 1, 1, 1,
				1, 0, 0, 0,
				0, 1, 1, 0,
				0, 0, 0, 1,
				1, 1, 1, 0
			};
		break;
		
		case 'C':
			textSprite.data = new uint8_t[textSprite.width * textSprite.height]{
				0, 1, 1, 1,
				1, 0, 0, 0,
				1, 0, 0, 0,
				1, 0, 0, 0,
				0, 1, 1, 1
			};
		break;

		case 'O':
			textSprite.data = new uint8_t[textSprite.width * textSprite.height]{
				0, 1, 1, 0,
				1, 0, 0, 1,
				1, 0, 0, 1,
				1, 0, 0, 1,
				0, 1, 1, 0
			};
		break;

		case 'R':
			textSprite.data = new uint8_t[textSprite.width * textSprite.height]{
				1, 1, 1, 0,
				1, 0, 0, 1,
				1, 1, 1, 0,
				1, 0, 1, 0,
				1, 0, 0, 1
			};
		break;

		case 'E':
			textSprite.data = new uint8_t[textSprite.width * textSprite.height]{
				1, 1, 1, 1,
				1, 0, 0, 0,
				1, 1, 1, 0,
				1, 0, 0, 0,
				1, 1, 1, 1
			};
		break;

		case '0':
			textSprite.data = new uint8_t[textSprite.width * textSprite.height]{
				1, 1, 1, 0,
				1, 0, 1, 0,
				1, 0, 1, 0,
				1, 0, 1, 0,
				1, 1, 1, 0
			};
		break;

		case '1':
			textSprite.data = new uint8_t[textSprite.width * textSprite.height]{
				0, 1, 0, 0,
				0, 1, 0, 0,
				0, 1, 0, 0,
				0, 1, 0, 0,
				0, 1, 0, 0
			};
		break;

		case '2':
			textSprite.data = new uint8_t[textSprite.width * textSprite.height]{
				1, 1, 1, 0,
				0, 0, 1, 0,
				1, 1, 0, 0,
				1, 0, 0, 0,
				1, 1, 1, 0
			};
		break;

		case '3':
			textSprite.data = new uint8_t[textSprite.width * textSprite.height]{
				1, 1, 1, 0,
				0, 0, 1, 0,
				0, 1, 0, 0,
				0, 0, 1, 0,
				1, 1, 1, 0
			};
		break;

		case '4':
			textSprite.data = new uint8_t[textSprite.width * textSprite.height]{
				1, 0, 1, 0,
				1, 0, 1, 0,
				1, 1, 1, 0,
				0, 0, 1, 0,
				0, 0, 1, 0
			};
			break;

		case '5':
			textSprite.data = new uint8_t[textSprite.width * textSprite.height]{
				0, 1, 1, 1,
				0, 1, 0, 0,
				0, 1, 1, 0,
				0, 0, 0, 1,
				0, 1, 1, 0
			};
			break;

		case '6':
			textSprite.data = new uint8_t[textSprite.width * textSprite.height]{
				1, 1, 1, 0,
				1, 0, 0, 0,
				1, 1, 1, 0,
				1, 0, 1, 0,
				1, 1, 1, 0
			};
			break;

		case '7':
			textSprite.data = new uint8_t[textSprite.width * textSprite.height]{
				1, 1, 1, 0,
				0, 0, 1, 0,
				0, 0, 1, 0,
				0, 0, 1, 0,
				0, 0, 1, 0
			};
			break;

		case '8':
			textSprite.data = new uint8_t[textSprite.width * textSprite.height]{
				1, 1, 1, 0,
				1, 0, 1, 0,
				1, 1, 1, 0,
				1, 0, 1, 0,
				1, 1, 1, 0
			};
			break;

		case '9':
			textSprite.data = new uint8_t[textSprite.width * textSprite.height]{
				1, 1, 1, 0,
				1, 0, 1, 0,
				1, 1, 1, 0,
				0, 0, 1, 0,
				0, 0, 1, 0
			};
			break;
	}

	return textSprite;
}

Game CreateGame() {
	Game game;
	game.width = buffer_width;
	game.height = buffer_height;
	game.num_bullets = 0;
	game.num_aliens = 55;
	game.aliens = new Alien[game.num_aliens];

	game.player.x = 112 - 5;
	game.player.y = 32;

	game.player.life = 3;
	return game;
}

Sprite CreateBullet() {
	Sprite bullet_sprite;
	bullet_sprite.width = 1;
	bullet_sprite.height = 3;
	bullet_sprite.data = new uint8_t[3]
	{
		1, // @
		1, // @
		1  // @
	};
	return bullet_sprite;
}

SpriteAnimation* CreateAnimation(Sprite* alien_sprites) {
	SpriteAnimation * alien_animation = new SpriteAnimation[3];

	for (size_t i = 0; i < 3; ++i)
	{
		alien_animation[i].loop = true;
		alien_animation[i].num_frames = 2;
		alien_animation[i].frame_duration = 10;
		alien_animation[i].time = 0;

		alien_animation[i].frames = new Sprite * [2];
		alien_animation[i].frames[0] = &alien_sprites[2 * i];
		alien_animation[i].frames[1] = &alien_sprites[2 * i + 1];
	};

	return alien_animation;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	switch (key) {
	case GLFW_KEY_ESCAPE:
		if (action == GLFW_PRESS) game_running = false;
		break;
	case GLFW_KEY_RIGHT:
		if (action == GLFW_PRESS) move_dir += 1;
		else if (action == GLFW_RELEASE) move_dir -= 1;
		break;
	case GLFW_KEY_LEFT:
		if (action == GLFW_PRESS) move_dir -= 1;
		else if (action == GLFW_RELEASE) move_dir += 1;
		break;
	case GLFW_KEY_SPACE:
		if (action == GLFW_RELEASE) fire_pressed = true;
		break;
	default:
		break;
	}
}