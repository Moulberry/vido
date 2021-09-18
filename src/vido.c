#include <string.h>
#include <locale.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "../json/cJSON.c"

#define _GNU_SOURCE              // enable ncurses wchar support
#define _XOPEN_SOURCE_EXTENDED 1 // enable ncurses wchar support

#if defined( WIN32 )
#include <curses.h>
#else
#include <ncurses.h>
#endif

#define LENGTH(X)               (sizeof X / sizeof X[0])
#define END(A)                  ((A) + LENGTH(A))

struct sized_win {
	WINDOW *window;
	uint32_t lines;
	uint32_t cols;
} title, body, controls;

void make_sized_win(struct sized_win* win, int height, int width, int y, int x);
void setup_windows();
void render_title();
void render_body();
void render_controls();
void render_borders();

struct task {
	char *name;
	bool done; //This task has been completed
	bool expanded; //Whether groups should show their subtasks

	bool is_root;

	struct task *prev;
	struct task *next;
	struct task *parent;
	struct task *child;
};

bool process_ch_visual(int ch);
bool process_ch_normal(int ch);

void update_scroll();
bool has_expanded_root();
void update_tasks();

struct task *create_task(char *name);
void destroy_task(struct task *task);
void link_next(struct task *from, struct task *to);
void link_child(struct task *from, struct task *to);

void load_json_from_file();
void save_task_to_file();
void task_to_json();
void json_to_task();

struct task *root_task = NULL;
struct task *selected_task = NULL;
cJSON *task_json = NULL;

int scroll_amount = 5;
bool expanded_root = false;
char error_msg[512];
char buffer[256];
bool visual_mode = true;

typedef union {
        int i;
        unsigned int ui;
        float f;
        const void *v;
} Arg;

typedef struct {
	int key;
	void (*func)(const Arg *);
	const Arg arg;
} Key;

typedef struct {
	char *command;
	void (*func)(const Arg *, int, char **);
	const Arg arg;
} Command;

#include "commands.c"
#include "config.h"

#include "render.c"

int main(int argc, char *argv[]) {
	setlocale(LC_ALL, "");
	initscr();
	cbreak();
	noecho();
	set_escdelay(100);
	keypad(stdscr, TRUE);
	curs_set(0);

	start_color();
	init_pair(1, COLOR_BLACK, COLOR_WHITE);
	init_pair(2, COLOR_WHITE, COLOR_RED);

	load_json_from_file();
	json_to_task();

	if(root_task == NULL) {
		root_task = create_task("Main");
		save_task_to_file();
	}

	for(struct task *curr_task = root_task; curr_task != NULL; curr_task = curr_task->next) {
		curr_task->expanded = false;
	}
	update_tasks();

	setup_windows();

	while(true) {
		int ch = getch();
		
		bool redraw = false;

		if(ch == KEY_RESIZE) {
			setup_windows();
		} else if(visual_mode) {
			redraw = process_ch_visual(ch);
		} else {
			redraw = process_ch_normal(ch);
		}

		if(redraw) {
			render_body();
			render_controls();
		}
	}

	endwin();
}

/** Input handling **/

bool process_ch_visual(int ch) {
	bool handled = false;
        const Key *k;
        for(k = keys; k < END(keys); k++) {
                if(k->key == ch) {
                        k->func(&k->arg);
                        handled = true;
                }
        }
        return handled;
}

void exec_cmd(int argc, char **argv) {
	bool handled = false;
        const Command *c;
        for(c = commands; c < END(commands); c++) {
                if(!strcmp(argv[0], c->command)) {
                        c->func(&c->arg, argc, argv);
                        handled = true;
                }
        }

        if(!handled) {
		sprintf(error_msg, "Invalid command: %s", buffer+1);
	}
}

