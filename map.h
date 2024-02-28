#ifndef MAP_H
#define MAP_H

#define ENEMY_TILE_SHIP (130)
#define ENEMY_TILE_HELI (146)
#define ENEMY_TILE_PLANE (158)
#define ENEMY_TILE_FUEL (20)
#define ENEMY_TILE_BRIDGE (86)

void init_map();
void draw_map_screen();
void draw_map();

void get_margins(char *left, char *right, char x, char y);

void move_enemies();
void draw_enemies();

actor* find_colliding_enemy(actor *other);


#endif /* MAP_H */