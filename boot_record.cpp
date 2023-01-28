#include "boot_record.h"
#include "util.h"
#include <fstream>
#include <iomanip>
#include <iostream>

using namespace std;

BootRecord::BootRecord(char* buffer)
{
    bytes_per_sector = io::to_le2(buffer + 0x0b);
    sectors_per_cluster = io::to_le1(buffer + 0x0d);
    cluster_sizes = bytes_per_sector * sectors_per_cluster;
    fat_count = io::to_le1(buffer + 0x10);
    fat1_area = io::to_le2(buffer + 0x0e) * bytes_per_sector;
    reserved_area = io::to_le4(buffer + 0x24) * fat_count * bytes_per_sector;
    data_block_area = fat1_area + reserved_area;
    fat_size = reserved_area * bytes_per_sector;
}