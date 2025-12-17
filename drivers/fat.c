#include "fat.h"

extern multiboot_info_t *multiboot_info;

fat_BS_t boot_record;
uint32_t total_sectors;
uint32_t fat_size;
uint32_t root_dir_sectors;
uint32_t first_data_sectors;
uint32_t first_fat_sector;
uint32_t data_sectors;
uint32_t total_clusters;
uint32_t first_root_dir_sector;

void to_fat_name(char source[12], char destination[12]) {
	int i = 0;
	int j = 0;
	while(j < 12) {
		if(source[i] == '.') {
			i++;
			j = 8;
		}
		destination[j] = source[i];
		j++;
		i++;
	}
	destination[11] = '\0';
}

void to_fat_name_83(const char *src, char dest[12]) {
    // Inisialisasi dengan spaces
    for (int i = 0; i < 11; i++)
        dest[i] = ' ';
    dest[11] = '\0';

    // Bagian nama (8 char)
    int di = 0;
    int si = 0;

    // salin nama sampai '.' atau sampai 8
    while (src[si] && src[si] != '.' && di < 8) {
        char c = src[si++];
        dest[di++] = (unsigned char)c;
    }

    // kalau ada '.', lompat
    if (src[si] == '.')
        si++;

    // Bagian ekstensi (3 char)
    di = 8;
    int ei = 0;
    while (src[si] && ei < 3) {
        char c = src[si++];
        dest[di++] = (unsigned char)c;
        ei++;
    }
}

void to_fat_name_fixed(const char *source, char dest[11]) {
    // Reset semua ke space
    for (int i = 0; i < 11; i++) dest[i] = ' ';

    int j = 0;
    int dot_seen = 0;
    for (int i = 0; source[i] != '\0' && j < 11; i++) {
        char c = source[i];
        if (c == '.') {
            j = 8;  // pindah ke ekstensi
            dot_seen = 1;
            continue;
        }
        // uppercase
        if (c >= 'a' && c <= 'z') c -= 32;
        dest[j++] = c;
    }
}


fat_BS_t parse_BS(const char *buffer) {
	fat_BS_t _boot_record;
	memcpy(&_boot_record, buffer, sizeof(fat_BS_t));
	return _boot_record;
}

void init_fat16() {
	char buffer[512];
	char buffer1[512];
	ata_read_sector(2048, buffer);
	boot_record = parse_BS(buffer);
	total_sectors = boot_record.total_sector_16;
	
	to_string(total_sectors, buffer1);
	kprint("total_sectors: ", multiboot_info);
	kprint(buffer1, multiboot_info);
	print_char('\n', multiboot_info);
	
	fat_size = boot_record.table_size_16;

	to_string(fat_size, buffer1);
	kprint("fat_size: ", multiboot_info);
	kprint(buffer1, multiboot_info);
	print_char('\n', multiboot_info);

	root_dir_sectors = ((boot_record.root_entry_count * 32) + (boot_record.bytes_per_sector - 1)) / boot_record.bytes_per_sector;

	to_string(root_dir_sectors, buffer1);
	kprint("root_dir_sectors: ", multiboot_info);
	kprint(buffer1, multiboot_info);
	print_char('\n', multiboot_info);

	first_data_sectors = boot_record.reserved_sector_count + (boot_record.table_count * fat_size) + root_dir_sectors;
	
	to_string(first_data_sectors, buffer1);
	kprint("first_data_sectors: ", multiboot_info);
	kprint(buffer1, multiboot_info);
	print_char('\n', multiboot_info);

	first_fat_sector = boot_record.reserved_sector_count;
	
	to_string(first_fat_sector, buffer1);
	kprint("first_fat_sector: ", multiboot_info);
	kprint(buffer1, multiboot_info);
	print_char('\n', multiboot_info);

	data_sectors = total_sectors - (boot_record.reserved_sector_count + (boot_record.table_count * fat_size) + root_dir_sectors);
	
	to_string(data_sectors, buffer1);
	kprint("data_sectors: ", multiboot_info);
	kprint(buffer1, multiboot_info);
	print_char('\n', multiboot_info);

	total_clusters = data_sectors / boot_record.sector_per_cluster;
	
	to_string(total_clusters, buffer1);
	kprint("total_clusters: ", multiboot_info);
	kprint(buffer1, multiboot_info);
	print_char('\n', multiboot_info);

	first_root_dir_sector = boot_record.reserved_sector_count + (boot_record.table_count * fat_size);

	to_string(first_root_dir_sector, buffer1);
	kprint("first_root_dir_sector: ", multiboot_info);
	kprint(buffer1, multiboot_info);
	print_char('\n', multiboot_info);
}

