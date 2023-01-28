#include "fat_table.h"
#include "util.h"
#include <fstream>
#include <string>
#include <iostream>

using namespace std;

FatTable::FatTable(vector<char> fat_bytes, int fat_size)
{
	for (int i = 0; i < fat_size; i += 4)
	{
		char entry = io::to_le4((char*) &fat_bytes[i]);	// init
		fat.push_back(entry);
	}
}