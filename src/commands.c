void move_cursor_kb(const Arg *arg) {
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
	update_scroll();
}

void toggle_visual_kb(const Arg *arg) {
	visual_mode = arg->i;
	if(!visual_mode) {
		//Clear command buffer and set first char
		memset(buffer, '\0', 256);
		buffer[0] = ':';
	}
}

void toggle_selected_kb(const Arg *arg) {
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

		save_task_to_file(true);
	}
}


void set_move_mode_kb(const Arg *arg);
void set_move_mode_cmd(const Arg *arg, int argc, char **argv) {
	set_move_mode_kb(arg);
}

void set_move_mode_kb(const Arg *arg) {
	visual_mode = true;
	if(move_mode != arg->i) {
		move_mode = arg->i;
		update_tasks();
	}
}


void quit_kb(const Arg *arg);
void quit_cmd(const Arg *arg, int argc, char **argv) {
	quit_kb(arg);
}

void quit_kb(const Arg *arg) {
	endwin();
	exit(0);
}


void move_seltask_impl(int direction) {
	visual_mode = true;

	if(selected_task->prev == NULL && selected_task->next == NULL) return;

	if(selected_task->is_root) selected_task->expanded = false;

	if(selected_task->prev != NULL) {
		link_next(selected_task->prev, selected_task->next);
	} else if(selected_task->parent != NULL) {
		link_child(selected_task->parent, selected_task->next);
	} else if(selected_task == root_task && root_task->next != NULL) {
		root_task = root_task->next;
		root_task->prev = NULL;
	} else {
		return;
	}

	bool insert_above = true;

	struct task *task = selected_task->next;
	if(task == NULL) {
		task = selected_task->prev;
		insert_above = false;
	}

	while(true) {
		if(direction > 0) {
			if(task->next == NULL) {
				insert_above = false;	
				break;
			}
			task = task->next;
			direction--;
		} else if(direction < 0) {
			if(task->prev == NULL) {
				insert_above = true;
				break;
			}
			task = task->prev;
			direction++;
		} else {
			break;
		}
	}

	if(insert_above && task == root_task) {
		link_next(selected_task, task);
		root_task = selected_task;
		root_task->prev = NULL;
	} else if(insert_above && task->prev != NULL) {
		link_next(task->prev, selected_task);
		link_next(selected_task, task);
	} else if(!insert_above) {
		link_next(selected_task, task->next);
		link_next(task, selected_task);
	} else if(task->parent != NULL) {
		link_child(task->parent, selected_task);
		link_next(selected_task, task);
	}

	update_tasks();
	save_task_to_file(true);
	update_scroll();
}

void move_seltask_kb(const Arg *arg) {
	move_seltask_impl(arg->i);
}

void move_seltask_cmd(const Arg *arg, int argc, char **argv) {
	if(argc == 1) {
		move_seltask_impl(arg->i);
	} else {
		int amount = atoi(argv[1]);
		move_seltask_impl(arg->i * amount);
	}
}

void horz_move_seltask_impl(int direction) {
	if(direction > 0) {
		if(selected_task->next == NULL) return;
		
		if(selected_task->prev != NULL) {
			link_next(selected_task->prev, selected_task->next);
		} else if(selected_task->parent != NULL) {
			link_child(selected_task->parent, selected_task->next);
		} else if(selected_task == root_task && root_task->next != NULL) {
			root_task = root_task->next;
			root_task->prev = NULL;
		} else {
			return;
		}

		struct task *new_parent = selected_task->next;
		new_parent->expanded = true;

		for(int i=0; i<direction-1; i++) {
			if(new_parent->child != NULL) break;
			
			new_parent = new_parent->child;
			new_parent->expanded = true;
		}

		link_next(selected_task, new_parent->child);
		link_child(new_parent, selected_task);
	} else if(direction < 0) {
		if(selected_task->parent == NULL) return;
		
		if(selected_task->prev != NULL) {
			link_next(selected_task->prev, selected_task->next);
		} else if(selected_task->parent != NULL) {
			link_child(selected_task->parent, selected_task->next);
		} else {
			return;
		}

		for(int i=0; i<-direction; i++) {
			if(selected_task->parent == NULL) return;

			struct task *parent = selected_task->parent;

			link_next(selected_task, parent->next);
			link_next(parent, selected_task);
			if(parent->is_root) {
				parent->expanded = false;
				selected_task->expanded = false;
			}
				
			selected_task->parent = NULL;
			move_seltask_impl(-1); //Move up
		}
	} else {
		return;
	}

	update_tasks();
	save_task_to_file(true);
	update_scroll();
}

void horz_move_seltask_kb(const Arg *arg) {
	horz_move_seltask_impl(arg->i);
}

void horz_move_seltask_cmd(const Arg *arg, int argc, char **argv) {
	if(argc == 1) {
		horz_move_seltask_impl(arg->i);
	} else {
		int amount = atoi(argv[1]);
		horz_move_seltask_impl(arg->i * amount);
	}
}

void append_sibling_cmd(const Arg *arg, int argc, char **argv) {
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
		save_task_to_file(true);
		update_scroll();
		sprintf(feedback_msg, "Appended sibling: %s", argv[1]);
	}
}

void append_child_cmd(const Arg *arg, int argc, char **argv) {
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
		save_task_to_file(true);
		update_scroll();
		sprintf(feedback_msg, "Appended child: %s", argv[1]);
	}
}

void save_kb(const Arg *arg);
void save_cmd(const Arg *arg, int argc, char **argv) {
	save_kb(arg);
}
void save_kb(const Arg *arg) {
	save_task_to_file(false);
	sprintf(feedback_msg, "Saved!");
}

void rename_seltask_cmd(const Arg *arg, int argc, char **argv) {
	if(argc == 1) {
		sprintf(error_msg, "Missing argument: task name");
	} else {
		for(int i=2; i<argc; i++) *(argv[i]-1) = ' ';
		
		if(selected_task != NULL) {
			rename_task(selected_task, argv[1]);
		}

		visual_mode = true;
		save_task_to_file(true);
		sprintf(feedback_msg, "Renamed to: %s", argv[1]);
	}
}

void delete_seltask_kb(const Arg *arg);
void delete_seltask_cmd(const Arg *arg, int argc, char **argv) {
	delete_seltask_kb(arg);
}

void delete_seltask_kb(const Arg *arg) {
	if(selected_task->prev != NULL) {
		link_next(selected_task->prev, selected_task->next);

		struct task *to_destroy = selected_task;
		if(selected_task->next != NULL) {
			selected_task = selected_task->next;
		} else {
			selected_task = selected_task->prev;
		}

		to_destroy->next = NULL;
		sprintf(feedback_msg, "Deleted: %s", to_destroy->name);
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
		sprintf(feedback_msg, "Deleted: %s", to_destroy->name);
		destroy_task(to_destroy);
	} else if(selected_task == root_task && root_task->next != NULL) {
		root_task = root_task->next;
		root_task->prev = NULL;

		selected_task->next = NULL;
		sprintf(feedback_msg, "Deleted: %s", selected_task->name);
		destroy_task(selected_task);

		selected_task = root_task;
	} else {
		return;
	}

	visual_mode = true;
	save_task_to_file(true);
	update_scroll();
}