uint16_t readFATTable(uint16_t active_cluster) {
    // Pastikan cluster valid
    if (active_cluster < 2 || active_cluster >= total_clusters + 2) {
        return 0xFFFF; // EOF / invalid
    }

    // FAT16: 2 bytes per entry
    uint32_t fat_offset = active_cluster * 2;

    // sector di FAT yang mengandung entri
    uint32_t fat_sector = first_fat_sector + (fat_offset / boot_record.bytes_per_sector);

    // offset dalam sektor
    uint32_t ent_offset = fat_offset % boot_record.bytes_per_sector;

    // baca sektor FAT
    uint8_t sector_data[boot_record.bytes_per_sector]; // asumsikan bytes_per_sector = 512
    ata_read_sector(2048 + fat_sector, sector_data);

    // ambil 2 byte little-endian
    uint16_t table_value = sector_data[ent_offset] | (sector_data[ent_offset + 1] << 8);

    // Validasi EOF / bad cluster
    if (table_value >= 0xFFF8) table_value = 0xFFFF; // EOF

    return table_value;
}

void writeFATTable(uint16_t active_cluster, uint16_t value) {
    if(active_cluster < 2 || active_cluster >= total_clusters) {
        return;
    }
    // FAT16: 2 bytes per entry
    uint32_t fat_offset = active_cluster * 2;

    // sector di FAT yang mengandung entri
    uint32_t fat_sector = first_fat_sector + (fat_offset / boot_record.bytes_per_sector);

    // offset dalam sektor
    uint32_t ent_offset = fat_offset % boot_record.bytes_per_sector;

    // baca sektor FAT
    uint8_t sector_data[boot_record.bytes_per_sector]; // asumsikan bytes_per_sector = 512
    ata_read_sector(2048 + fat_sector, sector_data);

    uint8_t value_lo = (uint8_t)(value & 0xFF);
    uint8_t value_hi = (uint8_t)((value >> 8) & 0xFF); 

    memcpy(&sector_data[ent_offset], &value_lo, sizeof(uint8_t));
    memcpy(&sector_data[ent_offset+1], &value_hi, sizeof(uint8_t));

    ata_write_sector(2048 + fat_sector, sector_data);
}

void readFATuntilEOC(uint16_t active_cluster) {
	uint16_t table_value = active_cluster;
	char buffer[512];
	while(!(table_value >= 0xFFF8)) {
		uint32_t lba = cluster_to_LBA(table_value);
		for(int i = 0;boot_record.sector_per_cluster; i++)
		{
			ata_read_sector(2048 + lba, buffer);
			kprint(buffer, multiboot_info);
		}
		table_value = readFATTable(active_cluster);
	}
	print_char('\n', multiboot_info);
}

void readFATuntil10(uint16_t active_cluster) {
	uint16_t table_value = active_cluster;
	char buffer[512];
	int i = 0;
	while(!(table_value >= 0xFFF8 || i > 10)) {
		uint32_t lba = cluster_to_LBA(table_value);
		ata_read_sector(2048 + lba, buffer);
		kprint(buffer, multiboot_info);
		table_value = readFATTable(active_cluster);
		i++;
	}
	print_char('\n', multiboot_info);
}

