#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "lib/SMSlib.h"
#include "lib/PSGlib.h"
#include "actor.h"
#include "map.h"
#include "status.h"
#include "score.h"
#include "data.h"

#define PLAYER_TOP (0)
#define PLAYER_LEFT (0)
#define PLAYER_RIGHT (256 - 16)
#define PLAYER_BOTTOM (SCREEN_H - 16)
#define PLAYER_SPEED (1)
#define PLAYER_MIN_SPEED (0x080)
#define PLAYER_MED_SPEED (0x100)
#define PLAYER_MAX_SPEED (0x250)
#define PLAYER_SHOT_SPEED (5)
#define PLAYER_NEUTRAL_TILE (8)
#define PLAYER_CRASHING_TILE (PLAYER_NEUTRAL_TILE + 64)
#define PLAYER_SHOT_TILE (18)
#define PLAYER_DEATH_DELAY (2)
#define PLAYER_CRASHING_COUNTDOWN (60)

#define SCORE_TILE (192)

actor player;
actor shot;

score_display score;
score_display life;

struct ply_ctl {
	char crashing_countdown;
	char death_delay;
	fixed speed, displacement, prev_speed;
	char was_refuelling;
	int level_number;
} ply_ctl;

char frames_elapsed;

char engine_sound_countdown;

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

void kill_player() {
	ply_ctl.death_delay = PLAYER_DEATH_DELAY;

	PSGSFXPlay(explosion_psg, SFX_CHANNELS2AND3);
	engine_sound_countdown = 32;
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
		ply_ctl.speed.w = PLAYER_MAX_SPEED;
	} else if (joy & PORT_A_KEY_DOWN) {
		ply_ctl.speed.w = PLAYER_MIN_SPEED;
	}
	
	if (joy & (PORT_A_KEY_1 | PORT_A_KEY_2)) {
		if (!shot.active) {
			shot.x = player.x + 4;
			shot.y = player.y - 12;
			shot.active = 1;
			
			PSGPlayNoRepeat(shot_psg);
		}
	}
	
	if (!ply_ctl.death_delay && is_fuel_empty()) {
		kill_player();
	}

	if (ply_ctl.death_delay) {
		if (ply_ctl.death_delay == 1) player.active = 0;
		ply_ctl.death_delay--;
	}	
}

void draw_player() {
	draw_actor(&player);
}

void move_shot() {
	shot.y -= PLAYER_SHOT_SPEED;
	if (shot.y < 0) shot.active = 0;
}

void draw_shot() {
	draw_actor(&shot);
}

void check_player_enemy_collision() {
	actor *enm = find_colliding_enemy(&player);
	if (!enm) {
		if (ply_ctl.was_refuelling) PSGStop();
		ply_ctl.was_refuelling = 0;
		return;
	}
	
	if (enm->type == ENEMY_TILE_FUEL) {
		// Refuel player
		increase_fuel_gauge();
		
		if (!ply_ctl.was_refuelling) PSGPlayNoRepeat(fueling_psg);
		ply_ctl.was_refuelling = 1;
	} else {
		// Other enemies kill the player
		kill_player();
	}
}

void check_shot_enemy_collision() {
	if (!shot.active) return;
	
	actor *enm = find_colliding_enemy(&shot);
	if (enm) {
		enm->active = 0;
		shot.active = 0;
		
		increment_score_display(&score, 5);
		
		PSGSFXPlay(explosion_psg, SFX_CHANNELS2AND3);
		engine_sound_countdown = 32;
		
		// Destroyed a bridge: update level counter
		if (enm->type == ENEMY_TILE_BRIDGE) ply_ctl.level_number = get_level_number();
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
				if (!ply_ctl.death_delay) kill_player();
			} else {
				ply_ctl.crashing_countdown = PLAYER_CRASHING_COUNTDOWN;
			}
		}
	} else {
		ply_ctl.crashing_countdown = 0;
	}
}

void init_life_counter() {
	init_score_display(&life, 26, PLAYER_BOTTOM - 30, SCORE_TILE);
	update_score_display(&life, 3);
}

void decrement_life_counter() {
	increment_score_display(&life, -1);
}

void draw_life_counter() {
	SMS_addSprite(16, PLAYER_BOTTOM - 32, 212);	
	draw_score_display(&life);
}

