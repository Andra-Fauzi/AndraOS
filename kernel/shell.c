#include "shell.h"

void init_shell(multiboot_info_t *mb_info) {
	clear_screen(mb_info);
	shell_mode = true;
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
	char *help = "help\0";
	char *clear = "clear\0";
	char *read_sector = "readsector\0";
	char *write_sector = "writesector\0";
	if (cmp(help, buffer) == 0) {
		kprint("clear : buat membersihkan layar\n", mb_info);
	} else if (cmp(clear, buffer) == 0) {
		clear_screen(mb_info);
	} else if (cmp(read_sector, buffer) == 0) {
		kprint("sektor dari 1-", mb_info);
		char* total = char_total_sectors();
		kprint(total, mb_info);
		kprint("\n", mb_info);
		kprint("ketik angka untuk sector\n", mb_info);
		char *input = read_input(mb_info);
		int i = 0;
		int sector = 0;
		while(input[i] != '\0') {
			if(input[i] >= '0' && input[i] <= '9') {
				sector = sector * 10 + (input[i] - '0');
			} else if(input[i] == '\n') {
				kprint("sector sudah dapat\n", mb_info);
			} else {
				kprint("itu bukan angka\n", mb_info);
			}
			i++;
		}
		
		kprint("membaca sector\n", mb_info);
		char buffer_read[512];
		if(sector-1 >= 0 && sector-1 < ata_get_total_sectors()) {
			int status = ata_read_sector(sector-1, buffer_read);
			if(status == 0) {
				kprint("sukses baca sector\n", mb_info);
				kprint(buffer_read, mb_info);
			} else {
				kprint("gagal baca sector\n", mb_info);
			}
		} else {
			kprint("batas sectornya 1-", mb_info);
			kprint(char_total_sectors(), mb_info);
			kprint("!\n", mb_info);
		}
	} else if (cmp(write_sector, buffer) == 0) {
		kprint("sektor dari 1-", mb_info);
		kprint(char_total_sectors(), mb_info);
		kprint("\n", mb_info);
		kprint("ketik angka untuk sector\n", mb_info);
		char *input = read_input(mb_info);
		int i = 0;
		int sector = 0;
		while(input[i] != '\0') {
			if(input[i] >= '0' && input[i] <= '9') {
				sector = sector * 10 + (input[i] - '0');
			} else if(input[i] == '\n') {
				kprint("sector sudah dapat\n", mb_info);
			} else {
				kprint("itu bukan angka\n", mb_info);
			}
			i++;
		}
		
		kprint("tolong tulis untuk sectornya\n", mb_info);
		char *write = read_input(mb_info);

		if(sector-1 >= 0 && sector-1 < ata_get_total_sectors()) {
			int status = ata_write_sector(sector-1, write);
			if(status == 0) {
				kprint("sukses tulis ke sector\n", mb_info);
			} else {
				kprint("gagal tulis ke sector\n", mb_info);
			}
		} else {
			kprint("batas sectornya 1-", mb_info);
			kprint(char_total_sectors(), mb_info);
			kprint("!\n", mb_info);
		}
	} else {
		kprint("command apa tuh? coba help\n", mb_info);
	}
}



char command_buffer[256];
int length_command = 0;

void shell_run(multiboot_info_t *mb_info) {
	if(terminal_x < 1 && terminal_y < 1) {
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
	else if(character) {
		print_char(character, mb_info);
		command_buffer[length_command] = character;
		length_command++;
	}
	if(character == '\n') {
		command_buffer[length_command] = '\0';
		print_char(character, mb_info);
		execute_command(command_buffer, length_command, mb_info);
		while(length_command > 0) {
			command_buffer[length_command] = 0;
			length_command--;
		}
		print_char('>', mb_info);
	}
}