void list_dir(uint16_t active_cluster) {
    kprint("Listing Directory for cluster:\n", multiboot_info);
    char buffer[20];
    to_string(active_cluster, buffer);
    kprint("Cluster: ", multiboot_info);
    kprint(buffer, multiboot_info);
    print_char('\n', multiboot_info);

    if (active_cluster < 2) {
        // Root directory
        list_root_dir();
        return;
    }

    // Baca cluster direktori
    uint32_t lba = cluster_to_LBA(active_cluster);
    for(int j = 0; j < boot_record.sector_per_cluster; j++) {

        uint8_t sector_data[boot_record.bytes_per_sector];
        ata_read_sector(2048 + lba + j, sector_data);

        uint32_t entries_per_sector = boot_record.bytes_per_sector / sizeof(fat_dir_entry_t);
        fat_dir_entry_t *entries = (fat_dir_entry_t *)sector_data;
        for (uint32_t i = 0; i < entries_per_sector; i++) {
            fat_dir_entry_t *e = &entries[i];

            // jika entry kosong (nama byte pertama = 0) → tidak ada lagi
            if (e->name[0] == 0x00) {
                return;
            }

            // jika entry dihapus (0xE5) → skip
            if ((uint8_t)e->name[0] == 0xE5) {
                continue;
            }

            // dapat atribut
            uint8_t attr = e->attr;

            // cek apakah folder
            bool is_dir = (attr & 0x10) != 0;

            // ambil nama 8.3 sebagai string
            char name[12];
            for (int j = 0; j < 11; j++) {
                name[j] = e->name[j];
            }
            name[11] = '\0';

            // ambil first cluster dan size
            uint16_t first_cluster = e->first_cluster_lo;
            uint32_t size = e->file_size;

            // print (atau kprint di OS kamu)
            if (is_dir) {
                kprint("[DIR] ", multiboot_info);
            } else {
                kprint("[FILE] ", multiboot_info);
            }
            kprint(name, multiboot_info);

            char bufnum[20];
            to_string(first_cluster, bufnum);
            kprint(" clst=", multiboot_info);
            kprint(bufnum, multiboot_info);
            to_string(size, bufnum);
            kprint(" size=", multiboot_info);
            kprint(bufnum, multiboot_info);
            print_char('\n', multiboot_info);
        }
    }
}

void list_root_dir() {
    kprint("Listing Root Directory:\n", multiboot_info);
    uint8_t buf[512];
    uint32_t entries_per_sector = boot_record.bytes_per_sector / sizeof(fat_dir_entry_t);
    for (uint32_t s = 0; s < root_dir_sectors; s++) {
        kprint("Reading sector: \n", multiboot_info);
        ata_read_sector(2048 + first_root_dir_sector + s, buf);
        fat_dir_entry_t *entries = (fat_dir_entry_t *)buf;

        for (uint32_t i = 0; i < entries_per_sector; i++) {
            fat_dir_entry_t *e = &entries[i];

            // jika entry kosong (nama byte pertama = 0) → tidak ada lagi
            if (e->name[0] == 0x00) {
                return;
            }

            // jika entry dihapus (0xE5) → skip
            if ((uint8_t)e->name[0] == 0xE5) {
                continue;
            }

            // iapat atribut
            uint8_t attr = e->attr;

            // cek apakah folder
            bool is_dir = (attr & 0x10) != 0;

            // ambil nama 8.3 sebagai string
            char name[12];
            for (int j = 0; j < 11; j++) {
                name[j] = e->name[j];
            }
            name[11] = '\0';

            // ambil first cluster dan size
            uint16_t first_cluster = e->first_cluster_lo;
            uint32_t size = e->file_size;

            // print (atau kprint di OS kamu)
            if (is_dir) {
                kprint("[DIR] ", multiboot_info);
            } else {
                kprint("[FILE] ", multiboot_info);
            }
            kprint(name, multiboot_info);

            char bufnum[20];
            to_string(first_cluster, bufnum);
            kprint(" clst=", multiboot_info);
            kprint(bufnum, multiboot_info);

            to_string(size, bufnum);
            kprint(" size=", multiboot_info);
            kprint(bufnum, multiboot_info);

            print_char('\n', multiboot_info);
        }
    }
}

uint32_t cluster_to_LBA(uint16_t cluster) {
	uint32_t result = first_data_sectors + ((cluster - 2) * boot_record.sector_per_cluster);
    return result;
}



