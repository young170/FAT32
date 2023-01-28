#include "dir_entry.h"
#include "util.h"
#include <fstream>
#include <string>
#include <iostream>

using namespace std;

DirectoryEntry::DirectoryEntry(char* buffer)
{
    attr = io::to_le1(buffer + 0x0b);	// read attribute
    if ((attr & 0x10) == 0x10)
    {
        is_dir = true;	// found a directory
    }
    else if((attr & 0x20) == 0x20)
    {
        is_dir = false;	// not a directory
    }

    string temp_name(buffer, 0, 8);	// first 8 bytes
    name = temp_name;
    
    cluster_hi = io::to_le2(buffer + 0x14);
    cluster_lo = io::to_le2(buffer + 0x1a);

    first_cluster_no = (cluster_hi << 16) | cluster_lo;	// shift left 2 and concat

    file_size = io::to_le4(buffer + 0x1c);	// size of file
}