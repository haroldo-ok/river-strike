#include <string.h>
#include "lib/SMSlib.h"
#include "actor.h"
#include "status.h"
#include "data.h"

#define FUEL_TILE_LEFT (22)

#define FUEL_TOP (24)
#define FUEL_LEFT (16)

#define FUEL_STEPS (48)
#define FUEL_MAX (FUEL_STEPS << 8)
#define FUEL_DECREMENT (0x06)

struct fuel_ctl {
	fixed amount, prev_amount;
	char dirty;
} fuel_ctl;

void init_fuel_gauge() {
	fuel_ctl.amount.w = FUEL_MAX;
	fuel_ctl.prev_amount.w = 0;
	fuel_ctl.dirty = 1;
}

void handle_fuel_gauge() {
	if (fuel_ctl.amount.b.h) {
		fuel_ctl.amount.w -= FUEL_DECREMENT;
	} else {
		fuel_ctl.amount.w = 0;
	}
	
	if (fuel_ctl.amount.b.h != fuel_ctl.prev_amount.b.h) fuel_ctl.dirty = 1;
	fuel_ctl.prev_amount.w = fuel_ctl.amount.w;
}

void draw_fuel_gauge() {
	static unsigned char masked_tiles[gauge_tiles_bin_size];
	
	if (fuel_ctl.dirty) {
		memcpy(masked_tiles, gauge_tiles_bin, gauge_tiles_bin_size);

		unsigned char *p = masked_tiles + (3 * 4);
		for (int count = FUEL_STEPS - ((int) fuel_ctl.amount.b.h) << 2; count; count--, p++) {
			*p = *p & 0x81;
		}
		
		SMS_loadTiles(masked_tiles, FUEL_TILE_LEFT, gauge_tiles_bin_size);
		
		fuel_ctl.dirty = 0;
	}
	
	for (char count = 4, y = FUEL_TOP, tile = FUEL_TILE_LEFT; count; count--, y += 16, tile += 2) {
		SMS_addSprite(FUEL_LEFT, y, tile);
	}
}
