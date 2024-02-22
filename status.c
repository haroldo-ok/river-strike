#include "lib/SMSlib.h"
#include "actor.h"
#include "status.h"
#include "data.h"

#define FUEL_TILE_LEFT (22)

#define FUEL_TOP (24)
#define FUEL_LEFT (16)

void draw_fuel_gauge() {
	static unsigned char masked_tiles[gauge_tiles_bin_size];
	
	int count;
	unsigned char *o = gauge_tiles_bin;
	unsigned char *d = masked_tiles;
	for (count = gauge_tiles_bin_size; count; count--, o++, d++) {
		*d = *o & 0x81;
	}
	SMS_loadTiles (masked_tiles, FUEL_TILE_LEFT, gauge_tiles_bin_size);
	
	for (char count = 4, y = FUEL_TOP, tile = FUEL_TILE_LEFT; count; count--, y += 16, tile += 2) {
		SMS_addSprite(FUEL_LEFT, y, tile);
	}
}