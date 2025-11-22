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
		ata_read_sector(2048 + lba, buffer);
		kprint(buffer, multiboot_info);
		table_value = readFATTable(active_cluster);
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

uint32_t cluster_to_LBA(uint16_t cluster) {
	uint32_t result = first_data_sectors + ((cluster - 2) * boot_record.sector_per_cluster);
    return result;
}



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
