#include "shell.h"

char command_buffer[256];
int length_command = 0;
uint16_t active_cluster = 0;
char current_dir[255];
command_args_t args[5];


void init_shell(multiboot_info_t *mb_info) {
	clear_screen(mb_info);
	memset(current_dir, 0, sizeof(current_dir));
	current_dir[0] = 'r';
	current_dir[1] = 'o';
	current_dir[2] = 'o';
	current_dir[3] = 't';
	current_dir[4] = '\0';
	shell_mode = true;
	memset(args, 0, sizeof(args) * 5);
	memset(args[0].name, 0, sizeof(char) * 255);
	memcpy(args[0].name, "cd\0", sizeof(char) * 2);
	args[0].function = c_cd;
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

/*
void execute_command(char* buffer, int length, multiboot_info_t *mb_info) {
	char *help = "help\0";
	char *clear = "clear\0";
	char *read_sector = "readsector\0";
	char *write_sector = "writesector\0";
	char *read_fat = "readfat\0";
	char *read_cluster = "readcluster\0";
	char *list_root = "lsroot\0";
	char *listdir = "lsdir\0";
	char *createentry = "mkentry\0";
	if (cmp(help, buffer) == 0) {
		kprint("clear : buat membersihkan layar\n", mb_info);
	} else if(cmp(createentry, buffer) == 0) {
		kprint("ketik cluster untuk dilist\n", mb_info);
		char *input = read_input(mb_info);
		int i = 0;
		uint16_t cluster = 0;
		while(input[i] != '\0') {
			if(input[i] >= '0' && input[i] <= '9') {
				cluster = cluster * 10 + (input[i] - '0');
			} else if(input[i] == '\n') {
				kprint("cluster sudah dapat\n", mb_info);
			} else {
				kprint("itu bukan angka\n", mb_info);
			}
			i++;
		}
		fat_dir_entry_t test;

		memset(&test, 0, sizeof(test));

		memset(test.name, ' ', 11);

		test.name[0] = 'T';
		test.name[1] = 'E';
		test.name[2] = 'S';
		test.name[3] = 'T';
		test.name[8] = 'I';
		test.name[9] = 'N';
		test.name[10] = 'I';

		test.attr = 0x20;

		create_entry(test, cluster);
	} else if(cmp(listdir, buffer) == 0) {
		kprint("ketik cluster untuk dilist\n", mb_info);
		char *input = read_input(mb_info);
		int i = 0;
		uint16_t cluster = 0;
		while(input[i] != '\0') {
			if(input[i] >= '0' && input[i] <= '9') {
				cluster = cluster * 10 + (input[i] - '0');
			} else if(input[i] == '\n') {
				kprint("cluster sudah dapat\n", mb_info);
			} else {
				kprint("itu bukan angka\n", mb_info);
			}
			i++;
		}
		kprint("melist directory di cluster itu\n", mb_info);
		list_dir(cluster);
	} else if(cmp(read_cluster, buffer) == 0) {
		kprint("ketik cluster untuk dibaca\n", mb_info);
		char *input = read_input(mb_info);
		int i = 0;
		uint16_t cluster = 0;
		while(input[i] != '\0') {
			if(input[i] >= '0' && input[i] <= '9') {
				cluster = cluster * 10 + (input[i] - '0');
			} else if(input[i] == '\n') {
				kprint("cluster sudah dapat\n", mb_info);
			} else {
				kprint("itu bukan angka\n", mb_info);
			}
			i++;
		}
		kprint("membaca data di cluster itu\n", mb_info);
		uint32_t lba = cluster_to_LBA(cluster);

		char buffer[512];

		to_string(cluster, buffer);
		kprint("reading cluster: ", mb_info);
		kprint(buffer, mb_info);
		print_char('\n', mb_info);
		to_string(lba, buffer);
		kprint("reading lba: ", mb_info);
		kprint(buffer, mb_info);
		print_char('\n', mb_info);
		ata_read_sector(2048 + lba, buffer);
		kprint("reading data: \n", mb_info);
		kprint(buffer, mb_info);
		print_char('\n', mb_info);

	} else if(cmp(read_fat, buffer) == 0) {
		kprint("ketik cluster untuk dibaca dari FAT table\n", mb_info);
		char *input = read_input(mb_info);
		int i = 0;
		int cluster = 0;
		while(input[i] != '\0') {
			if(input[i] >= '0' && input[i] <= '9') {
				cluster = cluster * 10 + (input[i] - '0');
			} else if(input[i] == '\n') {
				kprint("cluster sudah dapat\n", mb_info);
			} else {
				kprint("itu bukan angka\n", mb_info);
			}
			i++;
		}
		readFATuntilEOC(cluster);
	} else if(cmp(list_root, buffer) == 0) {
		list_root_dir();
	} 
	else if (cmp(clear, buffer) == 0) {
		clear_screen(mb_info);
	} else if (cmp(read_sector, buffer) == 0) {
		kprint("sektor dari 0-", mb_info);
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
		if(sector >= 0 && sector < ata_get_total_sectors()) {
			int status = ata_read_sector(sector, buffer_read);
			if(status == 0) {
				kprint("sukses baca sector\n", mb_info);
				kprint(buffer_read, mb_info);
			} else {
				kprint("gagal baca sector\n", mb_info);
			}
		} else {
			kprint("batas sectornya 0-", mb_info);
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
*/


void execute_command(char* buffer, int length, multiboot_info_t *mb_info) {
	if(length == 0) {
		return;
	}
	char command_args[3][128];
	substr(buffer, ' ', length, command_args);
	if(memcmp(command_args[0], "ls", length) == 0) {
		list_dir(active_cluster);
	}
	else if (memcmp(command_args[0], "cd", 2) == 0) {
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
	else if(memcmp(command_args[0], "clear", length) == 0) {
		clear_screen(mb_info);
	}
	else {
		kprint("command not found?, try help\n", mb_info);
	}
}

void shell_run(multiboot_info_t *mb_info) {
	if(terminal_x < 1 && terminal_y) {
		kprint(current_dir, mb_info);
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
	else if(character == '\n') {
		print_char(character, mb_info);
		execute_command(command_buffer, length_command, mb_info);
		while(length_command > 0) {
			command_buffer[length_command] = 0;
			length_command--;
		}
		kprint(current_dir, mb_info);
		print_char('>', mb_info);
	}
	else if(character != '\b' && character != '\n') {
		print_char(character, mb_info);
		command_buffer[length_command] = character;
		length_command++;
	}
}

