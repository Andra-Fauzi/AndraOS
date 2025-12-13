#include "create_file.h"

extern uint16_t active_cluster;

void c_create_file(char *buffer, int length, multiboot_info_t *mb_info) {
    char args[3][128];
    substr(buffer, ' ', length, args);

    if (args[1][0] == 0) {
        kprint("usage: create <name> <content>\n", mb_info);
        return;
    }

    // Convert to FAT 8.3
    char fat_name[11];
    to_fat_name_fixed(args[1], fat_name);

    // Ensure active_cluster is directory

    if (find_cluster(active_cluster, fat_name) != -1) {
        kprint("file sudah ada\n", mb_info);
        return;
    }

    char *content = args[2];
    int size = content ? strlen(content) : 0;

    writefile(active_cluster, fat_name, content, size);
    kprint("Done.\n", mb_info);
}


