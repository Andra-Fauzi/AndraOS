#include "shell.h"

char command_buffer[256];
int length_command = 0;
uint16_t active_cluster = 0;
char current_dir[255];
command_args_t args[5];
int args_count = 0;


void add_args(char *name, void (*function)(char *, int, multiboot_info_t *), int length_command) {
	args[args_count].length_command = length_command;
	args[args_count].function = function;
	memcpy(args[args_count].name, name, length_command);
	args_count++;
}

void init_shell(multiboot_info_t *mb_info) {
	clear_screen(mb_info);
	memset(current_dir, 0, sizeof(current_dir));
	current_dir[0] = 'r';
	current_dir[1] = 'o';
	current_dir[2] = 'o';
	current_dir[3] = 't';
	current_dir[4] = '\0';
	shell_mode = true;
	memset(args, 0, sizeof(args));
	add_args("cd", c_cd, 2);
	add_args("ls", c_ls, 2);
}

char shell_getchar() {
	char sc = keyboard_getchar();
	if (sc == -1) return -1;

	char c = keyboard_map[sc];
	//if(c) {
	//	print_char(c);
	//}
	return c;
}

int cmp(char* source, char* destination) {
	int i = 0;
	int cmp = 0;
	while(source[i]) {
		if(source[i] != destination[i]) {
			cmp++;
		}
		i++;
	}
	return cmp;
}
char* read_input(multiboot_info_t *mb_info) {
	static char buffer[512];
	int length = 0;
	char character;
	while(true) {
        	while((character = shell_getchar()) == -1) {
			asm volatile("hlt");
		}
		if(character == '\b') {
			buffer[length] = 0;
			length--;
		} 
		if(character) {
			buffer[length] = character;
			length++;
			print_char(character, mb_info);
		}
		if(character == '\n') {
			buffer[length] = '\0';
			break;
		}
	}
	return buffer;
}

void execute_command(char* buffer, int length, multiboot_info_t *mb_info) {
	if(length == 0) {
		return;
	}
	char command_args[3][128];
	substr(buffer, ' ', length, command_args);
	bool found = false;
	for(int i = 0; i < args_count; i++) {
		if((memcmp(args[i].name, command_args[0], args[i].length_command)) == 0) {
			args[i].function(buffer, length, mb_info);
			found = true;
			break;
		}
	}

	if(found == false) {
		kprint("command not found try help\n", mb_info);
	}
}

void shell_run(multiboot_info_t *mb_info) {
	if(shell_mode == false) {
		return;
	}
	if(terminal_x < 1 && terminal_y) {
		kprint(current_dir, mb_info);
		print_char('>', mb_info);
	}
	char character;
	while((character = shell_getchar()) == -1) {
		asm volatile("hlt");
	}
	if (character == '\b' && length_command > 0) {
		command_buffer[length_command] = 0;
		length_command--;
		print_char(character, mb_info);
	} 
	else if(character == '\n') {
		print_char(character, mb_info);
		execute_command(command_buffer, length_command, mb_info);
		while(length_command > 0) {
			command_buffer[length_command] = 0;
			length_command--;
		}
		kprint(current_dir, mb_info);
		print_char('>', mb_info);
	}
	else if(character != '\b' && character != '\n') {
		print_char(character, mb_info);
		command_buffer[length_command] = character;
		length_command++;
	}
}