int find_cluster_dir(uint16_t cluster, char *name, int length) {
    // Convert input name to uppercase for FAT16 matching
    char upper_name[11];
    // upper_name[length] = '\0';
    // str_to_upper(upper_name, length);
    to_fat_name_fixed(name, upper_name);
    // kprint(upper_name, multiboot_info);

    if(cluster < 2) {
        uint32_t entries_per_sector = boot_record.bytes_per_sector / sizeof(fat_dir_entry_t);
        for(uint32_t j = 0; j < root_dir_sectors; j++) {
            uint8_t sector_data[boot_record.bytes_per_sector];
            ata_read_sector(2048 + first_root_dir_sector + j, sector_data);
            fat_dir_entry_t *entries = (fat_dir_entry_t *)sector_data;
            for(uint32_t i = 0; i < entries_per_sector; i++) {
                fat_dir_entry_t *e = &entries[i];

                // Skip empty or deleted entries
                // if(e->name[0] == 0x00) return 0;
                // if((uint8_t)e->name[0] == 0xE5) continue;

                // Check if it's a directory (fix operator precedence!)
                if((e->attr & 0x10) != 0) {
                    // Compare first 'length' characters of the name
                    bool found = true;
                    int i = 0;
                    while(i < 11 && e->name[i] != 0x20) {
                        if(e->name[i] != upper_name[i]) {
                            found = false;
                            break;
                        }
                        i++;
                    }
                    if(found) {
                        // kprint("dapat ni: ", multiboot_info);
                        // kprint(upper_name, multiboot_info);
                        // print_char('\n', multiboot_info);
                        // char clusterbuf[255];
                        // to_string(e->first_cluster_lo, clusterbuf);
                        // kprint("cluster: ", multiboot_info);
                        // kprint(clusterbuf, multiboot_info);
                        // print_char('\n', multiboot_info);
                        return e->first_cluster_lo;
                    }
                }
            }
        }
    } else {

        uint32_t lba = cluster_to_LBA(cluster);
        uint32_t entries_per_sector = boot_record.bytes_per_sector / sizeof(fat_dir_entry_t);
        for(uint32_t j = 0; j < boot_record.sector_per_cluster; j++) {
            uint8_t sector_data[boot_record.bytes_per_sector];
            ata_read_sector(2048 + lba + j, sector_data);
            fat_dir_entry_t *entries = (fat_dir_entry_t *)sector_data;
            for(uint32_t i = 0; i < entries_per_sector; i++) {
                fat_dir_entry_t *e = &entries[i];
                
                // Skip empty or deleted entries
                // if(e->name[0] == 0x00) return 0;
                // if((uint8_t)e->name[0] == 0xE5) continue;
                
                // Check if it's a directory (fix operator precedence!)
                if((e->attr & 0x10) != 0) {
                    // Compare first 'length' characters of the name
                    if(memcmp(name, "..", 2) == 0) {
                        // kprint("ini ..", multiboot_info);
                        if(memcmp(e->name, "..", 2) == 0) {
                            // kprint("ini dapat ..", multiboot_info);
                            return e->first_cluster_lo;
                        }
                    }
                    else {

                        bool found = true;
                        int i = 0;
                        while(i < 11 && e->name[i] != 0x20) {
                            if(e->name[i] != upper_name[i]) {
                                found = false;
                                break;
                            }
                            i++;
                        }
                        if(found) {
                            // kprint("dapat ni: ", multiboot_info);
                            // kprint(upper_name, multiboot_info);
                            // print_char('\n', multiboot_info);
                            // char clusterbuf[255];
                            // to_string(e->first_cluster_lo, clusterbuf);
                            // kprint("cluster: ", multiboot_info);
                            // kprint(clusterbuf, multiboot_info);
                            // print_char('\n', multiboot_info);
                            return e->first_cluster_lo;
                        }
                    }
                }
            }
        } 
    }
    kprint("not found: ", multiboot_info);
    kprint(upper_name, multiboot_info);
    print_char('\n', multiboot_info);
    return -1;
}

// char *name_of_cluster(uint16_t cluster) {
//     if(cluster < 2) {
//         return "\0";
//     } else {

