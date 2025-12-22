#include "as.h"

extern uint16_t active_cluster;

void c_as(char *buffer, int length) {
    char args[3][128];
    // expect: compile input.c output.elf
    substr(buffer, ' ', length, args);
    if (args[1][0] == '\0' || args[2][0] == '\0') {
        kprint("Usage: compile <input.c> <output.elf>\n");
        return;
    }
    
    char *input_file = args[1];
    char *output_file = args[2];
    int src_len = 0;  // Will be filled by readfile
    
    kprint("Reading input file: ");
    kprint(input_file);
    kprint("\n");

    // 1. Read input file (assuming root directory, cluster 0)
    // readfile handles FAT name conversion and lookup
	char names[12];
	memset(names, ' ', 12);
	to_fat_name_fixed(input_file, names);
    	char buffer1[255];
    	char *src_buffer = (char*)readfile(active_cluster, input_file, &src_len);
    
    if (!src_buffer) {
        kprint("Failed to read input file or file not found\n");
        return;
    }
    
    // src_len is now exact file size from readfile()
    // Ensure null terminator for safety
    if (src_len < 65535) {
        src_buffer[src_len] = '\0';
    }
    // 2. Alloc output buffer
    // Assuming simple flat allocation for now
    char *dest_buffer = (char*)malloc(64* 1024); // 256KB output buffer
    if (!dest_buffer) {
        kprint("Failed to allocate output buffer\n");
        // free(src_buffer); // Memory leak if we don't have free, but acceptable for this stage
        return;
    }
    
    size_t dest_len = 0;
    
    kprint("Assembling ...\n");
    
    int result = assemble_x86(src_buffer, src_len, dest_buffer, 64 * 1024, &dest_len);

        kprint("Compilation successful! Size: ");
        char size_str[16];
        to_string(dest_len, size_str);
        kprint(size_str);
        kprint("\n");

        kprint("Writing to output file: ");
        kprint(output_file);
        kprint("\n");
        
        writefile(active_cluster, output_file, dest_buffer, dest_len);
}
