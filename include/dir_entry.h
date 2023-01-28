#pragma once
#include <iostream>

using namespace std;

class DirectoryEntry
{
    public:
		DirectoryEntry(char* buffer);

        int attr;

		bool is_dir = { false };

        string name;

		int cluster_hi;
		int cluster_lo;
		int first_cluster_no;

		int file_size;
};