//         uint32_t lba = cluster_to_LBA(cluster);
//         uint32_t entries_per_sector = boot_record.bytes_per_sector / sizeof(fat_dir_entry_t);
//         for(uint32_t j = 0; j < boot_record.sector_per_cluster; j++) {
//             uint8_t sector_data[boot_record.bytes_per_sector];
//             ata_read_sector(2048 + lba + j, sector_data);
//             fat_dir_entry_t *entries = (fat_dir_entry_t *)sector_data;
//             for(uint32_t i = 0; i < entries_per_sector; i++) {
//                 fat_dir_entry_t *e = &entries[i];
                
//                 // Skip empty or deleted entries
//                 // if(e->name[0] == 0x00) return 0;
//                 // if((uint8_t)e->name[0] == 0xE5) continue;
                
//                 // Check if it's a directory (fix operator precedence!)
//                 if((e->attr & 0x10) != 0) {
//                     // Compare first 'length' characters of the name
//                     if(memcmp(e->name, upper_name, length) == 0) {
//                         kprint("dapat ni: ", multiboot_info);
//                         kprint(upper_name, multiboot_info);
//                         print_char('\n', multiboot_info);
//                         return e->first_cluster_lo;
//                     }
//                 }
//             }
//         } 
//     }
// }

void create_entry(fat_dir_entry_t entry, uint16_t cluster) {
    uint16_t cluster_real = cluster;
    uint16_t table_value = 1;
    while(readFATTable(cluster_real) != 0x0000) {
        cluster_real++;
    }
    writeFATTable(cluster_real, 0xFFFF);
    char buffer[sizeof(fat_dir_entry_t)];
    char buffer1[512];
    to_string(cluster_real, buffer1);
    kprint("cluster untuk entri ini ada di :", multiboot_info);
    kprint(buffer1, multiboot_info);
    print_char('\n', multiboot_info);
    entry.first_cluster_lo = cluster_real;
    memcpy(buffer, &entry, sizeof(fat_dir_entry_t));
    uint32_t lba = cluster_to_LBA(cluster);
    uint32_t place_sector = 0;
    for(uint32_t i = 0; i < boot_record.sector_per_cluster; i++) {
        uint8_t sector_data[boot_record.bytes_per_sector];
        ata_read_sector(2048 + lba + i, sector_data);
        uint32_t entries_per_sector = boot_record.bytes_per_sector / sizeof(fat_dir_entry_t);
        fat_dir_entry_t *entries = (fat_dir_entry_t *)sector_data;
        for(uint32_t j = 0; j < entries_per_sector; j++) {
            fat_dir_entry_t *e = &entries[j];
            if(e->name[0] == 0x00)
            {
                memcpy(&entries[j], &entry, sizeof(fat_dir_entry_t));
                ata_write_sector(2048 + lba + i, sector_data);
                return;
            }
        }
    }
}

void create_entry_in_root_directory(fat_dir_entry_t entry) {

}

