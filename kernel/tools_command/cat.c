#include "cat.h"

extern uint16_t active_cluster;

void c_cat(char *buffer, int length, multiboot_info_t *mb_info) {
	char command_args[3][128];
	substr(buffer, ' ', length, command_args);
	char name[11];
	memcpy(name, command_args[1], length - 4);
	int hasil = find_cluster(active_cluster, name);
	if(hasil == -1) {
		kprint("file tidak di temukan\n", mb_info);
		return;
	}
	int size = 0;
	uint8_t *file = readfile(active_cluster, command_args[1], &size);
	kprint(file, mb_info);
}
