#include "tools_command/cd.h"

extern uint16_t active_cluster;
extern char current_dir[255];

void c_cd(char *buffer, int length, multiboot_info_t *mb_info) {
	char command_args[3][128];
	substr(buffer, ' ', length, command_args);
	if(command_args[1][0] != '\0') {
		int hasil = find_cluster(active_cluster, command_args[1], length-3);
		if(hasil == -1) {
			kprint("folder tidak di temukan\n", mb_info);
		}
		else if(memcmp(command_args[1], "..", 2) == 0) {
			active_cluster = hasil;
			int i = 255;
			while(current_dir[i] != '\\') {
				current_dir[i] = 0;
				i--;
			}
			current_dir[i] = '\0';
		}
		else {
			active_cluster = hasil;
			if(memcmp(command_args[1], ".", 1) != 0) {
				int i = 0;
				while(current_dir[i] != '\0') {
					i++;
				}
				current_dir[i] = '\\';
				i++;
				memcpy(current_dir+i, command_args[1], length-3);
				current_dir[i + length-3] = '\0';
			}
		}
	}
}	
