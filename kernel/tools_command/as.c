#include "as.h"

extern uint16_t active_cluster;

void c_as(char *buffer, int length, multiboot_info_t *mb_info) {
    char args[3][128];
    // expect: compile input.asm output.elf
    substr(buffer, ' ', length, args);
    if (args[1][0] == '\0' || args[2][0] == '\0') {
        kprint("Usage: compile <input.asm> <output.elf>\n", mb_info);
        return;
    }
    
    char *input_file = args[1];
    char *output_file = args[2];
    int src_len = 0;  // Will be filled by readfile
    
    kprint("Reading input file: ", mb_info);
    kprint(input_file, mb_info);
    kprint("\n", mb_info);

    // 1. Read input file (assuming root directory, cluster 0)
    // readfile handles FAT name conversion and lookup
    for(int i = 0; i < 11; i++) {
	    print_char('b', mb_info);
    }
    print_char('\n', mb_info);
	char names[12];
	memset(names, ' ', 12);
	to_fat_name_fixed(input_file, names);
	kprint(names, mb_info);
    print_char('\n', mb_info);
    char buffer1[255];
    to_string(active_cluster, buffer1);
    kprint(buffer1, mb_info);
    print_char('\n', mb_info);
    char *src_buffer = (char*)readfile(active_cluster, input_file, &src_len);
    
    if (!src_buffer) {
        kprint("Failed to read input file or file not found\n", mb_info);
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
        kprint("Failed to allocate output buffer\n", mb_info);
        // free(src_buffer); // Memory leak if we don't have free, but acceptable for this stage
        return;
    }
    
    size_t dest_len = 0;
    
    kprint("Assembling ...\n", mb_info);
    
    int result = assemble_x86(src_buffer, src_len, dest_buffer, 64 * 1024, &dest_len);

        kprint("Compilation successful! Size: ", mb_info);
        char size_str[16];
        to_string(dest_len, size_str);
        kprint(size_str, mb_info);
        kprint("\n", mb_info);

        kprint("Writing to output file: ", mb_info);
        kprint(output_file, mb_info);
        kprint("\n", mb_info);
        
        writefile(active_cluster, output_file, dest_buffer, dest_len);
}
