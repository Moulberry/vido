# vido (vee-doo) - a simple vim-inspired hierachical to-do manager

## Dependencies
NCURSES should usually come pre-installed on your distribution, if it is not, try installing:  
ncurses (arch)  
libncurses5-dev (debian)  

## How to build:
$ git clone https://github.com/Moulberry/vido  
$ cd vido  
$ sudo make clean install  

## How to run:
$ vido  

## Ensure your todos save/load
Create the folder `~/.vido` for vido to save it's `task.json` to  
You can change this location by modifying `TASK_JSON_LOCATION` in `src/config.h`  
Vido will not create the folders for you, but it will create the json file

## Basic usage
In visual mode, you can select different tasks using the arrow keys or vim keys  
Pressing l/Right will complete tasks & expand groups  
Pressing h/Left will uncomplete tasks & unexpand groups  
Pressing ENTER will toggle completion & expansion  
Pressing m will enter "move" mode, allowing tasks to be rearranged using the arrow keys or vim keys  

## Command basics
Most of the interaction with the actual todo list is performed with vim-like commands  
Enter command mode by typing a colon  
You can return to visual mode pressing ESC  

## Commands
:q or :quit - Close vido  
:a (task name) - Appends new task as a sibling of the currently selected task  
:o (task name) - Appends new task as a child of the currently selected task  
:d - Deletes currently selected task, including all children  
:r (new name) - Renames currenly selected task  
:w - Saves the tasks file. This is not necessary with AUTOSAVE enabled  

## Customization
Customizing vido is done through editing source files (like dwm, st, etc.)  

General tweaks and keybinds/commands can be set up in `src/config.h`  
If you wish to add functions to be used by keybinds/commands, it is recommended you put the functions in `src/commands.c`  
Most of the visuals should be customizable through editing `src/render.c`  
If you wish to modify the functionality of the todo-list (eg. scrolling, saving, etc.), edit `src/vido.c`  

## Patches
If you have a customization that you would like to share with others; or you would like to see the patches that others have shared, head to https://github.com/Moulberry/vido/wiki/Patches
