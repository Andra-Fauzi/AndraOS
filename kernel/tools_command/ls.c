#include "tools_command/ls.h"

extern uint16_t active_cluster;

void c_ls(char *buffer, int length) {
	list_dir(active_cluster);
}
