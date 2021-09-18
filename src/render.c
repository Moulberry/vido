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

void make_sized_win(struct sized_win* win, int height, int width, int y, int x) {
        if(win->window != NULL) delwin(win->window);
        win->window = newwin(height, width, y, x);
        win->cols = width;
        win->lines = height;
}

void render_title() {
        char *title_str = ROOT_NAME;

        struct task *curr_task = root_task;
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

void render_task(struct task *task, int32_t *y, int32_t indent, uint32_t has_below);
void render_body() {
        int32_t y = -scroll_amount;

        wclear(body.window);

        if(root_task != NULL) {
                expanded_root = has_expanded_root();
                render_task(root_task, &y, 0, 0);
        }

        wrefresh(body.window);
}

void render_controls() {
        wclear(controls.window);

        if(visual_mode) {
                wattron(controls.window, A_BOLD);
                if(move_mode) {
			wprintw(controls.window, "-- MOVE --");
		} else {
			wprintw(controls.window, "-- VISUAL --");
		}
                wattroff(controls.window, A_BOLD);

		if(feedback_msg[0] != '\0') {
			wmove(controls.window, 0, controls.cols - strlen(feedback_msg));
			wprintw(controls.window, feedback_msg);
		}
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

			if(feedback_msg[0] != '\0') {
				wmove(controls.window, 0, controls.cols - strlen(feedback_msg));
				wprintw(controls.window, feedback_msg);
			}
                }

        }

        wrefresh(controls.window);
}

void render_borders(int lines, int cols) {
#ifdef DRAW_WINDOW_BORDERS
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
#endif
        refresh();
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

#ifndef DRAW_TASK_CONNECTING_LINES
                        waddch(body.window, ' ');
#else
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
#endif
                }
                wmove(body.window, *y, (indent)*2);

                char *indicator;
                if(task->child != NULL) {
                        indicator = task->done ? GROUP_FINISHED : GROUP_UNFINISHED;
                } else {
                        indicator = task->done ? TASK_FINISHED : TASK_UNFINISHED;
                }

                wprintw(body.window, indicator);

                if(task->highlighted || task == selected_task) wattron(body.window, COLOR_PAIR(1));
                wprintw(body.window, "%s", task->name);
                if(task->highlighted || task == selected_task) wattroff(body.window, COLOR_PAIR(1));
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
