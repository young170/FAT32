#pragma once

class BootRecord
{
    public:
		BootRecord(char* buffer);

        int bytes_per_sector;       // to_le2, 0x0b
        int sectors_per_cluster;    // to_le1, 0x0d
        int cluster_sizes;			// bytes_per_sector * sectors_per_clusters
        int fat_count;              // 0x10, 2
        int fat_size;

        int reserved_area;          // 0x215c00
        int fat1_area;
        int data_block_area;        // 0x400000
};