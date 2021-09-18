// Constants
#define TASK_JSON_LOCATION "/.vido/tasks.json"
#define ROOT_NAME "Projects"
#define MAX_COLS 100
#define EDGE_SCROLL_DISTANCE 5

// Task/group icons
#define GROUP_UNFINISHED "\u2B21 "
#define GROUP_FINISHED "\u2B22 "
#define TASK_UNFINISHED "\u2B1C"
#define TASK_FINISHED "\u2B1B"

// Comment/uncomment to change behaviour
#define AUTOSAVE
#define DRAW_WINDOW_BORDERS
#define DRAW_TASK_CONNECTING_LINES

static const Key visual_keys[] = {
/*	key		function		arg  */
	{'k',		move_cursor_kb,		{.i = -1}},
	{KEY_UP,	move_cursor_kb,		{.i = -1}},
	
	{'j',		move_cursor_kb,		{.i = 1}},
	{KEY_DOWN,	move_cursor_kb,		{.i = 1}},
	
	{'l',		toggle_selected_kb,	{.i = 1}},
	{KEY_RIGHT,	toggle_selected_kb,	{.i = 1}},
	
	{'h',		toggle_selected_kb,	{.i = -1}},
	{KEY_LEFT,	toggle_selected_kb,	{.i = -1}},

	{'m', 		set_move_mode_kb,	{.i = true}},

	{10,		toggle_selected_kb,	{.i = 0}},
	
//	{'W',		save_kb,		{.v = NULL}},
//	{'Q',		quit_kb,		{.v = NULL}},
//	{'D',		delete_seltask_kb,	{.v = NULL}},

	{':',		toggle_visual_kb,	{.i = false}}
};

static const Key move_keys[] = {
/*	key		function		arg  */
	{'k',		move_seltask_kb,	{.i = -1}},
	{KEY_UP,	move_seltask_kb,	{.i = -1}},
	
	{'j',		move_seltask_kb,	{.i = 1}},
	{KEY_DOWN,	move_seltask_kb,	{.i = 1}},
	
	{'l',		horz_move_seltask_kb,	{.i = 1}},
	{KEY_RIGHT,	horz_move_seltask_kb,	{.i = 1}},
	
	{'h',		horz_move_seltask_kb,	{.i = -1}},
	{KEY_LEFT,	horz_move_seltask_kb,	{.i = -1}},

	{'m', 		set_move_mode_kb,	{.i = false}},
	{':',		toggle_visual_kb,	{.i = false}}
};

static const Command commands[] = {
	{"q",		quit_cmd,		{.v = NULL}},
	{"quit",	quit_cmd,		{.v = NULL}},
	{"a",		append_sibling_cmd,	{.v = NULL}},
	{"o",		append_child_cmd,	{.v = NULL}},
	{"r",		rename_seltask_cmd,	{.v = NULL}},
	{"d",		delete_seltask_cmd,	{.v = NULL}},
	{"w",		save_cmd,		{.v = NULL}}
};
