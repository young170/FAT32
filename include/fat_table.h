#pragma once
#include <vector>

using namespace std;

class FatTable
{
    public:
		FatTable(vector<char> fat_bytes, int fat_size);

        vector<uint32_t> fat;
};