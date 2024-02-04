#ifndef MAP_H
#define MAP_H

void init_map(void *level_data);
void draw_map_screen();
void draw_map();

void get_margins(char *left, char *right, char x, char y);

void move_enemies();
void draw_enemies();

actor* find_colliding_enemy(actor *other);


#endif /* MAP_H */