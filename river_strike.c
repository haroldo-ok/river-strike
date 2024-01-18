#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "lib/SMSlib.h"
#include "lib/PSGlib.h"
#include "actor.h"
#include "map.h"
#include "data.h"

#define PLAYER_TOP (0)
#define PLAYER_LEFT (0)
#define PLAYER_RIGHT (256 - 16)
#define PLAYER_BOTTOM (SCREEN_H - 16)
#define PLAYER_SPEED (3)
#define PLAYER_NEUTRAL_TILE (8)
#define PLAYER_CRASHING_TILE (PLAYER_NEUTRAL_TILE + 64)

actor player;

struct ply_ctl {
	char death_delay;
} ply_ctl;

char frames_elapsed;

void load_standard_palettes() {
	SMS_loadBGPalette(tileset_palette_bin);
	SMS_loadSpritePalette(sprites_palette_bin);
	SMS_setSpritePaletteColor(0, 0);
}

void wait_button_press() {
	do {
		SMS_waitForVBlank();
	} while (!(SMS_getKeysStatus() & (PORT_A_KEY_1 | PORT_A_KEY_2)));
}

void wait_button_release() {
	do {
		SMS_waitForVBlank();
	} while (SMS_getKeysStatus() & (PORT_A_KEY_1 | PORT_A_KEY_2));
}

void handle_player_input() {
	static unsigned char joy;	
	joy = SMS_getKeysStatus();

	if (joy & PORT_A_KEY_LEFT) {
		if (player.x > PLAYER_LEFT) player.x -= PLAYER_SPEED;
	} else if (joy & PORT_A_KEY_RIGHT) {
		if (player.x < PLAYER_RIGHT) player.x += PLAYER_SPEED;
	}

	if (joy & PORT_A_KEY_UP) {
		if (player.y > PLAYER_TOP) player.y -= PLAYER_SPEED;
	} else if (joy & PORT_A_KEY_DOWN) {
		if (player.y < PLAYER_BOTTOM) player.y += PLAYER_SPEED;
	}
	
}

void draw_player() {
	if (!(ply_ctl.death_delay & 0x08)) draw_actor(&player);
}

char is_colliding_against_player(actor *_act) {
	static actor *act;
	static int act_x, act_y;
	
	act = _act;
	act_x = act->x;
	act_y = act->y;
	
	if (player.x > act_x - 12 && player.x < act_x + 12 &&
		player.y > act_y - 12 && player.y < act_y + 12) {
		return 1;
	}
	
	return 0;
}

void draw_background() {
	unsigned int *ch = background_tilemap_bin;
	
	SMS_setNextTileatXY(0, 0);
	for (char y = 0; y != 30; y++) {
		// Repeat pattern every two lines
		if (!(y & 0x01)) {
			ch = background_tilemap_bin;
		}
		
		for (char x = 0; x != 32; x++) {
			unsigned int tile_number = *ch + 256;
			SMS_setTile(tile_number);
			ch++;
		}
	}
}

void clear_tilemap() {
	SMS_setNextTileatXY(0, 0);
	for (int i = (SCREEN_CHAR_W * SCROLL_CHAR_H); i; i--) {
		SMS_setTile(0);
	}
}

void interrupt_handler() {
	PSGFrame();
	PSGSFXFrame();
	frames_elapsed++;
}

void draw_collision() {
	static char left, right;
	static char px;
	
	px = player.x + 8;
	get_margins(&left, &right, px, player.y + 8);
	
	/*	
	SMS_addSprite(left, player.y, 16);
	SMS_addSprite(right, player.y, 16);
	*/
	
	player.base_tile = PLAYER_NEUTRAL_TILE;
	if (left < px && right < px || left > px && right > px) {
		player.base_tile = PLAYER_CRASHING_TILE;
	}
}

void gameplay_loop() {	
	srand(1234);

	SMS_useFirstHalfTilesforSprites(1);
	SMS_setSpriteMode(SPRITEMODE_TALL);
	SMS_VDPturnOnFeature(VDPFEATURE_HIDEFIRSTCOL);

	SMS_displayOff();
	SMS_loadPSGaidencompressedTiles(sprites_tiles_psgcompr, 0);
	SMS_loadPSGaidencompressedTiles(tileset_tiles_psgcompr, 256);
	load_standard_palettes();
	
	init_map(level1_bin);
	draw_map_screen();

	SMS_setLineInterruptHandler(&interrupt_handler);
	SMS_setLineCounter(180);
	SMS_enableLineInterrupt();

	SMS_displayOn();
	
	init_actor(&player, 116, PLAYER_BOTTOM - 16, 2, 1, PLAYER_NEUTRAL_TILE, 1);
	player.animation_delay = 20;
	ply_ctl.death_delay = 0;
	
	while (1) {	
		handle_player_input();
		
		SMS_initSprites();

		draw_collision();
		draw_player();
		draw_enemies();
		
		SMS_finalizeSprites();
		SMS_waitForVBlank();
		SMS_copySpritestoSAT();
		
		// Scroll two lines per frame
		draw_map();		
		draw_map();		
	}
}

void main() {	
	while (1) {
		gameplay_loop();
	}
}

SMS_EMBED_SEGA_ROM_HEADER(9999,0); // code 9999 hopefully free, here this means 'homebrew'
SMS_EMBED_SDSC_HEADER(0,1, 2024,1,12, "Haroldo-OK\\2024", "River Strike",
  "A River Raid Clone.\n"
  "Originally made for the Minigame a Month - JANUARY 2024 - Water - https://itch.io/jam/minigame-a-month-january-2024\n"
  "Built using devkitSMS & SMSlib - https://github.com/sverx/devkitSMS");
