#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <map>
#include <sys/stat.h>

#include "byte_buffer.hpp"

using namespace std;

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
    uint32_t get_root_cluster_addr()  { return root_cluster_addr; }

  private:
    uint16_t sector_size;
    uint16_t cluster_size;
    uint16_t rsvd_sector_cnt;
    uint8_t fat_no;
    uint32_t fat_offset;
    uint32_t fat_sector_no;
    uint32_t fat_area_size;
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
    void set_end_cluster_no(vector<uint32_t> clusters)
    {
      for (int i = start_cluster_no; i < clusters.size(); i++) {
        if (clusters.at(i) == 0x0FFFFFFF) {
          end_cluster_no = i;
          return;
        }
      }
    }

    void set_end_cluster_no()
    {
      end_cluster_no = file_size;
    }

    void set_path(string path) { this->path = path; }

    bool is_deleted_file()
    {
      uint8_t first_byte = static_cast<uint8_t>(file_name_hex & 0xFF);
      return (first_byte == 0xE5);
    }

  public:
    string get_file_name()          { return file_name; }
    string get_file_ext()           { return file_ext; }
    uint8_t get_attribute()         { return attribute; }
    uint32_t get_start_cluster_no() { return start_cluster_no; }
    uint32_t get_end_cluster_no()   { return end_cluster_no; }
    uint32_t get_file_size()        { return file_size; }
    vector<DirectoryEntry> get_children() { return children; }

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
    vector<DirectoryEntry> children;
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
        cerr << "error";

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
    void build(sys::io::byte_buffer bb)
    {
      uint32_t root_inode_addr = cal_data_offset(super_block->get_root_cluster_addr()); // 0x400000

      DirectoryEntry root_dir(bb);
      root_dir.set_path("root_inode");

      root_dir.get_children() = 

      // Directory Entry
      // char direntry_buffer[32] = {1}; // contains all 1's as dummy values

      // string root_dir = "root_inode";

      // fstream ifs("FAT32_simple.mdf");
      // read_dir(direntry_buffer, root_inode_addr, root_inode_addr, ifs, fat_area, super_block, root_dir);
      // ifs.close();
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
    uint32_t cal_data_offset(uint32_t cluster_no)
    {
      uint32_t data_area_addr = super_block->get_fat_offset() + super_block->get_fat_area_size();
      uint32_t data_offset = data_area_addr + ((cluster_no - super_block->get_root_cluster_addr()) * super_block->get_cluster_size());
      return data_offset;
    }
  
  private:
    SuperBlock* super_block;
    FatArea* fat_area;
};

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
    res += tolower(s[i]);
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

uint32_t cal_data_offset(uint32_t cluster_no, SuperBlock super_block)
{
  uint32_t data_area_addr = super_block.get_fat_offset() + super_block.get_fat_area_size();
  uint32_t data_offset = data_area_addr + ((cluster_no - 0x2) * super_block.get_cluster_size());

  return data_offset;
}

void read_dir(char *direntry_buffer, const uint32_t data_area_offset, uint32_t direntry_offset, fstream &ifs, FatArea fat_area, SuperBlock super_block, string file_path)
{
  // while not end of data, 0x00000000
  uint32_t end_of_data_marker = 0x00000000;

  while (memcmp(direntry_buffer, &end_of_data_marker, sizeof(end_of_data_marker)) != 0)
  {
    ifs.seekg(direntry_offset, ios::beg);
    ifs.read(direntry_buffer, 32);

    DirectoryEntry direntry((uint8_t*)direntry_buffer, 32);

    uint8_t attribute = direntry.get_attribute();
    string file_name = direntry.get_file_name();
    rtrim(file_name);
    
    if (file_name.compare(".") == 0 || file_name.compare("..") == 0) { // skip "." or ".."
      direntry_offset += 0x20;
      continue;
    } else if (attribute == 0x10) { // if dir, then recursivly traverse
      read_dir(direntry_buffer, data_area_offset, data_area_offset + ((direntry.get_start_cluster_no() - 0x2) * super_block.get_cluster_size()), ifs, fat_area, super_block, file_path + "/" + file_name);
      memset(direntry_buffer, 1, strlen(direntry_buffer) + 1); // reset buffer with dummy values

      direntry_offset += 0x20;
      continue;
    } else if (attribute != 0x20) { // if not file, then skip: {Hidden, Volume Label, LFN}
      direntry_offset += 0x20;
      continue;
    }

    direntry.set_end_cluster_no(fat_area.get_clusters());
    uint32_t direntry_start_addr = cal_data_offset(direntry.get_start_cluster_no(), super_block);
    
    uint32_t direntry_end_addr = direntry_start_addr + direntry.get_file_size(); // set end cluster number based on file size
    // uint32_t direntry_end_addr = cal_data_offset(direntry.get_end_cluster_no(), super_block);
    if (direntry.is_deleted_file()) {
      file_name = file_name.substr(1, file_name.size());
    }

    uint32_t direntry_file_size = direntry_end_addr - direntry_start_addr;

    vector<char> output_buffer(direntry_file_size);
    ifs.seekg(direntry_start_addr, ios::beg);
    ifs.read(&output_buffer[0], direntry_file_size);

    sys::io::byte_buffer output_bb((uint8_t*)output_buffer.data(), 0, (int)direntry_file_size);
    string data = output_bb.get_ascii(direntry_file_size);

    string file_ext = tolower(direntry.get_file_ext());
    file_name +=  + "." + file_ext; // append file extension to file name

    export_file(file_path, file_name, data);

    direntry_offset += 0x20; // directory entry size
  }
}

int main(int argc, char* argv[])
{
  FAT32 fat32("FAT32_simple.mdf");
  fat32.build();

  return 0;
}
