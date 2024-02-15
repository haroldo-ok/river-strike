#include "lib/SMSlib.h"
#include "actor.h"
#include "status.h"

#define FUEL_TILE_LEFT (22)

#define FUEL_TOP (24)
#define FUEL_LEFT (16)

void draw_fuel_gauge() {
	for (char count = 4, y = FUEL_TOP, tile = FUEL_TILE_LEFT; count; count--, y += 16, tile += 2) {
		SMS_addSprite(FUEL_LEFT, y, tile);
	}
}