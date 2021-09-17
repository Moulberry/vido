# vido - work in progress

## How to build:
$ sudo make

$ sudo make install

$ vido

## Ensure your todos save/load
Create the folder `~/.vido` for vido to save it's task.json to

You can change this location by modifying TASK_JSON_LOCATION in src/vido.c

Vido will not create the folders for you, but it will create the json file

## Basic usage
In visual mode, you can select different tasks using the arrow keys or vim keys

Pressing L/Right will complete tasks & expand groups

Pressing H/Left will uncomplete tasks & unexpand groups

Pressing ENTER will toggle completion & expansion

## Command basics
Most of the interaction with the actual todo list is performed with vim-like commands

Enter command mode by typing a colon

You can return to visual mode using ESC

## Commands
:a (task name) - Appends new task as a sibling

:o (task name) - Appends new task as a child

:d - Deletes currently selected task, including all children

:up (n) - Navigate up

:down (n) - Navigate down

## Searching
Coming soon - maybe?