void title_screen() {
	PSGStop();
	PSGSFXStop();
	
	SMS_displayOff();

	SMS_useFirstHalfTilesforSprites(1);
	SMS_setSpriteMode(SPRITEMODE_TALL);
	SMS_VDPturnOffFeature(VDPFEATURE_HIDEFIRSTCOL);

	SMS_setBGScrollX(0);
	SMS_setBGScrollY(0);

	SMS_loadPSGaidencompressedTiles(title_logo_tiles_psgcompr, 0);
	SMS_loadBGPalette(title_logo_palette_bin);
	SMS_loadTileMapArea(0, 0, title_logo_tilemap_bin, SCREEN_CHAR_W, 7);

	SMS_loadPSGaidencompressedTiles(title_image_tiles_psgcompr, 180);
	SMS_loadSpritePalette(title_image_palette_bin);
	unsigned int *t = title_image_tilemap_bin;
	for (char y = 7; y != SCREEN_CHAR_H; y++) {
		SMS_setNextTileatXY(0, y);
		for (char x = 0; x != SCREEN_CHAR_W; x++) {
			SMS_setTile(*t + 180 + TILE_USE_SPRITE_PALETTE);
			t++;
		}
	}

	clear_sprites();

	SMS_displayOn();
	
	wait_button_press();
	wait_button_release();
}

void gameplay_loop() {	
	SMS_displayOff();

	SMS_useFirstHalfTilesforSprites(1);
	SMS_setSpriteMode(SPRITEMODE_TALL);
	SMS_VDPturnOnFeature(VDPFEATURE_HIDEFIRSTCOL);

	SMS_loadPSGaidencompressedTiles(sprites_tiles_psgcompr, 0);
	SMS_loadPSGaidencompressedTiles(tileset_tiles_psgcompr, 256);
	load_standard_palettes();
	
	SMS_setLineInterruptHandler(&interrupt_handler);
	SMS_setLineCounter(180);
	SMS_enableLineInterrupt();

	SMS_displayOn();

	ply_ctl.level_number = 1;
	init_life_counter();
	init_score_display(&score, 16, PLAYER_BOTTOM + 2, SCORE_TILE);
	
	while (life.value) {
		SMS_displayOff();

		init_map(ply_ctl.level_number);
		draw_map_screen();

		SMS_displayOn();

		init_actor(&player, 116, PLAYER_BOTTOM - 16, 2, 1, PLAYER_NEUTRAL_TILE, 1);
		player.animation_delay = 20;
		ply_ctl.crashing_countdown = 0;
		ply_ctl.death_delay = 0;
		ply_ctl.speed.w = PLAYER_MED_SPEED;
		ply_ctl.displacement.w = 0;
		ply_ctl.prev_speed.w = 0;
		ply_ctl.was_refuelling = 0;
		
		init_actor(&shot, 0, 0, 1, 1, PLAYER_SHOT_TILE, 1);
		shot.active = 0;
		
		init_enemies();
		init_fuel_gauge();
		
		engine_sound_countdown = 0;
		
		while (player.active) {	
			handle_player_input();
			move_shot();
			move_enemies();
			check_player_enemy_collision();
			check_shot_enemy_collision();
			
			handle_fuel_gauge();

			if (ply_ctl.prev_speed.w != ply_ctl.speed.w) {
				ply_ctl.prev_speed.w = ply_ctl.speed.w;
				engine_sound_countdown = 0;
			}
			
			if (engine_sound_countdown) {
				engine_sound_countdown--;
			} else {
				PSGSFXPlay(
					ply_ctl.speed.w == PLAYER_MAX_SPEED ? engine_fast_psg :
					ply_ctl.speed.w == PLAYER_MIN_SPEED ? engine_slow_psg :
					engine_normal_psg, 
					SFX_CHANNELS2AND3);
				engine_sound_countdown = 32;
			}

			SMS_initSprites();

			draw_collision();
			draw_player();
			draw_enemies();
			draw_shot();
			
			draw_fuel_gauge();
			draw_score_display(&score);
			draw_life_counter();
			
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
		
		wait_frames(60);		
		decrement_life_counter();
	}
}

void main() {	
	while (1) {
		title_screen();
		gameplay_loop();
	}
}

SMS_EMBED_SEGA_ROM_HEADER(9999,0); // code 9999 hopefully free, here this means 'homebrew'
SMS_EMBED_SDSC_HEADER(0,8, 2024,4,1, "Haroldo-OK\\2024", "River Strike (Initial prototype)",
  "A River Raid Clone.\n"
  "Originally made for the Minigame a Month - JANUARY 2024 - Water - https://itch.io/jam/minigame-a-month-january-2024\n"
  "Vastly improved for the SMS Power Coding Competition 2024 - https://www.smspower.org/forums/19973-Competitions2024\n"
  "Built using devkitSMS & SMSlib - https://github.com/sverx/devkitSMS");
