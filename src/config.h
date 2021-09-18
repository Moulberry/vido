// Constants
#define TASK_JSON_LOCATION "/.vido/tasks.json"
#define ROOT_NAME "Projects"
#define MAX_COLS 100

// Task/group icons
#define GROUP_UNFINISHED "\u2B21 "
#define GROUP_FINISHED "\u2B22 "
#define TASK_UNFINISHED "\u2B1C"
#define TASK_FINISHED "\u2B1B"

// Comment/uncomment to change behaviour
#define DRAW_WINDOW_BORDERS
#define DRAW_TASK_CONNECTING_LINES

static const Key keys[] = {
/*	key		function		arg  */
	{'k',		move_cursor,		{.i = -1}},
	{KEY_UP,	move_cursor,		{.i = -1}},
	
	{'j',		move_cursor,		{.i = 1}},
	{KEY_DOWN,	move_cursor,		{.i = 1}},
	
	{'l',		toggle_selected,	{.i = 1}},
	{KEY_RIGHT,	toggle_selected,	{.i = 1}},
	
	{'h',		toggle_selected,	{.i = -1}},
	{KEY_LEFT,	toggle_selected,	{.i = -1}},
	
	{10,		toggle_selected,	{.i = 0}},
	
//	{'Q',		quit,			{.v = NULL}},
//	{'D',		delete_seltask,		{.v = NULL}},

	{':',		toggle_visual,		{.i = false}}
	
};

static const Command commands[] = {
	{"q",		quit_cmd,		{.v = NULL}},
	{"quit",	quit_cmd,		{.v = NULL}},
	{"a",		append_new_task,	{.v = NULL}},
	{"o",		append_child_new_task,	{.v = NULL}},
	{"d",		delete_seltask_cmd,	{.v = NULL}}
};
