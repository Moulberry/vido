void move_cursor(const Arg *arg) {
        if(arg->i == 0) return;
	
	if(arg->i < 0) {
		//Move up by -arg->i
		for(int i=0; i<-arg->i; i++) {
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
	} else {
		//Move down by arg->i
		for(int i=0; i<arg->i; i++) {
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
}

void toggle_visual(const Arg *arg) {
	visual_mode = arg->i;
	if(!visual_mode) {
		//Clear command buffer and set first char
		memset(buffer, '\0', 256);
		buffer[0] = ':';
	}
}

void toggle_selected(const Arg *arg) {
	bool toggle = arg->i == 0;
	int value = arg->i == 1 ? 1 : 0;

	if(selected_task != NULL) {
		if(selected_task->child == NULL) {
			if(toggle) {
				selected_task->done = !selected_task->done;
			} else {
				if(selected_task->done == value) return;
				selected_task->done = value;
			}
		} else {
			if(toggle) {
				selected_task->expanded = !selected_task->expanded;
			} else {
				if(selected_task->expanded == value) return;
				selected_task->expanded = value;
			}
		}
		update_tasks();
		render_title();

		save_task_to_file();
	}
}

void quit(const Arg *arg) {
	endwin();
	exit(0);
}

void quit_cmd(const Arg *arg, int argc, char **argv) {
	quit(arg);
}

void append_new_task(const Arg *arg, int argc, char **argv) {
	if(argc == 1) {
		sprintf(error_msg, "Missing argument: task name");
	} else {
		for(int i=2; i<argc; i++) *(argv[i]-1) = ' ';

		struct task *new_task = create_task(argv[1]);

		if(selected_task == NULL) {
			destroy_task(root_task);
			root_task = new_task;
		} else {
			if(selected_task->is_root) {
				selected_task->expanded = false;
			}

			link_next(new_task, selected_task->next);
			link_next(selected_task, new_task);
			selected_task = new_task;
		}
		visual_mode = true;
		save_task_to_file();
	}
}

void append_child_new_task(const Arg *arg, int argc, char **argv) {
	if(argc == 1) {
		sprintf(error_msg, "Missing argument: task name");
	} else {
		for(int i=2; i<argc; i++) *(argv[i]-1) = ' ';

		struct task *new_task = create_task(argv[1]);

		if(selected_task == NULL) {
			destroy_task(root_task);
			root_task = new_task;
		} else {
			selected_task->expanded = true;

			link_next(new_task, selected_task->child);
			link_child(selected_task, new_task);
			selected_task = new_task;
		}
		visual_mode = true;
		save_task_to_file();
	}
}

void delete_seltask(const Arg* arg);
void delete_seltask_cmd(const Arg *arg, int argc, char **argv) {
	delete_seltask(arg);
}

void delete_seltask(const Arg *arg) {
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
	} else if(selected_task == root_task && root_task->next != NULL) {
		root_task = root_task->next;
		root_task->prev = NULL;

		selected_task->next = NULL;
		destroy_task(selected_task);

		selected_task = root_task;
	} else {
		return;
	}

	visual_mode = true;
	save_task_to_file();
}
