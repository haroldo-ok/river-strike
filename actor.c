#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "lib/SMSlib.h"
#include "lib/PSGlib.h"
#include "actor.h"
#include "data.h"

void draw_meta_sprite(int x, int y, char w, char h, unsigned char tile) {
	int sy = y;
	for (char i = h; i; i--) {
		char st = tile;
		if (y >= 0 && y < SCREEN_H) {
			int sx = x;
			for (char j = w; j; j--) {
				if (sx >= 0 && sx < SCREEN_W) {
					SMS_addSprite(sx, sy, st);
				}
				sx += 8;
				st += 2;
			}
		}
		sy += 16;
		tile += 64;
	}
}

void init_actor(actor *act, int x, int y, int char_w, int char_h, unsigned char base_tile, unsigned char frame_count) {
	static actor *sa;
	sa = act;
	
	sa->active = 1;
	
	sa->x = x;
	sa->y = y;
	
	sa->spd_x = 0;
	sa->min_x = 0;
	sa->max_x = 0;
	
	sa->type = base_tile;
	sa->facing_left = 1;
	
	sa->char_w = char_w;
	sa->char_h = char_h;
	sa->pixel_w = char_w << 3;
	sa->pixel_h = char_h << 4;
	
	sa->animation_delay = 0;
	sa->animation_delay_max = 2;
	
	sa->base_tile = base_tile;
	sa->frame_count = frame_count;
	sa->frame = 0;
	sa->frame_increment = char_w * (char_h << 1);
	sa->frame_max = sa->frame_increment * frame_count;
	
	sa->col_w = sa->pixel_w - 4;
	sa->col_h = sa->pixel_h - 4;
	sa->col_x = (sa->pixel_w - sa->col_w) >> 1;
	sa->col_y = (sa->pixel_h - sa->col_h) >> 1;
	
	sa->state = 0;
	sa->state_timer = 256;
}

void move_actor(actor *_act) {
	static actor *act;
	static path_step *step, *curr_step;
	static char path_flags;
	
	if (!_act->active) {
		return;
	}
	
	act = _act;
	
	act->x += act->spd_x;
	if (act->min_x || act->max_x) {
		if (act->x < act->min_x) {
			act->x = act->min_x;
			act->spd_x = -act->spd_x;
		} else if (act->x > act->max_x) {
			act->x = act->max_x;
			act->spd_x = -act->spd_x;
		}
	}
		
	if (act->state_timer) act->state_timer--;
}

void draw_actor(actor *act) {
	if (!act->active) {
		return;
	}
	
	char frame_tile = act->base_tile + act->frame;
	if (!act->facing_left) {
		frame_tile += act->frame_max;
	}
	
	draw_meta_sprite(act->x, act->y, act->char_w, act->char_h, frame_tile);	

	if (act->animation_delay) {
		act->animation_delay--;
	} else {
		char frame = act->frame;		
		frame += act->frame_increment;
		if (frame >= act->frame_max) frame = 0;		
		act->frame = frame;

		act->animation_delay = act->animation_delay_max;
	}
}

void wait_frames(int wait_time) {
	for (; wait_time; wait_time--) SMS_waitForVBlank();
}

void clear_sprites() {
	SMS_initSprites();	
	SMS_finalizeSprites();
	SMS_copySpritestoSAT();
}

char is_touching(actor *act1, actor *act2) {
	static actor *collider1, *collider2;
	static int r1_tlx, r1_tly, r1_brx, r1_bry;
	static int r2_tlx, r2_tly, r2_bry;

	// Use global variables for speed
	collider1 = act1;
	collider2 = act2;
	
	// Collision on the Y axis
	
	r1_tly = collider1->y + collider1->col_y;
	r1_bry = r1_tly + collider1->col_h;
	r2_tly = collider2->y + collider2->col_y;
	
	// act1 is too far above
	if (r1_bry < r2_tly) {
		return 0;
	}
	
	r2_bry = r2_tly + collider2->col_h;
	
	// act1 is too far below
	if (r1_tly > r2_bry) {
		return 0;
	}
	
	// Collision on the X axis
	
	r1_tlx = collider1->x + collider1->col_x;
	r1_brx = r1_tlx + collider1->col_w;
	r2_tlx = collider2->x + collider2->col_x;
	
	// act1 is too far to the left
	if (r1_brx < r2_tlx) {
		return 0;
	}
	
	int r2_brx = r2_tlx + collider2->col_w;
	
	// act1 is too far to the left
	if (r1_tlx > r2_brx) {
		return 0;
	}
	
	return 1;
}