bool process_ch_normal(int ch) {
	if(error_msg[0] != '\0') {
		memset(error_msg, '\0', 256);
		if(ch == ':' || ch == '/') {
			buffer[0] = ch;
			return true;
		}
	}

	if(ch == 27) {
		visual_mode = true;
		return true;
	} else if(ch >= ' ' && ch <= 'z') {
		for(int i=0; i<256; i++) {
			if(buffer[i] == '\0') {
				buffer[i] = ch;
				return true;
			}
		}
		return false;
	} else if(ch == KEY_BACKSPACE) {
		for(int i=255; i >= 0; i--) {
			if(buffer[i] != '\0') {
				buffer[i] = '\0';
				break;
			}
		}
		if(buffer[0] == '\0') visual_mode = true;
		return true;
	} else if(ch == 10) {
		char first_char = buffer[0];
		
		int buff_len = strlen(buffer);

		bool new_arg = true;
		int argc = 0;
		char *argv[64];
		for(int i=1; i<buff_len; i++) {
			if(buffer[i] == ' ') {
				buffer[i] = '\0';
				new_arg = true;
			} else if(new_arg) {
				argv[argc++] = &(buffer[i]);
				new_arg = false;
			}
		}

		if(first_char == ':') exec_cmd(argc, argv);

		for(int i=0; i<buff_len; i++) {
			if(buffer[i] == '\0') buffer[i] = ' ';
		}

		memset(buffer, '\0', 256);
		buffer[0] = first_char;

		return true;
	}
	return false;
}

/** Task updating **/

uint32_t get_task_size(struct task* task, struct task* stop_at) {
	if(task->child == NULL || !task->expanded) return 1;

	task = task->child;

	uint32_t size = 1;
	while(task != NULL) {
		if(task == stop_at) break;

		size += get_task_size(task, stop_at);

		if(task->next != NULL) {
			task = task->next;
		} else {
			break;
		}
	}

	return size;
}

void update_scroll() {
	uint32_t selected_y = 0;

	//We want to step back up the hierarchy to figure out the y-value of this task
	
	struct task* task = selected_task;
	while(true) {
		if(task->prev != NULL) {
			task = task->prev;
			selected_y += get_task_size(task, NULL);
		} else if(task->parent != NULL) {
			selected_y += get_task_size(task->parent, task);
			task = task->parent;
		} else {
			break;
		}
	}

	if(body.lines <= 1) {
		scroll_amount = selected_y;
	} else if(selected_y < scroll_amount+1) {
		scroll_amount = selected_y-1;
	} else if(selected_y > scroll_amount+body.lines-2) {
		scroll_amount = selected_y - body.lines + 2;
	}
	if(scroll_amount < 0) scroll_amount = 0;
	//TODO: Maybe apply maximum limit to scroll_amount?

}

void update_task(struct task* task, bool root, bool root_expanded, bool *all_selected) {
	if(task->child != NULL) {
		task->done = true;
		update_task(task->child, false, root_expanded, &task->done);
	}
	
	if(all_selected != NULL) (*all_selected) &= task->done;

	task->is_root = root;
	if(root && task->expanded) {
		if(root_expanded) {
			task->expanded = false;
		} else {
			root_expanded = true;	
		}
	}

	if(task->next != NULL) {
		update_task(task->next, root, root_expanded, all_selected);
	}
}

bool has_expanded_root() {
	struct task *curr_task = root_task;
	while(curr_task != NULL) {
		if(curr_task->child != NULL && curr_task->expanded) return true;
		curr_task = curr_task->next;
	}
	return false;
}

void update_tasks() {
	if(selected_task == NULL) selected_task = root_task;
	expanded_root = has_expanded_root();
	update_task(root_task, true, false, NULL);
}

/** Task creation & linking **/

struct task *create_task(char *name) {
	struct task *task = (struct task*) calloc(1, sizeof(struct task));
	
	char *name_cpy = calloc(1, strlen(name)+1);
	strcpy(name_cpy, name);
	task->name = name_cpy;

