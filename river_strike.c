#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "lib/SMSlib.h"
#include "lib/PSGlib.h"
#include "actor.h"
#include "shot.h"
#include "shots.h"
#include "map.h"
#include "score.h"
#include "data.h"

#define PLAYER_TOP (0)
#define PLAYER_LEFT (0)
#define PLAYER_RIGHT (256 - 16)
#define PLAYER_BOTTOM (SCREEN_H - 16)
#define PLAYER_SPEED (3)

#define POWERUP_BASE_TILE (100)
#define POWERUP_LIGHTINING_TILE (POWERUP_BASE_TILE)
#define POWERUP_FIRE_TILE (POWERUP_BASE_TILE + 8)
#define POWERUP_WIND_TILE (POWERUP_BASE_TILE + 16)
#define POWERUP_NONE_TILE (POWERUP_BASE_TILE + 24)
#define POWERUP_LIGHTINING (1)
#define POWERUP_FIRE (2)
#define POWERUP_WIND (3)

#define POWERUP_MAX (3)

#define TIMER_MAX (60)

actor player;
actor icons[2];
actor powerups[POWERUP_MAX];
actor timer_label;
actor time_over;

score_display timer;
score_display score;

struct ply_ctl {
	char shot_delay;
	char shot_type;
	char pressed_shot_selection;
	
	char powerup1, powerup2;
	char powerup1_active, powerup2_active;

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

void select_combined_powerup() {
	switch (ply_ctl.powerup1) {
	case POWERUP_LIGHTINING:
		switch (ply_ctl.powerup2) {
		case POWERUP_LIGHTINING: ply_ctl.shot_type = 3; break; // Thunderstrike
		case POWERUP_FIRE: ply_ctl.shot_type = 6; break; // Firebolt
		case POWERUP_WIND: ply_ctl.shot_type = 7; break; // Thunderstorm
		}
		break;
	
	case POWERUP_FIRE:
		switch (ply_ctl.powerup2) {
		case POWERUP_LIGHTINING: ply_ctl.shot_type = 6; break; // Firebolt
		case POWERUP_FIRE: ply_ctl.shot_type = 4; break; // Hellfire
		case POWERUP_WIND: ply_ctl.shot_type = 8; break; // Firestorm
		}
		break;

	case POWERUP_WIND:
		switch (ply_ctl.powerup2) {
		case POWERUP_LIGHTINING: ply_ctl.shot_type = 7; break; // Thunderstorm
		case POWERUP_FIRE: ply_ctl.shot_type = 8; break; // Firestorm
		case POWERUP_WIND: ply_ctl.shot_type = 5; break; // Tempest
		}
		break;

	}
}

void switch_powerup() {
	if (ply_ctl.powerup1_active && ply_ctl.powerup2_active) {
		// Only the first powerup will be active
		ply_ctl.powerup1_active = 1;
		ply_ctl.powerup2_active = 0;
		ply_ctl.shot_type = ply_ctl.powerup1 - 1;
	} else if (ply_ctl.powerup1_active) {
		// Only the second powerup will be active
		ply_ctl.powerup1_active = 0;
		ply_ctl.powerup2_active = 1;
		ply_ctl.shot_type = ply_ctl.powerup2 - 1;
	} else {
		// Both powerups will be active
		ply_ctl.powerup1_active = 1;
		ply_ctl.powerup2_active = 1;
		select_combined_powerup();
	}
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
	
	if (joy & PORT_A_KEY_2) {
		if (!ply_ctl.shot_delay) {
			if (fire_player_shot(&player, ply_ctl.shot_type)) {
				ply_ctl.shot_delay = player_shot_infos[ply_ctl.shot_type].firing_delay;
			}
		}
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

void init_powerups() {
	static actor *pwr;

	init_actor(icons, 256 - 32 - 8, 8, 2, 1, POWERUP_LIGHTINING_TILE, 1);	
	init_actor(icons + 1, 256 - 16 - 8, 8, 2, 1, POWERUP_FIRE_TILE, 1);	

	FOR_EACH(pwr, powerups) {
		init_actor(pwr, 0, 0, 2, 1, POWERUP_LIGHTINING_TILE, 2);
		pwr->active = 0;
	}
}

char powerup_base_tile(char type) {
	switch (type) {
	case POWERUP_LIGHTINING: return POWERUP_LIGHTINING_TILE;
	case POWERUP_FIRE: return POWERUP_FIRE_TILE;
	case POWERUP_WIND: return POWERUP_WIND_TILE;
	}
	
	return POWERUP_NONE_TILE;
}

void handle_icons() {
	static int tile;
	
	tile = powerup_base_tile(ply_ctl.powerup1);
	if (!ply_ctl.powerup1_active) tile += 4;
	icons[0].base_tile = tile;
	
	tile = powerup_base_tile(ply_ctl.powerup2);
	if (ply_ctl.powerup2 && !ply_ctl.powerup2_active) tile += 4;
	icons[1].base_tile = tile;
}

void spawn_powerup(char x, char type) {
	static actor *pwr, *selected;

	selected = 0;
	FOR_EACH(pwr, powerups) {
		if (!pwr->active) selected = pwr;
	}
	
	if (!selected) {
		selected = pwr;
		FOR_EACH(pwr, powerups) {
			if (pwr->y > selected->y) selected = pwr;
		}		
	}

	if (selected) {
		selected->x = x;
		selected->y = -16;
		selected->active = 1;
		selected->state = type;
		selected->base_tile = powerup_base_tile(selected->state);
	}
}

void handle_powerups() {
	static actor *pwr;

	FOR_EACH(pwr, powerups) {	
		pwr->y++;
		if (pwr->y > SCREEN_H) pwr->active = 0;

		if (pwr->active) {
			// Check collision with player
			if (pwr->x > player.x - 16 && pwr->x < player.x + 24 &&
				pwr->y > player.y - 16 && pwr->y < player.y + 16) {
				update_score(pwr, 0);

				if (!ply_ctl.powerup2) {
					// Second is absent
					ply_ctl.powerup2 = pwr->state;
				} else  if (!ply_ctl.powerup1_active) {
					// First is inactive
					ply_ctl.powerup1 = pwr->state;
				} else if (!ply_ctl.powerup2_active) {
					// Second is inactive
					ply_ctl.powerup2 = pwr->state;
				} else {
					// Both are active
					ply_ctl.powerup1 = ply_ctl.powerup2;
					ply_ctl.powerup2 = pwr->state;				
				}
				
				ply_ctl.powerup1_active = 1;
				ply_ctl.powerup2_active = 1;
				select_combined_powerup();
				
				pwr->active = 0;			
			}
		}	
	}
}

void draw_powerups() {
	static actor *pwr;

	draw_actor(icons);
	draw_actor(icons + 1);		

	FOR_EACH(pwr, powerups) {	
		draw_actor(pwr);
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
	ply_ctl.shot_delay = 0;
	ply_ctl.shot_type = 0;
	ply_ctl.powerup1 = 1;
	ply_ctl.powerup2 = 0;
	ply_ctl.powerup1_active = 1;
	ply_ctl.powerup2_active = 0;
	ply_ctl.death_delay = 0;
	
	init_player_shots();
	init_powerups();
	init_score();
	
	while (timer.value) {	
		handle_player_input();
		handle_icons();
		handle_powerups();
		handle_player_shots();
		handle_score();
		
		SMS_initSprites();

		draw_player();
		draw_powerups();
		draw_player_shots();
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
		draw_player_shots();
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
