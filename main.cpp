#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <map>
#include <cstring>
#include <sys/stat.h>

#include "byte_buffer.hpp"

using namespace std;


inline std::string& rtrim(std::string& s, const char* t = " \t\n\r\f\v")
{
  s.erase(s.find_last_not_of(t) + 1);
  return s;
}

// overload of tolower(char) function
string tolower(string s)
{
  string res = "";

  for (int i = 0; i < s.size(); i++) {
    res += tolower(s.at(i));
  }

  return res;
}

bool create_dirs(string file_path)
{
  istringstream iss(file_path);
  string directory;
  string curr_path = "";

  while (std::getline(iss, directory, '/'))
  {
    curr_path += directory + "/";

    struct stat info;
    if (stat(curr_path.c_str(), &info) != 0) {
        if (mkdir(curr_path.c_str(), 0777) != 0) {
            std::cerr << "Error creating directory: " << strerror(errno) << std::endl;
            return false;
        }
    }
  }

  return true;
}

void export_file(string file_path, string file_name, string file_data)
{
  if (!create_dirs(file_path)) {
    cerr << "error";
  }

  ofstream ofs(file_path + "/" + file_name);

  if (!ofs.good())
    cerr << "error";

  ofs << file_data;

  ofs.close();
}


class SuperBlock
{
  public:
    SuperBlock() {}

    SuperBlock(uint8_t *buffer, int size)
    {
      sys::io::byte_buffer bb((uint8_t*)buffer, 0, size);
      bb.skip(0x0B);

      sector_size = bb.get_uint16_le();
      uint8_t sector_per_cluster = bb.get_uint8();
      cluster_size = sector_size * sector_per_cluster;
      rsvd_sector_cnt = bb.get_uint16_le();
      
      fat_no = bb.get_uint8();
      bb.skip(0x13);
      fat_offset = rsvd_sector_cnt * sector_size;
      fat_sector_no = bb.get_uint32_le();
      fat_area_size = fat_sector_no * sector_size * fat_no;

      data_area_addr = fat_offset + fat_area_size;
      bb.skip(0x04);
      root_cluster_addr = bb.get_uint32_le();
    }

  public:
    uint16_t get_cluster_size()       { return cluster_size; }
    uint16_t get_rsvd_sector_cnt()    { return rsvd_sector_cnt; }
    uint16_t get_sector_size()        { return sector_size; }
    uint32_t get_fat_offset()         { return fat_offset; }
    uint32_t get_fat_sector_no()      { return fat_sector_no; }
    uint32_t get_fat_area_size()      { return fat_area_size; }
    uint32_t get_data_area_addr()     { return data_area_addr; }
    uint32_t get_root_cluster_addr()  { return root_cluster_addr; }

  private:
    uint16_t sector_size;
    uint16_t cluster_size;
    uint16_t rsvd_sector_cnt;
    uint8_t fat_no;
    uint32_t fat_offset;
    uint32_t fat_sector_no;
    uint32_t fat_area_size;
    uint32_t data_area_addr;
    uint32_t root_cluster_addr;
};

class FatArea
{
  public:
    FatArea() {}

    FatArea(uint8_t *buffer, int size)
    {
      sys::io::byte_buffer bb((uint8_t*)buffer, 0, size);

      int entry_cnt = size / 4;
      clusters_vec.reserve(entry_cnt);

      for (int i = 0; i < entry_cnt; i++) {
        clusters_vec.push_back(bb.get_uint32_le());
      }
    }

  public:
    vector<uint32_t> get_clusters() { return clusters_vec; }

  private:
    vector<uint32_t> clusters_vec;
};

class DirectoryEntry
{
  public:
    DirectoryEntry() {}

    DirectoryEntry(sys::io::byte_buffer& bb)
    {
      file_name_hex = bb.get_uint64_le();
      bb.reset();

      file_name = bb.get_ascii(8);
      file_ext = bb.get_ascii(3);
      attribute = bb.get_uint8();

      bb.skip(0x08);
      start_cluster_hi = bb.get_uint16_le();
      bb.skip(0x04);
      start_cluster_lo = bb.get_uint16_le();

      start_cluster_no = ((uint32_t)start_cluster_hi << 16) | start_cluster_lo;

      file_size = bb.get_uint32_le();
    }

    DirectoryEntry(uint8_t *buffer, int size)
    {
      sys::io::byte_buffer bb((uint8_t*)buffer, 0, size);

      file_name_hex = bb.get_uint64_le();
      bb.reset();

      file_name = bb.get_ascii(8);
      file_ext = bb.get_ascii(3);
      attribute = bb.get_uint8();

      bb.skip(0x08);
      start_cluster_hi = bb.get_uint16_le();
      bb.skip(0x04);
      start_cluster_lo = bb.get_uint16_le();

      // Combine cluster numbers
      start_cluster_no = ((uint32_t)start_cluster_hi << 16) | start_cluster_lo;

      file_size = bb.get_uint32_le();
    }

  public:
    bool is_deleted_file()
    {
      uint8_t first_byte = static_cast<uint8_t>(file_name_hex & 0xFF);
      return (first_byte == 0xE5);
    }

