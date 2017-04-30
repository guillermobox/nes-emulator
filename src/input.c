#include <SDL/SDL.h>
#include <time.h>

typedef uint8_t byte;
extern byte gamepad_value;

void keychange(SDL_KeyboardEvent *key)
{
	byte mask = 0x00;

	switch (key->keysym.scancode) {
		case 0x34:
			mask = 0x01;
			break;
		case 0x35:
			mask = 0x02;
			break;
		case 0x41:
			mask = 0x04;
			break;
		case 0x24:
			mask = 0x08;
			break;
		case 0x6F:
			mask = 0x10;
			break;
		case 0x74:
			mask = 0x20;
			break;
		case 0x71:
			mask = 0x40;
			break;
		case 0x72:
			mask = 0x80;
			break;
	}

	if (key->type == SDL_KEYUP)
		gamepad_value &= ~mask;
	else
		gamepad_value |= mask;
};

void input_run()
{
	SDL_Event event;

	while (1) {
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_KEYDOWN: keychange(&event.key); break;
			case SDL_KEYUP: keychange(&event.key); break;
			}
		}
	}
}
