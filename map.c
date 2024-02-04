#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "lib/SMSlib.h"
#include "lib/PSGlib.h"
#include "actor.h"
#include "map.h"

#define MAP_W (16)
#define STREAM_MIN_W (2)
#define STREAM_MAX_W (5)
#define TILE_WATER (4)
#define TILE_LAND (17)

#define ENEMY_TILE_SHIP (130)
#define ENEMY_TILE_HELI (138)
#define ENEMY_TILE_PLANE (144)

typedef struct river_stream {
	char x, w;
} river_stream;

struct map_data {
	char *level_data;
	char *next_row;
	char background_y;
	char lines_before_next;
	char scroll_y;

	river_stream stream1, stream2;
	char circular_buffer[SCROLL_CHAR_H][SCREEN_CHAR_W];
} map_data;

actor enemies[8];

void init_enemies() {
	FOR_EACH(actor *enm, enemies) {
		init_actor(enm, 0, 0, 2, 1, 8, 1);
		enm->active = 0;
	}
}

void move_enemies() {
	FOR_EACH(actor *enm, enemies) {
		move_actor(enm);
	}
}

void scroll_enemies() {
	FOR_EACH(actor *enm, enemies) {
		if (enm->active) {
			enm->y++;
			if (enm->y > SCREEN_H) enm->active = 0;
		}
	}
}

void draw_enemies() {
	FOR_EACH(actor *enm, enemies) {
		draw_actor(enm);
	}
}

actor* find_free_enemy() {
	FOR_EACH(actor *enm, enemies) {
		if (!enm->active) return enm;
	}	
	return 0;
}

void init_map(void *level_data) {
	map_data.level_data = level_data;
	map_data.next_row = level_data;
	map_data.background_y = SCROLL_CHAR_H - 2;
	map_data.lines_before_next = 0;
	map_data.scroll_y = 0;
	
	map_data.stream1.x = 7;
	map_data.stream1.w = STREAM_MIN_W;
	map_data.stream2.x = 7;
	map_data.stream2.w = STREAM_MIN_W;
}

void get_margins(char *left, char *right, char x, char y) {
	static char bg_x, bg_y;
	static char *row_data;

	bg_y = map_data.background_y + (y >> 3) + 2;
	if (bg_y > SCROLL_CHAR_H) bg_y -= SCROLL_CHAR_H;
	row_data = map_data.circular_buffer[bg_y >> 1];

	for (bg_x = (x >> 4); bg_x && row_data[bg_x] == TILE_WATER; bg_x--);
	*left = (bg_x << 4);

	for (bg_x = (x >> 4); bg_x < 16 && row_data[bg_x] == TILE_WATER; bg_x++);
	*right = (bg_x << 4);
}

void set_min_max_x_to_margins(actor *act) {
	char left, right;
	get_margins(&left, &right, act->x + 16, act->y + 8);
	
	act->min_x = left;
	act->max_x = right + 16 - act->pixel_w;
	if (act->max_x < act->min_x) {
		act->max_x = act->min_x + 16;
		act->spd_x = 0;
	}
}

void update_river_stream(char *buffer, river_stream *stream) {
	static char *d;
	static char remaining;

	// Fill the stream area with water tiles
	d = buffer + stream->x;
	for (remaining = stream->w; remaining; remaining--) {
		*d = TILE_WATER;
		d++;
	}

	// Update width
	if (!(rand() & 0x03)) {
		if (rand() & 0x80) {
			stream->w--;
		} else {
			stream->w++;
		}
		
		if (stream->w < STREAM_MIN_W) {
			stream->w = STREAM_MIN_W;
		} else if (stream->w > STREAM_MAX_W) {
			stream->w = STREAM_MAX_W;
		}
	}

	// Update X coord
	if (stream->w > STREAM_MIN_W && !(rand() & 0x03)) {
		if (rand() & 0x80) {
			stream->x--;
		} else {
			stream->x++;
		}		
	}

	if (stream->x < 2) {
		stream->x = 2;
	} else if (stream->x + stream->w > MAP_W - 1) {
		stream->x = MAP_W - stream->w - 1;
	}
}

void generate_map_row(char *buffer) {
	static char *o, *d, *prev, *next;
	static char remaining, ch, repeat, pos;
	
	// Fill the row with land tiles
	d = buffer;
	for (remaining = MAP_W; remaining; remaining--) {
		*d = TILE_LAND;
		d++;
	}
	
	update_river_stream(buffer, &map_data.stream1);
	update_river_stream(buffer, &map_data.stream2);
	
	prev = buffer;
	d = buffer + 1;
	next = buffer + 2;
	for (remaining = MAP_W - 2; remaining; remaining--) {
		// Handle vertical edges
		if (*prev == TILE_LAND && *d == TILE_WATER) *prev = 18;
		if (*d == TILE_WATER && *next == TILE_LAND) *next = 16;

		prev++;
		d++;
		next++;
	}
	
	if (!(rand() & 0x07)) {
		actor *enm = find_free_enemy();
		
		if (enm) {
			int enm_x = map_data.stream1.x << 4;
			switch (rand() & 0x03) {
			case 0:
				init_actor(enm, enm_x, 0, 4, 1, ENEMY_TILE_SHIP, 1);
				set_min_max_x_to_margins(enm);
				enm->spd_x = 1;
				break;
				
			case 1:
				init_actor(enm, enm_x, 0, 3, 1, ENEMY_TILE_HELI, 1);
				set_min_max_x_to_margins(enm);
				enm->spd_x = 1;
				break;
				
			case 2:
				init_actor(enm, 0, 0, 3, 1, ENEMY_TILE_PLANE, 1);
				enm->spd_x = 2;
				break;
			}
		}
	}
}

void draw_map_row() {
	static char i, j;
	static char y;
	static char *map_char;
	static unsigned int base_tile, tile;
	static char *buffer;
	
	buffer = map_data.circular_buffer[map_data.background_y >> 1];

	generate_map_row(buffer);

	for (i = 2, y = map_data.background_y, base_tile = 256; i; i--, y++, base_tile++) {
		SMS_setNextTileatXY(0, y);
		for (j = 16, map_char = buffer; j; j--, map_char++) {
			tile = base_tile + (*map_char << 2);
			SMS_setTile(tile);
			SMS_setTile(tile + 2);
		}
	}
	
	//map_data.next_row += 16;
	if (*map_data.next_row == 0xFF) {
		// Reached the end of the map; reset
		map_data.next_row = map_data.level_data;
	}
	
	if (map_data.background_y) {
		map_data.background_y -= 2;
	} else {
		map_data.background_y = SCROLL_CHAR_H - 2;
	}
	map_data.lines_before_next = 15;	
}

void draw_map_screen() {
	map_data.background_y = SCREEN_CHAR_H - 2;
	
	while (map_data.background_y < SCREEN_CHAR_H) {
		draw_map_row();
	}
	draw_map_row();
}

void draw_map() {
	if (map_data.lines_before_next) {
		map_data.lines_before_next--;
	} else {
		draw_map_row();
	}

	SMS_setBGScrollY(map_data.scroll_y);
	if (map_data.scroll_y) {
		map_data.scroll_y--;
	} else {
		map_data.scroll_y = SCROLL_H - 1;
	}

	scroll_enemies();
}