  public:
    void set_end_cluster_no(vector<uint32_t> clusters)
    {
      for (int i = start_cluster_no; i < clusters.size(); i++) {
        if (clusters.at(i) == 0x0FFFFFFF) {
          end_cluster_no = i;
          return;
        }
      }
    }

    void add_child(DirectoryEntry *child)
    {
      children.push_back(child);
    }

    void set_end_cluster_no() { end_cluster_no = file_size; }
    void set_file_name(string str) { file_name = str; }
    void set_path(string path) { this->path = path; }

    string get_file_name()          { return file_name; }
    string get_file_ext()           { return file_ext; }
    uint8_t get_attribute()         { return attribute; }
    uint32_t get_start_cluster_no() { return start_cluster_no; }
    uint32_t get_end_cluster_no()   { return end_cluster_no; }
    uint32_t get_file_size()        { return file_size; }
    vector<DirectoryEntry*> get_children() { return children; }

  private:
    string file_name;
    uint64_t file_name_hex;
    string file_ext;
    uint8_t attribute;
    uint16_t start_cluster_hi;
    uint16_t start_cluster_lo;
    uint32_t start_cluster_no;
    uint32_t end_cluster_no;
    uint32_t file_size;

    string path;
    vector<DirectoryEntry*> children;
};

class Node
{
  public:
    void set_size(uint32_t size) { this->size = size; }
    void set_extents(map<uint32_t, uint32_t> extents)
    {
      this->extents = extents;
    }

  private:
    uint32_t size;
    map<uint32_t, uint32_t> extents;
};

class FAT32
{
  public:
    FAT32(string path)
    {
      fstream ifs(path);
      if (!ifs.good())
        cerr << "error reading file";

      // Super Block/Boot Record
      char buffer[96] = {0};
      ifs.read(buffer, 96);
      super_block = new SuperBlock((uint8_t*)buffer, 96);

      // FAT area
      vector<char> fat_buffer(super_block->get_fat_area_size());
      ifs.seekg(super_block->get_fat_offset(), ios::beg);
      ifs.read(&fat_buffer[0], super_block->get_fat_area_size());
      fat_area = new FatArea((uint8_t*)fat_buffer.data(), super_block->get_fat_area_size());

      ifs.close();
    }

  public:
    void build()
    {
      root_dir = new DirectoryEntry();
      root_dir->set_path("root_inode");

      build_dir_tree(root_dir, super_block->get_data_area_addr() - 0x20);
    }

    Node to_node(DirectoryEntry dentry)
    {
      Node node = Node();
      node.set_size(dentry.get_file_size());
      node.set_extents(to_extents(dentry.get_start_cluster_no()));
      return node;
    }

    map<uint32_t, uint32_t> to_extents(uint32_t cluster_no)
    {
      map<uint32_t, uint32_t> extents;

      uint32_t idx = cluster_no;
      while (fat_area->get_clusters().at(idx) != 0x0FFFFFFF)
      {
        extents.insert({fat_area->get_clusters().at(idx), super_block->get_cluster_size()});
        // Change to linked list-like ds
        // fat_area->get_clusters().at(idx)
        idx++;
      }

      return extents;
    }
  
  private:
    void build_dir_tree(DirectoryEntry *parent_entry, uint32_t parent_entry_addr) {
      uint32_t child_entry_addr = parent_entry_addr + 0x20; // directory entry size

      while (true) {
        fstream ifs("FAT32_simple.mdf");
        ifs.seekg(child_entry_addr, ios::beg);
        char child_direntry_buffer[32] = {0};
        ifs.read(child_direntry_buffer, 32);
        ifs.close();

        // Check for the end of the children list
        if (is_end_of_directory(child_direntry_buffer)) {
          break;
        }

        DirectoryEntry* child_direntry = new DirectoryEntry((uint8_t*)child_direntry_buffer, 32);
        uint8_t attribute = child_direntry->get_attribute();
        string file_name = child_direntry->get_file_name();
        rtrim(file_name);

        if (file_name.compare(".") == 0 || file_name.compare("..") == 0) { // skip "." or ".."
          child_entry_addr += 0x20;
          continue;
        } else if (attribute == 0x10) { // if dir, then recursivly traverse
          build_dir_tree(child_direntry, cal_data_offset(child_direntry->get_start_cluster_no()));
        } else if (attribute != 0x20) { // if not file, then skip: {Hidden, Volume Label, LFN}
          child_entry_addr += 0x20;
          continue;
        }

        parent_entry->add_child(child_direntry);

        child_entry_addr += 0x20;
      }
    }

    bool is_end_of_directory(char *buffer)
    {
      const char end_marker[32] = {0};
      return (std::memcmp(buffer, end_marker, sizeof(end_marker)) == 0);
    }

    uint32_t cal_data_offset(uint32_t cluster_no)
    {
      uint32_t data_offset = super_block->get_data_area_addr() + ((cluster_no - super_block->get_root_cluster_addr()) * super_block->get_cluster_size());
      return data_offset;
    }
  
  private:
    SuperBlock *super_block;
    FatArea *fat_area;
    DirectoryEntry *root_dir;
};

int main(int argc, char* argv[])
{
  FAT32 fat32("FAT32_simple.mdf");
  fat32.build();

  return 0;
}
