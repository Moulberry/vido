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

#define TASK_JSON_LOCATION "/.vido/tasks.json"
#define ROOT_NAME "Projects"
#define MAX_COLS 100

void render_borders();
void render_title();
void render_body();
void render_controls();

struct sized_win {
	WINDOW *window;
	uint32_t lines;
	uint32_t cols;
};

void setup_windows();

void make_sized_win(struct sized_win* win, int height, int width, int y, int x);
struct sized_win title;
struct sized_win body;
struct sized_win controls;

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

void link_next(struct task *from, struct task *to);
void link_child(struct task *from, struct task *to);

bool process_ch_visual(int ch);
bool process_ch_normal(int ch);

bool has_expanded_root();
struct task *create_task(char *name);
void destroy_task(struct task *task);
void update_tasks();

void load_json_from_file();
void save_task_to_file();

void task_to_json();
void json_to_task();

struct task *task = NULL;
cJSON *task_json = NULL;

int scroll_amount = 5;

bool expanded_root = false;
char error_msg[512];
char buffer[256];
bool visual_mode = true;
struct task *selected_task = NULL;

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

	if(task == NULL) {
		task = create_task("Main");
		save_task_to_file();
	}

	for(struct task *curr_task = task; curr_task != NULL; curr_task = curr_task->next) {
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

void setup_windows() {
	int cols = MAX_COLS > COLS ? COLS : MAX_COLS;
	make_sized_win(&title, 1, cols-2, 1, 1);
	make_sized_win(&body, LINES-5, cols-2, 3, 1);
	make_sized_win(&controls, 1, cols, LINES-1, 0);

	clear();

	render_borders(LINES, cols);
	
	render_title();
	render_body();
	render_controls();
}

void go_up(int count) {
	for(int i=0; i<count; i++) {
		if((!selected_task->is_root || !has_expanded_root()) &&
				selected_task->prev != NULL) {
			selected_task = selected_task->prev;
			if(selected_task->expanded && selected_task->child != NULL) {
				selected_task = selected_task->child;
				while(selected_task->next != NULL) {
					selected_task = selected_task->next;
				}
			}
		} else if(selected_task->parent != NULL) {
			selected_task = selected_task->parent;
		}
	}
}

void go_down(int count) {
	for(int i=0; i<count; i++) {
		if(selected_task->expanded && selected_task->child != NULL) {
			selected_task = selected_task->child;
		} else if(selected_task->next != NULL) {
			selected_task = selected_task->next;
		} else {
			struct task *curr_task = selected_task;
			bool parented = false;
			while(true) {
				if(parented) {
					parented = false;
					if(curr_task->next != NULL) {
						if(!curr_task->is_root) {
							selected_task = curr_task->next;
						}
						break;
					}
				} else {
					if(curr_task->prev != NULL) {
						curr_task = curr_task->prev;
					} else if(curr_task->parent != NULL) {
						parented = true;
						curr_task = curr_task->parent;
					} else {
						break;
					}
				}
			}
		}
	}
}

bool process_ch_visual(int ch) {
	switch(ch) {
		case 'k':
		case KEY_UP:
			go_up(1);
			return true;
		case 'j':
		case KEY_DOWN:
			go_down(1);
			return true;
		case '/':
		case ':':
			visual_mode = false;
			memset(buffer, '\0', 256);
			buffer[0] = ch;
			return true;
		case 'h':
		case KEY_LEFT:
			if(selected_task != NULL) {
				if(selected_task->child == NULL) {
					if(!selected_task->done) return false;
					selected_task->done = false;
				} else {
					if(!selected_task->expanded) return false;
					selected_task->expanded = false;
				}
				update_tasks();
				render_title();
				
				save_task_to_file();
			}
			return true;
		case 'l':
		case KEY_RIGHT:
			if(selected_task != NULL) {
				if(selected_task->child == NULL) {
					if(selected_task->done) return false;
					selected_task->done = true;
				} else {
					if(selected_task->expanded) return false;
					selected_task->expanded = true;
				}
				update_tasks();
				render_title();
				
				save_task_to_file();
			}
			return true;
		case 10:
			if(selected_task != NULL) {
				if(selected_task->child == NULL) {
					selected_task->done = !selected_task->done;
				} else {
					selected_task->expanded = !selected_task->expanded;
				}
				update_tasks();
				render_title();
				
				save_task_to_file();
			}
			return true;
	}
	return false;
}

void exec_cmd(int argc, char **argv) {
	if(!strcmp(argv[0], "q") || !strcmp(argv[0], "quit")) {
		endwin();
		exit(0);
	} else if(!strcmp(argv[0], "up")) {
		if(argc >= 2) {
			int amount = atoi(argv[1]);
			if(amount == 0) {
				sprintf(error_msg, "Invalid number: %s", argv[1]);
			}
			go_up(amount);
		} else {
			go_up(1);
		}
		return;
	} else if(!strcmp(argv[0], "down")) {
		if(argc >= 2) {
			int amount = atoi(argv[1]);
			if(amount == 0) {
				sprintf(error_msg, "Invalid number: %s", argv[1]);
			}
			go_down(amount);
		} else {
			go_down(1);
		}
		return;

	} else if(!strcmp(argv[0], "a") || !strcmp(argv[0], "o")) {
		if(argc == 1) {
			sprintf(error_msg, "Missing argument: task name");
		} else {
			for(int i=2; i<argc; i++) *(argv[i]-1) = ' ';
			
			struct task *new_task = create_task(argv[1]);

			if(selected_task == NULL) {
				destroy_task(task);
				task = new_task;
			} else {
				if(!strcmp(argv[0], "a")) {
					if(selected_task->is_root) {
						selected_task->expanded = false;
					}

					link_next(new_task, selected_task->next);
					link_next(selected_task, new_task);
				} else {
					selected_task->expanded = true;

					link_next(new_task, selected_task->child);
					link_child(selected_task, new_task);
				}
				selected_task = new_task;
			}


		}
		visual_mode = true;
		save_task_to_file();
		return;
	} else if(!strcmp(argv[0], "d")) {
		if(selected_task->prev != NULL) {
			link_next(selected_task->prev, selected_task->next);

			struct task *to_destroy = selected_task;
			if(selected_task->next != NULL) {
				selected_task = selected_task->next;
			} else {
				selected_task = selected_task->prev;
			}

			to_destroy->next = NULL;
			destroy_task(to_destroy);
		} else if(selected_task->parent != NULL) {
			link_child(selected_task->parent, selected_task->next);

			struct task *to_destroy = selected_task;
			if(selected_task->next != NULL) {
				selected_task = selected_task->next;
			} else {
				selected_task = selected_task->parent;
			}

			to_destroy->next = NULL;
			destroy_task(to_destroy);
		} else if(selected_task == task && task->next != NULL) {
			task = task->next;
			task->prev = NULL;

			selected_task->next = NULL;
			destroy_task(selected_task);
			
			selected_task = task;
		}
		visual_mode = true;
		save_task_to_file();
		return;
	}

	sprintf(error_msg, "Invalid command: %s", buffer+1);
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

void render_borders(int lines, int cols) {
	//Borders
	mvvline(0, 0, 0, lines-1); /* left */
	mvvline(0, cols-1, 0, lines-1); /* right */
	mvhline(0, 0, 0, cols); /* top */
	mvhline(2, 0, 0, cols); /* middle */
	mvhline(lines-2, 0, 0, cols); /* bottom */

	//Corners
	mvaddch(0, 0, ACS_ULCORNER);
	mvaddch(0, cols-1, ACS_URCORNER);
	mvaddch(2, 0, ACS_LTEE);
	mvaddch(2, cols-1, ACS_RTEE);
	mvaddch(lines-2, 0, ACS_LLCORNER);
	mvaddch(lines-2, cols-1, ACS_LRCORNER);

	refresh();
}

void render_title() {
	char *title_str = ROOT_NAME;

	struct task *curr_task = task;
	while(curr_task != NULL) {
		if(curr_task->child != NULL && curr_task->expanded) {
			title_str = curr_task->name;
			break;
		}
		curr_task = curr_task->next;
	}
	
	wclear(title.window);
	mvwprintw(title.window, 0, (title.cols-strlen(title_str))/2, title_str);
	wrefresh(title.window);
}

void render_task(struct task *task, int32_t *y, int32_t indent, uint32_t has_below) {	
	if(*y >= 0 && *y >= body.lines) {
		return;
	}

	if(indent == 0 && expanded_root && (task->child == NULL || !task->expanded)) {
		if(task->next != NULL) {
			render_task(task->next, y, indent, has_below);
		}
		return;
	}

	if(*y >= 0) {
		wmove(body.window, *y, 0);

		bool horzline = true;
		for(int i=(indent)*2-1; i>=0; i--) {
			wmove(body.window, *y, i);

			if(i % 2 == 0) {
				if(!horzline) {
					if(!(has_below & (1 << (i/2)))) {
						waddch(body.window, ' ');	
					} else {
						waddch(body.window, ACS_VLINE);
					}
				} else if(task->next != NULL) {
					waddch(body.window, ACS_LTEE);
				} else {
					waddch(body.window, ACS_LLCORNER);
				}
				horzline = false;
			} else if(horzline) {
				waddch(body.window, ACS_HLINE);
			}
		}
		wmove(body.window, *y, (indent)*2);

		char *indicator;
		if(task->child != NULL) {
			indicator = task->done ? "\u2B22 " : "\u2B21 ";
		} else {
			indicator = task->done ? "\u2B1B" : "\u2B1C";
		}

		wprintw(body.window, indicator);
		
		if(task == selected_task) wattron(body.window, COLOR_PAIR(1));
		wprintw(body.window, "%s", task->name);
		if(task == selected_task) wattroff(body.window, COLOR_PAIR(1));
	}

	(*y)++;
	
	if(task->expanded && task->child != NULL) {
		uint32_t has_below_child = has_below | (1 << (indent));

		if(task->next == NULL && (indent) > 0) {
			has_below_child &= ~(1 << (indent-1));
		}

		render_task(task->child, y, indent+1, has_below_child);
	}

	if(task->next != NULL) {
		render_task(task->next, y, indent, has_below);
	}
}

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
			fprintf(stderr, "PrevSize:%d\n", get_task_size(task, NULL));
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
	struct task *curr_task = task;
	while(curr_task != NULL) {
		if(curr_task->child != NULL && curr_task->expanded) return true;
		curr_task = curr_task->next;
	}
	return false;
}

void update_tasks() {
	if(selected_task == NULL) selected_task = task;
	expanded_root = has_expanded_root();
	update_task(task, true, false, NULL);
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

void render_body() {
	update_scroll();
	int32_t y = -scroll_amount;

	wclear(body.window);

	if(task != NULL) {
		expanded_root = has_expanded_root();
		render_task(task, &y, 0, 0);
	}
	
	wrefresh(body.window);
}

void render_controls() {
	wclear(controls.window);

	if(visual_mode) {
		wattron(controls.window, A_BOLD);
		wprintw(controls.window, "-- VISUAL --");
		wattroff(controls.window, A_BOLD);
	} else {
		if(error_msg[0] != '\0') {
			waddch(controls.window, ' ');
			wattron(controls.window, COLOR_PAIR(2));
			wprintw(controls.window, error_msg);
			wattroff(controls.window, COLOR_PAIR(2));
		} else {
			wprintw(controls.window, buffer);
			
			wattron(controls.window, COLOR_PAIR(1));
			waddch(controls.window, ' ');
			wattroff(controls.window, COLOR_PAIR(1));
		}

	}

	wrefresh(controls.window);
}

void make_sized_win(struct sized_win* win, int height, int width, int y, int x) {
	if(win->window != NULL) delwin(win->window);
	win->window = newwin(height, width, y, x);
	win->cols = width;
	win->lines = height;
}

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

	if(task != NULL) {
		for(struct task *child = task; child != NULL; child = child->next) {
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
	destroy_task(task);
	
	if(task_json != NULL) {
		task = json_to_task_impl(task_json->child);
	}
}