int find_cluster(uint16_t cluster, char name[11]) {
    // Convert input name to uppercase for FAT16 matching
    char upper_name[11];
    // memcpy(upper_name, name, 11);
    // upper_name[length] = '\0';
    // str_to_upper(upper_name, 11);
    to_fat_name_fixed(name, upper_name);
    //kprint(upper_name, multiboot_info);

    if(cluster < 2) {
        uint32_t entries_per_sector = boot_record.bytes_per_sector / sizeof(fat_dir_entry_t);
        for(uint32_t j = 0; j < root_dir_sectors; j++) {
            uint8_t sector_data[boot_record.bytes_per_sector];
            ata_read_sector(2048 + first_root_dir_sector + j, sector_data);
            fat_dir_entry_t *entries = (fat_dir_entry_t *)sector_data;
            for(uint32_t i = 0; i < entries_per_sector; i++) {
                fat_dir_entry_t *e = &entries[i];

                // Skip empty or deleted entries
                // if(e->name[0] == 0x00) return 0;
                // if((uint8_t)e->name[0] == 0xE5) continue;

                // Check if it's a directory (fix operator precedence!)
                if(!((e->attr & 0x10) != 0)) {
                    // Compare first 'length' characters of the name
                    bool found = (memcmp(e->name, upper_name, 11) == 0);
                    if(found) {
                        // kprint("dapat ni: ", multiboot_info);
                        // kprint(upper_name, multiboot_info);
                        // print_char('\n', multiboot_info);
                        // char clusterbuf[255];
                        // to_string(e->first_cluster_lo, clusterbuf);
                        // kprint("cluster: ", multiboot_info);
                        // kprint(clusterbuf, multiboot_info);
                        // print_char('\n', multiboot_info);
                        return e->first_cluster_lo;
                    }
                }
            }
        }
    } else {

        uint32_t lba = cluster_to_LBA(cluster);
        uint32_t entries_per_sector = boot_record.bytes_per_sector / sizeof(fat_dir_entry_t);
        for(uint32_t j = 0; j < boot_record.sector_per_cluster; j++) {
            uint8_t sector_data[boot_record.bytes_per_sector];
            ata_read_sector(2048 + lba + j, sector_data);
            fat_dir_entry_t *entries = (fat_dir_entry_t *)sector_data;
            for(uint32_t i = 0; i < entries_per_sector; i++) {
                fat_dir_entry_t *e = &entries[i];
                
                // Skip empty or deleted entries
                // if(e->name[0] == 0x00) return 0;
                // if((uint8_t)e->name[0] == 0xE5) continue;
                
                // Check if it's a directory (fix operator precedence!)
                if(!((e->attr & 0x10) != 0)) {
                    // Compare first 'length' characters of the name
                    if(memcmp(name, "..", 2) == 0) {
                        // kprint("ini ..", multiboot_info);
                        if(memcmp(e->name, "..", 2) == 0) {
                            // kprint("ini dapat ..", multiboot_info);
                            return e->first_cluster_lo;
                        }
                    }
                    else {

                        bool found = (memcmp(e->name, upper_name, 11) == 0);
                        if(found) {
                            // kprint("dapat ni: ", multiboot_info);
                            // kprint(upper_name, multiboot_info);
                            // print_char('\n', multiboot_info);
                            // char clusterbuf[255];
                            // to_string(e->first_cluster_lo, clusterbuf);
                            // kprint("cluster: ", multiboot_info);
                            // kprint(clusterbuf, multiboot_info);
                            // print_char('\n', multiboot_info);
                            return e->first_cluster_lo;
                        }
                    }
                }
            }
        } 
    }
    kprint("not found: ", multiboot_info);
    kprint(upper_name, multiboot_info);
    print_char('\n', multiboot_info);
    return -1;
}

