#pragma once

#include <cstdint>
#include <string>
#include <initializer_list>

////////////////////////////////////////////////////////////////////////////////
//
// byte_buffer is basically, a shallow version of byte buffer.
//
// But, It can own buffer memory
//
////////////////////////////////////////////////////////////////////////////////
namespace sys::io {

  using namespace std;
  
  class byte_buffer
  {
  public: 
    byte_buffer();
    byte_buffer(uint8_t* data, int offset, int count, bool owner=false);
    byte_buffer(uint8_t* data, int count, bool owner=false);
    explicit byte_buffer(string && src);
    byte_buffer(initializer_list<uint8_t> l);
    byte_buffer(byte_buffer &&) noexcept;

    auto operator=(byte_buffer const& rhs) -> byte_buffer&;

   ~byte_buffer();

  public:
    auto set_owner() -> byte_buffer&;

    auto resize(int to) -> int;
    auto append(byte_buffer const& b) -> byte_buffer&;
    auto append(byte_buffer* b) -> byte_buffer&;
    auto append(uint8_t* buffer, int offset, int count) -> int;
    
    auto get_ascii() const -> string;
    auto get_ascii(int size) const -> string;

    auto get_unicode16_le(int size) const -> string;

    auto to_s(int from=-1, int to=-1) const -> string;

    auto get_int8(int at=-1) const -> int8_t;
    auto get_uint8(int at=-1) const -> uint8_t;

    auto get_int16_be(int at=-1) const -> int16_t;
    auto get_int16_le(int at=-1) const -> int16_t;
    auto get_uint16_be(int at=-1) const -> uint16_t;
    auto get_uint16_le(int at=-1) const -> uint16_t;

    auto get_int24_be(int at=-1) const -> int32_t;
    auto get_int24_le(int at=-1) const -> int32_t;
    auto get_uint24_be(int at=-1) const -> uint32_t;
    auto get_uint24_le(int at=-1) const -> uint32_t;

    auto get_int32_be(int at=-1) const -> int32_t;
    auto get_int32_le(int at=-1) const -> int32_t;
    auto get_uint32_be(int at=-1) const -> uint32_t;
    auto get_uint32_le(int at=-1) const -> uint32_t;

    auto get_int40_be(int at=-1) const -> int64_t;
    auto get_int40_le(int at=-1) const -> int64_t;
    auto get_uint40_be(int at=-1) const -> uint64_t;
    auto get_uint40_le(int at=-1) const -> uint64_t;

    auto get_int48_be(int at=-1) const -> int64_t;
    auto get_int48_le(int at=-1) const -> int64_t;
    auto get_uint48_be(int at=-1) const -> uint64_t;
    auto get_uint48_le(int at=-1) const -> uint64_t;

    auto get_int56_be(int at=-1) const -> int64_t;
    auto get_int56_le(int at=-1) const -> int64_t;
    auto get_uint56_be(int at=-1) const -> uint64_t;
    auto get_uint56_le(int at=-1) const -> uint64_t;

    auto get_int64_be(int at=-1) const -> int64_t;
    auto get_int64_le(int at=-1) const -> int64_t;
    auto get_uint64_be(int at=-1) const -> uint64_t;
    auto get_uint64_le(int at=-1) const -> uint64_t;

    auto get_int_be(int sz) const -> int64_t;
    auto get_int_le(int sz) const -> int64_t;

    auto get_double(int at=-1) const -> double;

    auto get_bytes(int, int at=-1) const -> uint8_t*;
    auto get_hex_string(int, int at=-1) -> const string;

    auto get_varint() const -> int64_t;
    auto get_varint2() const -> pair<int64_t, int>;
    auto get_varint_with_size() const -> pair<int64_t, int>;

    auto has_remaining() const -> bool;
    auto remained_size() const -> int;

    auto begin() const { return m_begin; }
    auto offset(int off) { m_offset = off; }
    auto offset() const { return m_offset; }
    auto size() const { return m_count; }
    auto limit() const { return m_limit; }
    auto capacity() const { return m_capacity; }
    auto pointer() const { return m_data; }

    auto advance(int) -> byte_buffer&;
    auto skip(int) -> byte_buffer&;
    auto unget(int) -> byte_buffer&;
    auto take(int) const -> byte_buffer;

    auto slice(int from, int count) -> byte_buffer;
    auto copy_slice(int from, int count) -> byte_buffer;

    auto compare_range(int from, int count, uint8_t value) -> bool;

    auto first() const -> uint8_t;
    auto first(int) const -> byte_buffer;

    auto last() const -> uint8_t;
    auto last(int) const -> byte_buffer;

    auto starts_with(string const& str) const -> bool;

    auto reset() -> byte_buffer&;
    auto reset(initializer_list<uint8_t> l) -> void; 
    auto reset(uint8_t* buffer, size_t sz) -> void;
    auto destroy() -> void;

  public:
    auto operator[](uint32_t index) -> uint8_t&;
    auto operator[](uint32_t index) const -> uint8_t;

  public:
    static auto from_hexcode(string const& s, bool is_be=false) -> byte_buffer;

    auto debug_it() const -> string;

  private:
    auto check_offset(int) const -> void;
    auto leading_byte(uint8_t) const -> uint8_t;
    auto advance(int at, int dist) const -> int;

  private:
    mutable int m_offset{};

    int m_count{};
    int m_limit{};
    int m_begin{};
    int m_capacity{};

    uint8_t* m_data{};
    bool m_owner{};
  };

}

////////////////////////////////////////////////////////////////////////////////
//
//
//
////////////////////////////////////////////////////////////////////////////////
