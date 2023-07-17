#include <cassert>
#include "util.h"
#include "boot_record.h"
#include "dir_entry.h"
#include "fat_table.h"
#include <fstream>
#include <string>
#include <vector>
#include <iostream>

using namespace std;

void print_boot(BootRecord br);
void print_dir(DirectoryEntry de);
void print_fat(FatTable ft);

class Node
{
public:
    Node(ifstream& ifs, DirectoryEntry* de, FatTable* ft)
    {
        int idx = de->first_cluster_no;

        while (ft->fat[idx] != 0xfffffff)
        {
            cluster_no.push_back(idx);
            idx = ft->fat[idx];
        }
    }

    bool ExportTo(string path, ifstream& ifs)
    {
        ofstream ofs(path, ios::binary);

        for (auto& cluster : cluster_no)
        {
            ifs.seekg(cluster * ifs.tellg(), ios::beg);
            char buffer[0x1000];
            ifs.read(buffer, sizeof(buffer));
            ofs.write(buffer, sizeof(buffer));
        }

        return true;
    }

// private:
    vector<uint32_t> cluster_no;
};

class Fat32
{
public:
    Fat32(ifstream& ifs_) : ifs(ifs_)
    {
        /* init Boot Record */
        char buffer[0x200] = { 0 };
        ifs.read(buffer, 0x200);

        br = new BootRecord(buffer);

        /* create FAT Table */
        ifs.seekg(br->fat1_area, ios_base::beg);
        vector<char> fat_bytes(br->fat_size);
        ifs.read(&fat_bytes[0], br->fat_size);

        ft = new FatTable(fat_bytes, br->fat_size);

        /* print data */
        print_boot(*br);
        print_fat(*ft);
    }

    /* methods */
    void BuildFileSystem()
    {
        /*
        1. make root node
        2. read two lines(32bytes) and make nodes
            put under root as children
        3. as the root, if file leave it (empty children)
            - if dir, recursively read and connect

        save clusters(ex. 7 ~ 96) in a node stream
        cluster size is 0x1000
        */

        root = new Node(ifs, nullptr, ft);
    }

    Node* GetNode(const string& path)
    {
        vector<string> path_components;
        string component;

        for (char c : path)
        {
            if (c == '/')
            {
                if (!component.empty())
                {
                    path_components.push_back(component);
                    component.clear();
                }
            }
            else
            {
                component += c;
            }
        }

        if (!component.empty())
        {
            path_components.push_back(component);
        }

        Node* current_node = root;

        for (const auto& dir_name : path_components)
        {
            DirectoryEntry* found_entry = nullptr;

            for (auto& cluster : current_node->cluster_no)
            {
                ifs.seekg(cluster * br->bytes_per_sector * br->sectors_per_cluster, ios::beg);

                for (int i = 0; i < br->bytes_per_sector * br->sectors_per_cluster / sizeof(DirectoryEntry); ++i)
                {
                    DirectoryEntry de;
                    ifs.read(reinterpret_cast<char*>(&de), sizeof(DirectoryEntry));

                    if (de.name == dir_name && de.attr == 0x10) // Check if it's a directory
                    {
                        found_entry = &de;
                        break;
                    }
                }

                if (found_entry != nullptr)
                {
                    break;
                }
            }

            if (found_entry != nullptr)
            {
                current_node = new Node(ifs, found_entry, ft);
            }
            else
            {
                return nullptr; // Directory not found
            }
        }

        return current_node;
    }

private:
    BootRecord* br;
    FatTable* ft;
    Node* root;
    ifstream& ifs;
};

/* main */
int main(int argc, char* argv[])
{
    assert(argc == 2 && "<./exec> <filename>");

    ifstream ifs(argv[1], ios_base::binary);
    Fat32 fat32(ifs);

    fat32.BuildFileSystem();
    auto node = fat32.GetNode("/DIRT/LEAF.JOP");
    if (node != nullptr)
    {
        node->ExportTo("/Leaf.jpg", ifs);
    }
    else
    {
        cout << "File not found!" << endl;
    }

    return 0;
}

void print_boot(BootRecord br)
{
    cout << "\n";
    cout << "b per sec\t" << "-\t" << hex << br.bytes_per_sector << endl;
    cout << "sec per cl\t" << "-\t" << hex << br.sectors_per_cluster << endl;
    cout << "cl size\t\t" << "-\t" << hex << br.cluster_sizes << endl;
    cout << "# of fat\t" << "-\t" << hex << br.fat_count << endl;
    cout << "rev area\t" << "-\t0x" << hex << br.reserved_area << endl;
    cout << "fat1 area\t" << "-\t0x" << hex << br.fat1_area << endl;
    cout << "data area\t" << "-\t0x" << hex << br.data_block_area << endl;
}

void print_dir(DirectoryEntry de)
{
    cout << "\n";
    cout << "attribute\t" << "-\t" << de.attr << endl;
    cout << "name of file\t" << "-\t" << de.name << endl;
    cout << "# of 1st cl\t" << "-\t" << de.first_cluster_no << endl;
    cout << "size of file\t" << "-\t" << hex << de.file_size << endl;
}

void print_fat(FatTable ft)
{
    cout << "\n";
    cout << "7th FAT cl\t" << "-\t" << hex << ft.fat[7] << endl;
    cout << "96th FAT cl\t" << "-\t" << hex << ft.fat[0x96] << endl;
}