fat_dir_entry_t get_entry_file(uint16_t cluster, char name[11]) {
    // Convert input name to uppercase for FAT16 matching
    char upper_name[11];
    // memcpy(upper_name, name, 11);
    // upper_name[length] = '\0';
    // str_to_upper(upper_name, 11);
    to_fat_name_fixed(name, upper_name);
    // kprint(upper_name, multiboot_info);

    if(cluster < 2) {
        uint32_t entries_per_sector = boot_record.bytes_per_sector / sizeof(fat_dir_entry_t);
        for(uint32_t j = 0; j < root_dir_sectors; j++) {
            uint8_t sector_data[boot_record.bytes_per_sector];
            ata_read_sector(2048 + first_root_dir_sector + j, sector_data);
            fat_dir_entry_t *entries = (fat_dir_entry_t *)sector_data;
            for(uint32_t i = 0; i < entries_per_sector; i++) {
                fat_dir_entry_t *e = &entries[i];

                // Skip empty or deleted entries
                // if(e->name[0] == 0x00) return 0;
                // if((uint8_t)e->name[0] == 0xE5) continue;

                // Check if it's a directory (fix operator precedence!)
                if(!((e->attr & 0x10) != 0)) {
                    // Compare first 'length' characters of the name
                    bool found = (memcmp(e->name, upper_name, 11) == 0);
                    if(found) {
                        // kprint("dapat ni: ", multiboot_info);
                        // kprint(upper_name, multiboot_info);
                        // print_char('\n', multiboot_info);
                        // char clusterbuf[255];
                        // to_string(e->first_cluster_lo, clusterbuf);
                        // kprint("cluster: ", multiboot_info);
                        // kprint(clusterbuf, multiboot_info);
                        // print_char('\n', multiboot_info);
                        return *e;
                    }
                }
            }
        }
    } else {

        uint32_t lba = cluster_to_LBA(cluster);
        uint32_t entries_per_sector = boot_record.bytes_per_sector / sizeof(fat_dir_entry_t);
        for(uint32_t j = 0; j < boot_record.sector_per_cluster; j++) {
            uint8_t sector_data[boot_record.bytes_per_sector];
            ata_read_sector(2048 + lba + j, sector_data);
            fat_dir_entry_t *entries = (fat_dir_entry_t *)sector_data;
            for(uint32_t i = 0; i < entries_per_sector; i++) {
                fat_dir_entry_t *e = &entries[i];
                
                // Skip empty or deleted entries
                // if(e->name[0] == 0x00) return 0;
                // if((uint8_t)e->name[0] == 0xE5) continue;
                
                // Check if it's a directory (fix operator precedence!)
                if(!((e->attr & 0x10) != 0)) {
                    // Compare first 'length' characters of the name
                    if(memcmp(name, "..", 2) == 0) {
                        // kprint("ini ..", multiboot_info);
                        if(memcmp(e->name, "..", 2) == 0) {
                            // kprint("ini dapat ..", multiboot_info);
                            return *e;
                        }
                    }
                    else {

                        bool found = (memcmp(e->name, upper_name, 11) == 0);
                        if(found) {
                            // kprint("dapat ni: ", multiboot_info);
                            // kprint(upper_name, multiboot_info);
                            // print_char('\n', multiboot_info);
                            // char clusterbuf[255];
                            // to_string(e->first_cluster_lo, clusterbuf);
                            // kprint("cluster: ", multiboot_info);
                            // kprint(clusterbuf, multiboot_info);
                            // print_char('\n', multiboot_info);
                            return *e;
                        }
                    }
                }
            }
        } 
    }
    print_char('\n', multiboot_info);
    kprint("not found: ", multiboot_info);
    kprint(upper_name, multiboot_info);
    print_char('\n', multiboot_info);
}

uint8_t *readfile(uint16_t active_cluster, char real_name[12], int *out_size) {
	char name[12];
	memset(name, ' ', 12);
	to_fat_name_fixed(real_name, name);
	int cluster = find_cluster(active_cluster, name);
	if(cluster == -1) {
		kprint("tidak ditemukan\n", multiboot_info);
		if (out_size) *out_size = 0;
		return NULL;
	}
	fat_dir_entry_t entry = get_entry_file(active_cluster, name);
	char buffer[512];
	kprint("besar filenya :", multiboot_info);
	to_string(entry.file_size, buffer);
	kprint(buffer, multiboot_info);
	kprint("\n", multiboot_info);
	char *file = (char *)malloc(entry.file_size);
	int size = 0;
	uint16_t table_value = entry.first_cluster_lo;
	while(size < entry.file_size && !(table_value >= 0xFFF8)) {
		int lba = cluster_to_LBA(table_value);
		char buffer[512];
		bool done = false;
		for(int i = 0; i < boot_record.sector_per_cluster; i++) {
			ata_read_sector(2048 + lba + i, buffer);
			for(int j = 0; j < 512; j++) {
				file[size] = buffer[j];
				size++;
				if(size >= entry.file_size) {
					done = true;
					break;
				}
			}
			if(done == true) {
				break;
			}
		}
		if(done == true) {
			break;
		}
		table_value = readFATTable(table_value);
	}
	if (out_size) *out_size = size;
	return file;
}

uint16_t find_free_cluster() {
    for (uint16_t i = 2; i < total_clusters + 2; i++) {
        if (readFATTable(i) == 0x0000) {
            return i;
        }
    }
    return 0xFFFF; // Full
}

