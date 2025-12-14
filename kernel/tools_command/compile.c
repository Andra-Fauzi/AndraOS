#include "../shell.h"
#include "fat.h"
#include "terminal.h" 
// vga_text seems to not be in the file list I saw earlier (terminal.c/h was in drivers), so using terminal.h for kprint if needed or vga_text if it exists. 
// From earlier list_dir drivers: terminal.c, terminal.h. No vga_text.h.
// fat.c uses "terminal.h"
// But compile.c line 3 was #include "../drivers/   vga_text.h". 
// Let's assume user meant terminal.h or it's provided elsewhere. 
// Wait, I should check if vga_text exists. I only saw drivers listing. 
// Drivers listing: ata, elf, fat, keyboard, sleep, terminal, timer. No vga_text.
// So I should probably include terminal.h instead of vga_text.h if kprint is there.

// Defined in shell.h: int subc_compile(char *src, int src_len, char *dest, int dest_max, int *dest_len);

extern uint16_t active_cluster;

void c_compile(char* buffer, int length, multiboot_info_t *mb_info) {
    char args[3][128];
    // expect: compile input.c output.elf
    substr(buffer, ' ', length, args);
    
    // int is_test = 0;
    // if (args[1][0] == 't' && args[1][1] == 'e' && args[1][2] == 's' && args[1][3] == 't' && args[1][4] == '\0') is_test = 1;

    // if (is_test) {
    //     kprint("Running compiler test...\n", mb_info);
    //     char *src = "int main() { return 65; }";
    //     char dest[1024];
    //     int dest_len = 0;
    //     int ret = subc_compile(src, 25, dest, 1024, &dest_len);
    //     if (ret == 0) {
    //          kprint("Test compilation success!\n", mb_info);
    //          kprint("Output size: ", mb_info);
    //          char num[12]; to_string(dest_len, num); kprint(num, mb_info); kprint("\n", mb_info);
    //          kprint("Header: ", mb_info);
    //          for(int i=0; i<4; i++) {
    //              char h[3];
    //              unsigned char b = (unsigned char)dest[i];
    //              char *hex = "0123456789ABCDEF";
    //              h[0] = hex[(b >> 4) & 0xF];
    //              h[1] = hex[b & 0xF];
    //              h[2] = 0;
    //              kprint(h, mb_info); kprint(" ", mb_info);
    //             }
    //             writefile(0, "halo.elf", dest, dest_len);
    //          kprint("\n", mb_info);
    //     } else {
    //          kprint("Test compilation failed.\n", mb_info);
    //     }
    //     return;
    // }

    if (args[1][0] == '\0' || args[2][0] == '\0') {
        kprint("Usage: compile <input.c> <output.elf>\n", mb_info);
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
    char *dest_buffer = (char*)malloc(256 * 1024); // 256KB output buffer
    if (!dest_buffer) {
        kprint("Failed to allocate output buffer\n", mb_info);
        // free(src_buffer); // Memory leak if we don't have free, but acceptable for this stage
        return;
    }
    
    size_t dest_len = 0;
    
    kprint("Compiling...\n", mb_info);
    
    // 3. Compile
    int ret = subc_compile(src_buffer, src_len, dest_buffer, 64 * 1024, &dest_len);
    
    if (ret == 0) {
        kprint("Compilation successful! Size: ", mb_info);
        char size_str[16];
        to_string(dest_len, size_str);
        kprint(size_str, mb_info);
        kprint("\n", mb_info);
        
        // 4. Write output file
        kprint("Writing to output file: ", mb_info);
        kprint(output_file, mb_info);
        kprint("\n", mb_info);
        
        writefile(active_cluster, output_file, dest_buffer, dest_len);
        
        kprint("Done.\n", mb_info);
    } else {
        kprint("Compilation failed. Error code: ", mb_info);
        char err_str[16];
        to_string(ret, err_str);
        kprint(err_str, mb_info);
        kprint("\n", mb_info);
    }
    
    // Free buffers if possible
    // free(dest_buffer);
    // free(src_buffer);
}
