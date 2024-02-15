#include "lib/SMSlib.h"
#include "actor.h"
#include "status.h"

#define FUEL_TILE_LEFT (22)
#define FUEL_TILE_NEEDLE (24)
#define FUEL_TILE_RIGHT (26)

#define FUEL_TOP (SCREEN_H - 18)
#define FUEL_LEFT (128 - 32 + 8)
#define FUEL_RIGHT (128 + 32)

void draw_fuel_gauge() {
	SMS_addSprite(FUEL_LEFT, FUEL_TOP, FUEL_TILE_LEFT);
	SMS_addSprite(FUEL_RIGHT, FUEL_TOP, FUEL_TILE_RIGHT);	
	SMS_addSprite(FUEL_LEFT + 16, FUEL_TOP, FUEL_TILE_NEEDLE);
}