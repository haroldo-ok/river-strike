#ifndef ACTOR_H
#define ACTOR_H

#define SCREEN_W (256)
#define SCREEN_H (192)
#define SCROLL_H (224)
#define SCREEN_CHAR_W (32)
#define SCREEN_CHAR_H (24)
#define SCROLL_CHAR_H (28)

#define PATH_FLIP_X (0x01)
#define PATH_FLIP_Y (0x02)
#define PATH_2X_SPEED (0x04)

// Based on https://stackoverflow.com/a/400970/679240
#define FOR_EACH(item, array) \
    for(int keep = 1, \
            count = 0,\
            size = sizeof (array) / sizeof *(array); \
        keep && count != size; \
        keep = !keep, count++) \
      for(item = (array) + count; keep; keep = !keep)
		  
typedef union _fixed {
  struct {
    unsigned char l;
    signed char h;
  } b;
  int w;
} fixed;

typedef struct _path_step {
	signed char x, y;
} path_step;

typedef struct _path {
	signed char x, y;
	char flags;
	path_step *steps;
} path;

typedef struct actor {
	char active;
	
	int x, y;

	int spd_x;
	int min_x, max_x;
	
	char facing_left;
	
	char char_w, char_h;
	char pixel_w, pixel_h;
	
	char type;
	
	unsigned char animation_delay, animation_delay_max;
	unsigned char base_tile, frame_count;
	unsigned char frame, frame_increment, frame_max;
	
	unsigned char state;
	int state_timer;
	
	char col_x, col_y, col_w, col_h;
} actor;

void draw_meta_sprite(int x, int y, char w, char h, unsigned char tile);
void init_actor(actor *act, int x, int y, int char_w, int char_h, unsigned char base_tile, unsigned char frame_count);
void move_actor(actor *act);
void draw_actor(actor *act);

void wait_frames(int wait_time);
void clear_sprites();

char is_touching(actor *act1, actor *act2);

#endif /* ACTOR_H */