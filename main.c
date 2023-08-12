#include <math.h>
#include <time.h>

#include <SDL.h>
#define GL_GLEXT_PROTOTYPES
#include <SDL_opengl.h>
#include <SDL_opengl_glext.h>

#include "hdrpix.h"

#define PI (3.141592653589793)
//#define NC (3)
#define MAXLUM (16)


struct globals {
	SDL_Window* window;
	int true_screen_width;
	int true_screen_height;
	int screen_width;
	int screen_height;
	float pixel_ratio;

	struct hdrpix hdrpix;
	struct hdrpix_shader_config hdrpix_shader_config;
} g;

static void populate_screen_globals()
{
	const int prev_width = g.true_screen_width;
	const int prev_height = g.true_screen_height;
	SDL_GL_GetDrawableSize(g.window, &g.true_screen_width, &g.true_screen_height);
	int w, h;
	SDL_GetWindowSize(g.window, &w, &h);
	g.pixel_ratio = (float)g.true_screen_width / (float)w;
	g.screen_width = g.true_screen_width / g.pixel_ratio;
	g.screen_height = g.true_screen_height / g.pixel_ratio;
	if (g.true_screen_width == prev_width && g.true_screen_height == prev_height) {
		return;
	}
	hdrpix_set_display_dimensions(&g.hdrpix, g.true_screen_width, g.true_screen_height);
}

int main(int argc, char** argv)
{
	assert(SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO) == 0);
	atexit(SDL_Quit);

	SDL_GLContext glctx;

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);

	g.window = SDL_CreateWindow(
			"hdrpix",
			SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
			1920, 1080,
			SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI);

	if (g.window == NULL) {
		fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
		abort();
	}

	glctx = SDL_GL_CreateContext(g.window);
	if (!glctx) {
		fprintf(stderr, "SDL_GL_CreateContextfailed: %s\n", SDL_GetError());
		abort();
	}

	printf("                 GL_VERSION:  %s\n", glGetString(GL_VERSION));
	printf("                  GL_VENDOR:  %s\n", glGetString(GL_VENDOR));
	printf("                GL_RENDERER:  %s\n", glGetString(GL_RENDERER));
	printf("GL_SHADING_LANGUAGE_VERSION:  %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

	hdrpix_init(&g.hdrpix, 0, 360, MAXLUM);

	populate_screen_globals();

	int exiting = 0;
	int fullscreen = 0;
	int iteration = 0;
	float theta = 0.0f;
	unsigned last_ticks = 0;

	memset(&g.hdrpix_shader_config, 0, sizeof g.hdrpix_shader_config);
	g.hdrpix_shader_config.shader = HDRPIX_SHADER_NOISY;

	while (!exiting) {
		SDL_Event e;
		while (SDL_PollEvent(&e)) {
			if (e.type == SDL_QUIT) {
				exiting = 1;
			} else if (e.type == SDL_KEYDOWN) {
				if (e.key.keysym.sym == SDLK_ESCAPE) {
					exiting = 1;
				} else if (e.key.keysym.sym == SDLK_f) {
					fullscreen = !fullscreen;
					//SDL_SetWindowFullscreen(g.window, fullscreen ? SDL_WINDOW_FULLSCREEN : 0);
					SDL_SetWindowFullscreen(g.window, fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
				}
			} else if (e.type == SDL_WINDOWEVENT) {
				if (e.window.event == SDL_WINDOWEVENT_RESIZED) {
					populate_screen_globals();
				}
			}
		}

		{
			unsigned char* p = g.hdrpix.canvas;
			const int w = g.hdrpix.canvas_width;
			const int h = g.hdrpix.canvas_height;
			for (int y = 0; y < h; y++) {
				for (int x = 0; x < w; x++) {
					const int B = 60;
					const int is_inside =
						   (B <= x && x < (w-B))
						&& (B <= y && y < (h-B));

					const float intensity = ((sinf(theta)+1.0f)*0.5f) * MAXLUM;
					p[0] = hdrpix_enc(MAXLUM, is_inside ? (x<w/2?1.0f : 0.0f) * intensity : 0.0f),
					p[1] = hdrpix_enc(MAXLUM, is_inside ? (x/2<w/3?0.2f : 0.0f) * intensity : 0.0f),
					p[2] = hdrpix_enc(MAXLUM, is_inside ? 0.1f * intensity : 0.5f),
					p+=3;
				}
			}
		}

		hdrpix_present(&g.hdrpix, &g.hdrpix_shader_config);

		SDL_GL_SwapWindow(g.window);

		theta += 0.02f;
		while (theta > 6.2830f) theta -= 6.2830f;
		iteration++;

		{
			const unsigned ticks = SDL_GetTicks();
			const unsigned dticks = ticks - last_ticks;
			const double fps = 1000.0 / (double)dticks;
			printf("%.1ffps\n", fps);
			last_ticks = ticks;
		}
	}

	SDL_GL_DeleteContext(glctx);
	SDL_DestroyWindow(g.window);

	return EXIT_SUCCESS;
}