void writefile(uint16_t parent_cluster, const char *name, void *buffer, uint32_t size) {
    char fat_name[11];
    to_fat_name_fixed(name, fat_name);

    if (find_cluster(parent_cluster, fat_name) != -1) {
        kprint("File already exists\n", multiboot_info);
        return;
    }

    uint32_t bytes_per_cluster = boot_record.sector_per_cluster * boot_record.bytes_per_sector;
    uint32_t num_clusters = (size + bytes_per_cluster - 1) / bytes_per_cluster;
    if (num_clusters == 0) num_clusters = 1;

    uint16_t first_cluster = 0xFFFF;
    uint16_t prev_cluster = 0xFFFF;
    uint16_t allocated_clusters[num_clusters]; // track untuk rollback
    uint8_t *buf_ptr = (uint8_t *)buffer;
    uint32_t remaining_size = size;

    for (uint32_t i = 0; i < num_clusters; i++) {
        uint16_t current_cluster = find_free_cluster();
        if (current_cluster == 0xFFFF) {
            kprint("Disk Full, rollback\n", multiboot_info);
            // rollback
            for (uint32_t j = 0; j < i; j++) writeFATTable(allocated_clusters[j], 0x0000);
            return;
        }

        allocated_clusters[i] = current_cluster;

        if (first_cluster == 0xFFFF) first_cluster = current_cluster;
        if (prev_cluster != 0xFFFF) writeFATTable(prev_cluster, current_cluster);
        prev_cluster = current_cluster;

        uint32_t lba = cluster_to_LBA(current_cluster);
        uint8_t sector_buf[512];

        for (int s = 0; s < boot_record.sector_per_cluster; s++) {
            memset(sector_buf, 0, 512);
            uint32_t copy_size = (remaining_size < 512) ? remaining_size : 512;
	    //uint32_t copy_size = 512;
	    if (remaining_size > 0) {
                memcpy(sector_buf, buf_ptr, copy_size);
		// kprint("sector buffer : ", multiboot_info);
		// kprint(sector_buf, multiboot_info);
		// kprint("\n", multiboot_info);
                buf_ptr += copy_size;
                remaining_size -= copy_size;
            }
            ata_write_sector(2048 + lba + s, sector_buf);
        }
    }

    // set EOF di cluster terakhir
    writeFATTable(prev_cluster, 0xFFFF);

    // buat directory entry
    fat_dir_entry_t new_entry;
    memset(&new_entry, 0, sizeof(new_entry));
    memcpy(new_entry.name, fat_name, 11);
    new_entry.attr = 0x20; // file
    new_entry.first_cluster_lo = first_cluster;
    new_entry.file_size = size;

    // tulis ke directory
    if (parent_cluster < 2) {
        // root dir
        uint32_t entries_per_sector = boot_record.bytes_per_sector / sizeof(fat_dir_entry_t);
        for (uint32_t s = 0; s < root_dir_sectors; s++) {
            uint8_t sector_data[512];
            ata_read_sector(2048 + first_root_dir_sector + s, sector_data);
            fat_dir_entry_t *entries = (fat_dir_entry_t *)sector_data;
            for (uint32_t e = 0; e < entries_per_sector; e++) {
                if (entries[e].name[0] == 0x00 || (uint8_t)entries[e].name[0] == 0xE5) {
                    entries[e] = new_entry;
                    ata_write_sector(2048 + first_root_dir_sector + s, sector_data);
                    return;
                }
            }
        }
    } else {
        // subdirectory
        uint16_t p_cluster = parent_cluster;
        while (p_cluster < 0xFFF8) {
            uint32_t lba = cluster_to_LBA(p_cluster);
            uint32_t entries_per_sector = boot_record.bytes_per_sector / sizeof(fat_dir_entry_t);
            for (int s = 0; s < boot_record.sector_per_cluster; s++) {
                uint8_t sector_data[512];
                ata_read_sector(2048 + lba + s, sector_data);
                fat_dir_entry_t *entries = (fat_dir_entry_t *)sector_data;
                for (uint32_t e = 0; e < entries_per_sector; e++) {
                    if (entries[e].name[0] == 0x00 || (uint8_t)entries[e].name[0] == 0xE5) {
                        entries[e] = new_entry;
                        ata_write_sector(2048 + lba + s, sector_data);
                        return;
                    }
                }
            }
            uint16_t next = readFATTable(p_cluster);
            if (next >= 0xFFF8) break;
            p_cluster = next;
        }
    }
    kprint("Directory full\n", multiboot_info);
}
