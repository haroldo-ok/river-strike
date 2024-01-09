#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "lib/SMSlib.h"
#include "lib/PSGlib.h"
#include "actor.h"
#include "map.h"
#include "score.h"
#include "data.h"

#define PLAYER_TOP (0)
#define PLAYER_LEFT (0)
#define PLAYER_RIGHT (256 - 16)
#define PLAYER_BOTTOM (SCREEN_H - 16)
#define PLAYER_SPEED (3)

#define TIMER_MAX (60)

actor player;
actor timer_label;
actor time_over;

score_display timer;
score_display score;

struct ply_ctl {
	char death_delay;
} ply_ctl;

char timer_delay;
char frames_elapsed;

void load_standard_palettes() {
	SMS_loadBGPalette(tileset_palette_bin);
	SMS_loadSpritePalette(sprites_palette_bin);
	SMS_setSpritePaletteColor(0, 0);
}

void update_score(actor *enm, actor *sht);

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

void update_score(actor *enm, actor *sht) {
	increment_score_display(&score, 5);
}

void init_score() {
	init_actor(&timer_label, 16, 8, 1, 1, 178, 1);
	init_score_display(&timer, 24, 8, 236);
	update_score_display(&timer, TIMER_MAX);
	timer_delay = 60;
	frames_elapsed = 0;
	
	init_score_display(&score, 16, 24, 236);
}

void handle_score() {
	if (timer_delay) {
		timer_delay--;
	} else {
		if (timer.value) {
			char decrement = frames_elapsed / 60;
			if (decrement > timer.value) decrement = timer.value;
			increment_score_display(&timer, -decrement);
		}
		timer_delay = 60;
		frames_elapsed = 0;
	}
}

void draw_score() {
	draw_actor(&timer_label);
	draw_score_display(&timer);

	draw_score_display(&score);
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

void gameplay_loop() {	
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
	
	init_actor(&player, 116, PLAYER_BOTTOM - 16, 3, 1, 2, 4);
	player.animation_delay = 20;
	ply_ctl.death_delay = 0;
	
	init_score();
	
	while (timer.value) {	
		handle_player_input();
		handle_score();
		
		SMS_initSprites();

		draw_player();
		draw_score();
		
		SMS_finalizeSprites();
		SMS_waitForVBlank();
		SMS_copySpritestoSAT();
		
		// Scroll two lines per frame
		draw_map();		
		draw_map();
	}
}

void timeover_sequence() {
	char timeover_delay = 128;
	char pressed_button = 0;
	
	init_actor(&time_over, 107, 64, 6, 1, 180, 1);

	while (timeover_delay || !pressed_button) {
		SMS_initSprites();

		if (!(timeover_delay & 0x10)) draw_actor(&time_over);
		
		draw_player();
		draw_score();
		
		SMS_finalizeSprites();
		SMS_waitForVBlank();
		SMS_copySpritestoSAT();
		
		draw_map();
		
		if (timeover_delay) {
			timeover_delay--;
		} else {
			pressed_button = SMS_getKeysStatus() & (PORT_A_KEY_1 | PORT_A_KEY_2);
		}
	}
	
	wait_button_release();
}

void main() {	
	while (1) {
		gameplay_loop();
		timeover_sequence();
	}
}

SMS_EMBED_SEGA_ROM_HEADER(9999,0); // code 9999 hopefully free, here this means 'homebrew'
SMS_EMBED_SDSC_HEADER(0,1, 2024,1,8, "Haroldo-OK\\2024", "River Strike",
  "A River Raid Clone.\n"
  "Originally made for the Minigame a Month - JANUARY 2024 - Water - https://itch.io/jam/minigame-a-month-january-2024\n"
  "Built using devkitSMS & SMSlib - https://github.com/sverx/devkitSMS");
