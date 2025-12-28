#include "shell.h"

char command_buffer[256];
int length_command = 0;
uint16_t active_cluster = 0;
char current_dir[255];
command_args_t args[10];
int args_count = 0;

extern multiboot_info_t* multiboot_info;


void add_args(char *name, void (*function)(char *, int), int length_command) {
	args[args_count].length_command = length_command;
	args[args_count].function = function;
	memcpy(args[args_count].name, name, length_command);
	args_count++;
}

void init_shell() {
	clear_screen();
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
	add_args("elf", c_elf, 3);
	add_args("compile", c_compile, 7);
	add_args("cat", c_cat, 3);
	add_args("createfile", c_create_file, 10);
	add_args("as", c_as, 2);
	add_args("shutdown", c_shutdown, 8);
	add_args("reboot", c_reboot, 6);
	kprint(current_dir);
	print_char('>');
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
char* read_input() {
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
			print_char(character);
		}
		if(character == '\n') {
			buffer[length] = '\0';
			break;
		}
	}
	return buffer;
}

void execute_command(char* buffer, int length) {
	if(length == 0) {
		return;
	}
	char command_args[3][128];
	substr(buffer, ' ', length, command_args);
	bool found = false;
	for(int i = 0; i < args_count; i++) {
		if((memcmp(args[i].name, command_args[0], args[i].length_command)) == 0) {
			args[i].function(buffer, length);
			found = true;
			break;
		}
	}

	if(found == false) {
		kprint("command not found try help\n");
	}
}

void shell_run() {
	if(shell_mode == false) {
		return;
	}
	
	char character;
	while((character = shell_getchar()) == -1) {
		asm volatile("hlt");
	}
	if (character == '\b' && length_command > 0) {
		command_buffer[length_command] = 0;
		length_command--;
		print_char(character);
	} 
	else if(character == '\n') {
		print_char(character);
		execute_command(command_buffer, length_command);
		while(length_command > 0) {
			command_buffer[length_command] = 0;
			length_command--;
		}
		kprint(current_dir);
		print_char('>');
	}
	else if(character != '\b' && character != '\n') {
		print_char(character);
		command_buffer[length_command] = character;
		length_command++;
	}
}

