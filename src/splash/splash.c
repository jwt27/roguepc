#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL.h>

#define BSAVE_HEADER    7  // BSAVE header is 7 bytes in rogue.pic

#define CGA_WIDTH     320
#define CGA_HEIGHT    200

static const int CGA_COLORS[4][3] = {
	{0,     0,   0},  // Black
	{ 85, 255, 255},  // Light Cyan
	{255,  85, 255},  // Light Magenta
	{255, 255, 255}   // White
};

static const char * const PICFILE = "../rogue.pic";
static const char * PROGNAME = NULL;  // == argv[0], set by main()


void usage(FILE* stream)
{
	fprintf(stream, "Usage: %s [-h|--help] [PICFILE]\n", PROGNAME);
}


// explain_output_error_and_die() using stdlib only
_Noreturn void fatal(const char *fmt, ...)
{
	char msg[1000];

	va_list argp;
	va_start(argp, fmt);
	vsnprintf(msg, sizeof(msg), fmt, argp);
	va_end(argp);

	if (errno)
		// just like perror()!
		fprintf(stderr, "%s: %s: %s\n", PROGNAME, strerror(errno), msg);
	else
		fprintf(stderr, "%s: %s\n", PROGNAME, msg);
	usage(stderr);
	exit(EXIT_FAILURE);
}


int main(int argc, char* argv[])
{
	char* arg;
	const char* path = NULL;
	FILE* file = NULL;
	/*
	 * 0x4000 = 16384 bytes, the file size minus 7-byte header
	 * Oh, the wonders of modern platforms and megabytes of stack space!
	 */
	unsigned char data[0x4000];

	SDL_Window*   window   = NULL;
	SDL_Renderer* renderer = NULL;
	SDL_Event     event;

	PROGNAME = argv[0];

	while (--argc) {
		arg = *(++argv);
		if (!strcmp("-h", arg) || !strcmp("--help", arg)) {
			printf("Display a 320x200 PIC image in BSAVE format using SDL\n");
			printf("With CGA colors, palette 1i (Black / Cyan / Magenta / White)\n");
			printf("Default image path: %s\n", PICFILE);
			usage(stdout);
			return 0;
		}
		if (!strcmp("--", arg)) {
			argc--;
			break;
		}
		if (arg[0] == '-')
			fatal("invalid option: %s", arg);
		if (path)
			fatal("too many arguments: %s", arg);
		path = arg;
	}
	if (!path) {
		if (argc) {
			path = *(++argv);
			argc--;
		}
		else
			path = PICFILE;
	}
	if (argc)
		fatal("too many arguments: %s", *(++argv));

	assert(!argc && path);

	if ((file = fopen(path, "rb")) == NULL)
		fatal("%s", path);  // rely on strerror(errno) built-in message

	// Read the file data, discarding the header
	if (!fread(data, BSAVE_HEADER, 1, file) ||
	    !fread(data, sizeof(data), 1, file))
	{
		fclose(file);
		fatal("error reading file: %s", path);  // no errno set for EOF :(
	}
	fclose(file);

	if (
			SDL_Init(SDL_INIT_VIDEO) < 0 ||
			SDL_CreateWindowAndRenderer(0, 0,
				SDL_WINDOW_FULLSCREEN_DESKTOP,
				&window, &renderer) < 0
	) {
		SDL_Quit();
		fatal("could not initialize SDL: %s", SDL_GetError());
	}
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");  // Default is "nearest"
	SDL_RenderSetLogicalSize(renderer, CGA_WIDTH, CGA_HEIGHT);
	SDL_SetRenderDrawColor(renderer, 255, 255, 0, SDL_ALPHA_OPAQUE);  // Yellow
	SDL_RenderClear(renderer);  // Clear the screen
	SDL_Delay(100);  // let it breath to avoid initial garbage screen
	SDL_RenderPresent(renderer);
	SDL_Delay(50);  // ditto

	// Ready for the (X, Y, Color) loop
	int x=160, y=100, c=2;
	SDL_SetRenderDrawColor(
		renderer,
		CGA_COLORS[c][0],
		CGA_COLORS[c][1],
		CGA_COLORS[c][2],
		SDL_ALPHA_OPAQUE
	);
	SDL_RenderDrawPoint(renderer, x, y);

	SDL_RenderPresent(renderer);

	// Loop until key or mouse press
	while (1) {
		if (SDL_PollEvent(&event) && (
				event.type == SDL_QUIT ||
				event.type == SDL_KEYUP ||
				event.type == SDL_MOUSEBUTTONUP
		))
			break;
		SDL_Delay(17);  // ~60FPS
	}
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
}
