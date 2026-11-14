#include "torch.h"
#include "physics.h"
#include "rooms.h"

// posizione torce: five Pipe Hall, six Kong's Maze, five Library
static const struct { s16 room; u16 x, y; } s_home[TORCH_COUNT] = {
	{0, 70, 200}, {0, 160, 200}, {0, 220, 200}, {0, 136, 84}, {0, 264, 80},
	{1, 24, 46}, {1, 92, 25}, {1, 160, 59},
	{1, 58, 127}, {1, 262, 93}, {1, 126, 195},
	{2, 65, 48}, {2, 155, 48}, {2, 245, 48}, {2, 30, 200}, {2, 300, 200},
};

// the set-down ring, every slot within TORCH_CLUSTER_RADIUS of the others
static const s8 s_ring[TORCH_COUNT][2] = {
	{0, 0}, {6, 0}, {-6, 0}, {0, 6}, {0, -6},
	{5, 4}, {5, -4}, {-5, 4}, {-5, -4},
	{8, 0}, {-8, 0}, {0, 8}, {0, -8},
	{6, 6}, {6, -6}, {-6, 6},
};

Torch g_torches[TORCH_COUNT];

// reset a torch to its home position
static void reset_torch(u8 i){
	g_torches[i].room = s_home[i].room;
	g_torches[i].x = s_home[i].x;
	g_torches[i].y = s_home[i].y;
	g_torches[i].state = TORCH_HOME;
}

void torches_init(void){
	u8 i;
	for (i = 0; i < TORCH_COUNT; i++) reset_torch(i);
}

void torches_reset_room(s16 room){
	u8 i;
	for (i = 0; i < TORCH_COUNT; i++)
		if (s_home[i].room == room) reset_torch(i);
}

u8 torches_held(void){
	u8 i, n = 0;

	for (i = 0; i < TORCH_COUNT; i++)
		if (g_torches[i].state == TORCH_HELD) n++;
	return n;
}

static int square_dist(const Torch *t, s16 x, s16 y){
	s16 dx = (s16)t->x - x, dy = (s16)t->y - y;

	return (int)dx * dx + (int)dy * dy;
}

// keep a set-down torch inside the room
static u16 inside(int v, int hi){
	if (v < 0) return 0;
	if (v > hi) return (u16)hi;
	return (u16)v;
}

bool torches_take_touched(Player *p){
	int best = -1, best_d = TORCH_REACH * TORCH_REACH;
	u8 i;

	for (i = 0; i < TORCH_COUNT; i++){
		int d;

		if (g_torches[i].state != TORCH_HOME || g_torches[i].room != p->room)
			continue;
		d = square_dist(&g_torches[i], (s16)p->px, (s16)p->py);
		if (d <= best_d){ best_d = d; best = i; }
	}
	if (best < 0)
		return false;
	g_torches[best].state = TORCH_HELD;
	return true;
}

static u8 placed_in_room(s16 room){
	u8 i, n = 0;

	for (i = 0; i < TORCH_COUNT; i++)
		if (g_torches[i].state == TORCH_PLACED && g_torches[i].room == room)
			n++;
	return n;
}

bool torches_set_down(Player *p){
	u8 i, slot = placed_in_room(p->room);

	for (i = 0; i < TORCH_COUNT; i++){
		if (g_torches[i].state != TORCH_HELD)
			continue;
		g_torches[i].state = TORCH_PLACED;
		g_torches[i].room = p->room;
		g_torches[i].x = inside((s16)p->px + s_ring[slot][0], ROOM_W);
		g_torches[i].y = inside((s16)p->py + s_ring[slot][1], GROUND_Y);
		return true;
	}
	return false;
}

u16 torches_light(s16 room){
	int r2 = TORCH_CLUSTER_RADIUS * TORCH_CLUSTER_RADIUS;
	u8 i, j, lit = 0;

	for (i = 0; i < TORCH_COUNT; i++){
		if (g_torches[i].room != room || g_torches[i].state == TORCH_HELD)
			continue;
		for (j = 0; j < TORCH_COUNT; j++){
			if (j == i)
				continue;
			if (g_torches[j].room != room || g_torches[j].state == TORCH_HELD)
				continue;
			if (square_dist(&g_torches[i], (s16)g_torches[j].x,
					(s16)g_torches[j].y) <= r2){
				lit++;
				break;
			}
		}
	}
	return (u16)lit * TORCH_LIGHT;
}
