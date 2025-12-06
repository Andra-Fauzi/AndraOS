#include "tools_command/ls.h"

extern uint16_t active_cluster;

void c_ls(char *buffer, int length, multiboot_info_t * mb_info) {
	list_dir(active_cluster);
}
