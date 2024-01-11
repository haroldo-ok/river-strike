#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "lib/SMSlib.h"
#include "lib/PSGlib.h"
#include "actor.h"
#include "map.h"

#define MAP_W (16)
#define STREAM_MIN_W (2)
#define STREAM_MAX_W (6)

struct map_data {
	char *level_data;
	char *next_row;
	char background_y;
	char lines_before_next;
	char scroll_y;
	
	char stream_x;
	char stream_w;
} map_data;

void init_map(void *level_data) {
	map_data.level_data = level_data;
	map_data.next_row = level_data;
	map_data.background_y = SCROLL_CHAR_H - 2;
	map_data.lines_before_next = 0;
	map_data.scroll_y = 0;
	
	map_data.stream_x = 7;
	map_data.stream_w = STREAM_MIN_W;
}

void generate_map_row(char *buffer) {
	static char *o, *d;
	static char remaining, ch, repeat, pos;
	
	d = buffer;
	for (remaining = MAP_W; remaining; remaining--) {
		*d = 17;
		d++;
	}
	
	d = buffer + map_data.stream_x;
	for (remaining = map_data.stream_w; remaining; remaining--) {
		*d = 4;
		d++;
	}

	if (!(rand() & 0x03)) {
		if (rand() & 0x80) {
			map_data.stream_w--;
		} else {
			map_data.stream_w++;
		}
		
		if (map_data.stream_w < STREAM_MIN_W) {
			map_data.stream_w = STREAM_MIN_W;
		} else if (map_data.stream_w > STREAM_MAX_W) {
			map_data.stream_w = STREAM_MAX_W;
		}
	}

	if (map_data.stream_w > STREAM_MIN_W && !(rand() & 0x03)) {
		if (rand() & 0x80) {
			map_data.stream_x--;
		} else {
			map_data.stream_x++;
		}		
	}

	if (map_data.stream_x < 1) {
		map_data.stream_x = 1;
	} else if (map_data.stream_x + map_data.stream_w > MAP_W - 1) {
		map_data.stream_x = MAP_W - map_data.stream_w - 1;
	}
}

void draw_map_row() {
	static char i, j;
	static char y;
	static char *map_char;
	static unsigned int base_tile, tile;
	static char buffer[16];

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
}