	task->expanded = true;
	return task;
}

void destroy_task(struct task *task) {
	if(task == NULL) return;

	destroy_task(task->next);
	destroy_task(task->child); // we don't want orphans

	free(task->name);
	free(task);
}

void link_next(struct task *from, struct task *to) {
	from->next = to;
	if(to != NULL) {
		to->parent = NULL;
		to->prev = from;
	}
}

void link_child(struct task *from, struct task *to) {
	from->child = to;
	if(to != NULL) {
		to->prev = NULL;
		to->parent = from;
	}
}

/** I/O for saving tasks **/

void load_json_from_file() {
	if(task_json != NULL) cJSON_Delete(task_json);

	char filelocation[256];
	strcat(strcpy(filelocation, getenv("HOME")), TASK_JSON_LOCATION);

	FILE *file = fopen(filelocation, "r");
	if(file) {
		long file_size;

		//Determine file size
		fseek(file, 0L, SEEK_END);
		file_size = ftell(file);
		rewind(file);

		//Allocate memory
		char* file_contents = (char *)calloc(1, file_size+1);

		//Read file
		fread(file_contents, file_size, 1, file);

		//Create json
		task_json = cJSON_Parse(file_contents);

		//Free memory
		free(file_contents);

		//Close file handle
		fclose(file);
	}
}

void save_task_to_file() {
	task_to_json();

	char filelocation[256];

	strcat(strcpy(filelocation, getenv("HOME")), TASK_JSON_LOCATION);
	
	FILE *file = fopen(filelocation, "w");
	if(file) {
		//Allocate & create json string
		char *json_string = cJSON_Print(task_json);

		//Write to file
		fputs(json_string, file);

		//Free json string
		free(json_string);

		//Close file handle
		fclose(file);
	}
}

cJSON *task_to_json_impl(struct task *task) {
	cJSON *json = cJSON_CreateObject();
	
	cJSON_AddItemToObject(json, "text", cJSON_CreateString(task->name));
	cJSON_AddItemToObject(json, "done", cJSON_CreateBool(task->done));
	cJSON_AddItemToObject(json, "expanded", cJSON_CreateBool(task->expanded));

	if(task->child != NULL) {
		cJSON *children = cJSON_CreateArray();
		cJSON_AddItemToObject(json, "children", children);

		for(struct task *child = task->child; child != NULL; child = child->next) {
			cJSON_AddItemToArray(children, task_to_json_impl(child));
		}
	}

	return json;
}

void task_to_json() {
	if(task_json != NULL) cJSON_Delete(task_json);

	task_json = cJSON_CreateArray();

	if(root_task != NULL) {
		for(struct task *child = root_task; child != NULL; child = child->next) {
			cJSON_AddItemToArray(task_json, task_to_json_impl(child));
		}
	}
}

struct task* json_to_task_impl(cJSON *object) {
	struct task *root = NULL;

	struct task *last_task = NULL;
	for(cJSON *json = object; json != NULL; json = json->next) {
		cJSON *name = cJSON_GetObjectItem(json, "text");
		if(!cJSON_IsString(name)) continue;

		struct task *new_task = create_task(name->valuestring);
		
		new_task->done = cJSON_IsTrue(cJSON_GetObjectItem(json, "done"));
		new_task->expanded = cJSON_IsTrue(cJSON_GetObjectItem(json, "expanded"));
		
		if(root == NULL) {
			root = new_task;
		} else {
			link_next(last_task, new_task);
		}
		
		cJSON *children = cJSON_GetObjectItem(json, "children");
		if(cJSON_IsArray(children)) {
			link_child(new_task, json_to_task_impl(children->child));
		}

		last_task = new_task;
	}

	return root;
}

void json_to_task() {
	destroy_task(root_task);
	
	if(task_json != NULL) {
		root_task = json_to_task_impl(task_json->child);
	}
}
