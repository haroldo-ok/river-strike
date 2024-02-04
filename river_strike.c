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
#define PLAYER_SPEED (1)
#define PLAYER_MIN_SPEED (0x080)
#define PLAYER_MED_SPEED (0x100)
#define PLAYER_MAX_SPEED (0x250)
#define PLAYER_NEUTRAL_TILE (8)
#define PLAYER_CRASHING_TILE (PLAYER_NEUTRAL_TILE + 64)
#define PLAYER_DEATH_DELAY (90)
#define PLAYER_CRASHING_COUNTDOWN (60)

actor player;

struct ply_ctl {
	char crashing_countdown;
	char death_delay;
	fixed speed, displacement;
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

	ply_ctl.speed.w = PLAYER_MED_SPEED;
	if (joy & PORT_A_KEY_UP) {
		if (player.y > PLAYER_TOP) ply_ctl.speed.w = PLAYER_MAX_SPEED;
	} else if (joy & PORT_A_KEY_DOWN) {
		if (player.y < PLAYER_BOTTOM) ply_ctl.speed.w = PLAYER_MIN_SPEED;
	}
	
	if (ply_ctl.death_delay) ply_ctl.death_delay--;
}

void draw_player() {
	if (!(ply_ctl.death_delay & 0x08)) draw_actor(&player);
}

void check_player_enemy_collision() {
	actor *enm = find_colliding_enemy(&player);
	if (enm) ply_ctl.death_delay = PLAYER_DEATH_DELAY;
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
	SMS_addSprite(left, player.y + 8, 16);
	SMS_addSprite(right, player.y + 8, 16);]
	*/
	
	player.base_tile = PLAYER_NEUTRAL_TILE;
	if (left < px && right < px || left > px && right > px) {
		player.base_tile = PLAYER_CRASHING_TILE;
		if (!ply_ctl.death_delay) {
			if (ply_ctl.crashing_countdown) {
				ply_ctl.crashing_countdown--;
				if (!ply_ctl.death_delay) ply_ctl.death_delay = PLAYER_DEATH_DELAY;
			} else {
				ply_ctl.crashing_countdown = PLAYER_CRASHING_COUNTDOWN;
			}
		}
	} else {
		ply_ctl.crashing_countdown = 0;
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
	ply_ctl.crashing_countdown = 0;
	ply_ctl.death_delay = 0;
	ply_ctl.speed.w = PLAYER_MED_SPEED;
	ply_ctl.displacement.w = 0;
	
	while (1) {	
		handle_player_input();
		move_enemies();
		check_player_enemy_collision();

		SMS_initSprites();

		draw_collision();
		draw_player();
		draw_enemies();
		
		SMS_finalizeSprites();
		SMS_waitForVBlank();
		SMS_copySpritestoSAT();
		
		// Scroll map lines according to speed
		ply_ctl.displacement.w += ply_ctl.speed.w;
		for (char linesToScroll = ply_ctl.displacement.b.h; linesToScroll; linesToScroll--) {
			draw_map();		
		}
		ply_ctl.displacement.b.h = 0;
	}
}

void main() {	
	while (1) {
		gameplay_loop();
	}
}

SMS_EMBED_SEGA_ROM_HEADER(9999,0); // code 9999 hopefully free, here this means 'homebrew'
SMS_EMBED_SDSC_HEADER(0,3, 2024,2,2, "Haroldo-OK\\2024", "River Strike (Initial prototype)",
  "A River Raid Clone.\n"
  "Originally made for the Minigame a Month - JANUARY 2024 - Water - https://itch.io/jam/minigame-a-month-january-2024\n"
  "Vastly improved for the SMS Power Coding Competition 2024 - https://www.smspower.org/forums/19973-Competitions2024\n"
  "Built using devkitSMS & SMSlib - https://github.com/sverx/devkitSMS");
