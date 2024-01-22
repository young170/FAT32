//#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING

#include "byte_buffer.hpp"
#include "endian_swap.hpp"
//#include <bit>

#include <cctype>
#include <cstring>
#include <codecvt>
#include <locale>
#include <sstream>
#include <cassert>
#include <algorithm>

////////////////////////////////////////////////////////////////////////////////
//
// byte_buffer is for shallow version of buffer
//
////////////////////////////////////////////////////////////////////////////////
namespace sys::io {
  
  using namespace detail;

  byte_buffer::byte_buffer(uint8_t* data, int offset, int count, bool owner)
  {
    m_data   = data;
    m_offset = offset;
    m_begin  = offset;
    m_count  = count;
    m_limit  = m_begin + count;
    m_owner  = owner;

    m_capacity = owner ? m_limit : -1;
  }

  byte_buffer::byte_buffer(uint8_t* data, int count, bool owner)
    : byte_buffer(data, 0, count, owner)
  {}

  byte_buffer::byte_buffer()
    : byte_buffer(nullptr, 0, 0)
  {}

  byte_buffer::byte_buffer(string && src)
  {
    auto len = src.length();

    if (len > 0) 
    {
      m_data = new uint8_t[len];
      for (int i=0; i<len; i++) m_data[i] = src[i];

      m_begin  = 0;
      m_offset = 0;
      m_count  = int(len);
      m_limit  = int(m_begin + len);
      m_owner  = true;
      m_capacity = m_limit;
    }
  }

  byte_buffer::byte_buffer(initializer_list<uint8_t> l)
  {
    reset(l);
  }

  byte_buffer::byte_buffer(byte_buffer&& rhs) noexcept
  {
    m_data   = rhs.m_data;
    m_offset = rhs.m_offset;
    m_begin  = rhs.m_begin;
    m_count  = rhs.m_count;
    m_limit  = rhs.m_limit;
    m_owner  = rhs.m_owner;
    m_capacity = rhs.m_capacity;
    
    rhs.m_data  = nullptr;
    rhs.m_owner = false;
  }

  auto byte_buffer::operator=(byte_buffer const& rhs) -> byte_buffer&
  {
    if (&rhs != this)
    {
      m_offset = rhs.m_offset;
      m_begin  = rhs.m_begin;
      m_count  = rhs.m_count;
      m_limit  = rhs.m_limit;
      m_owner  = rhs.m_owner;
      m_capacity = rhs.m_capacity;

      // TODO test
      if (m_owner)
      {
        delete [] m_data;
        m_data = new uint8_t[m_count];
        copy(rhs.m_data, rhs.m_data + m_count, m_data);
      }
      else
      {
        m_data = rhs.m_data;
      }
    }

    return *this;
  }

  byte_buffer::~byte_buffer()
  {
    destroy();
  }

  auto byte_buffer::destroy() -> void
  {
    if (m_owner)
    {
      delete [] m_data;
      m_data = nullptr;
    }
  }

  auto byte_buffer::set_owner() -> byte_buffer&
  {
    if (!m_owner and m_count > 0)
    {
      auto new_data = new uint8_t[m_count];
      memcpy(new_data, m_data + m_begin, m_count);

      m_data = new_data;
      m_limit = m_count;
      m_capacity = m_limit;
      m_offset = m_offset - m_begin;
      m_begin = 0;
      m_owner = true;
    }

    return *this;
  }

  // very simple scheme
  auto byte_buffer::resize(int upto) -> int
  {
    if (!m_owner or upto <= m_limit)
      return -1;

    auto blocks = (upto + 4096 - 1) / 4096;
    m_capacity  = blocks * 4096;

    auto new_data = new uint8_t[m_capacity];
    memcpy(new_data, m_data, m_count);
    delete [] m_data;
    m_data = new_data;

    return m_capacity;
  }

  auto byte_buffer::append(byte_buffer const& b) -> byte_buffer&
  {
    append(b.m_data, b.offset(), b.m_count);

    return *this;
  }

  auto byte_buffer::append(byte_buffer* bb) -> byte_buffer&
  {
    append(bb->m_data, bb->offset(), bb->m_count);

    if (bb->m_owner) 
      delete bb;

    return *this;
  }

  //
  // Note: pre-condition for append operation
  //   1) m_owner == true
  //   2) m_begin == 0 and m_count == m_limit
  //   3) m_offset should not change
  //   4) later there should be one more mark called 'm_reserved' to prevent unnecessary allocation
  //
  auto byte_buffer::append(uint8_t* buffer, int offset, int count) -> int
  {
    if (!m_owner)
      throw runtime_error("only owned buffer can append");

    if (m_limit + count >= m_capacity)
      resize(m_count+count);

    memcpy(m_data + m_limit, buffer, count);
    m_limit += count;
    m_count += count;

    assert(m_limit <= m_capacity);

    return m_limit;
  }

  auto byte_buffer::operator[](uint32_t index) -> uint8_t&
  {
    check_offset(index);

    return m_data[m_begin + index];
  }

  auto byte_buffer::operator[](uint32_t index) const -> uint8_t
  {
    check_offset(index);

    return m_data[m_begin + index];
  }

  auto byte_buffer::get_int8(int at) const -> int8_t
  {
    check_offset(1);

    auto here = advance(at, 1);

    return (int8_t)m_data[here];
  }

  auto byte_buffer::get_uint8(int at) const -> uint8_t
  {
    check_offset(1);

    auto here = advance(at, 1);

    return m_data[here];
  }

  auto byte_buffer::get_int16_be(int at) const -> int16_t
  {
    check_offset(2);

    auto here = advance(at, 2);
    auto res  = *(int16_t *)(uint8_t *)&m_data[here];

    return detail::endian_swap_bytes<HOST_ENDIAN_ORDER, BIG_ENDIAN_ORDER>(res);

  }

  auto byte_buffer::get_int16_le(int at) const -> int16_t
  {
    check_offset(2);

    auto here = advance(at, 2);
    auto res  = *(int16_t *)(uint8_t *)&m_data[here];

    return res;
  }

  auto byte_buffer::get_uint16_be(int at) const -> uint16_t
  {
    check_offset(2);

    auto here = advance(at, 2);
    auto res  = *(uint16_t *)(uint8_t *)&m_data[here];

    return detail::endian_swap_bytes<HOST_ENDIAN_ORDER, BIG_ENDIAN_ORDER>(res);
  }

  auto byte_buffer::get_uint16_le(int at) const -> uint16_t
  {
    check_offset(2);

    auto here = advance(at, 2);
    auto res  = *(uint16_t *)(uint8_t *)&m_data[here];

    return res;
  }

  auto byte_buffer::get_int24_be(int at) const -> int32_t
  {
    check_offset(3);

    auto here = advance(at, 3);
    auto res  = (int32_t)leading_byte(m_data[here]);

    for (int i=here; i<here+3; i++)
      res = (res << 8) + m_data[i]; 

    return res;
  }

  auto byte_buffer::get_int24_le(int at) const -> int32_t
  {
    check_offset(3);

    auto here = advance(at, 3);
    
    auto first = m_data[here+2];
    auto l = (int32_t)leading_byte(first);
    auto r = l << 8 | first; r <<= 8;
    r |= m_data[here + 1]; r <<= 8;
    r |= m_data[here + 0];
    
    return int32_t(r);
  }

  auto byte_buffer::get_uint24_be(int at) const -> uint32_t
  {
    check_offset(3);

    auto here = advance(at, 3);
    auto res  = (uint32_t)0;

    for (int i=here; i<here+3; i++)
      res = (res << 8) + m_data[i];

    return res;
  }

  auto byte_buffer::get_uint24_le(int at) const -> uint32_t
  {
    check_offset(3);
    auto here = advance(at, 3);
    auto res  = uint32_t(m_data[here+2]); res <<= 8;
    res |= m_data[here + 1]; res <<= 8;
    res |= m_data[here];
    
    return res;
  }

  auto byte_buffer::get_int32_be(int at) const -> int32_t
  {
    check_offset(4);

    auto here = advance(at, 4);
    auto res  = *(int32_t *)(uint8_t *)&m_data[here];

    //return byteswap(res);
    return detail::endian_swap_bytes<HOST_ENDIAN_ORDER, BIG_ENDIAN_ORDER>(res);
  }

  auto byte_buffer::get_int32_le(int at) const -> int32_t
  {
    check_offset(4);

    auto here = advance(at, 4);
    auto res  = *(int32_t *)(uint8_t *)&m_data[here];

    return res;
  }

  auto byte_buffer::get_uint32_be(int at) const -> uint32_t
  {
    check_offset(4);

    auto here = advance(at, 4);
    auto res  = *(uint32_t *)(uint8_t *)&m_data[here];

    //return byteswap(res);
    return detail::endian_swap_bytes<HOST_ENDIAN_ORDER, BIG_ENDIAN_ORDER>(res);
  }

  auto byte_buffer::get_uint32_le(int at) const -> uint32_t
  {
    check_offset(4);

    auto here = advance(at, 4);
    auto res  = *(uint32_t *)(uint8_t *)&m_data[here];

    return res;
  }

  auto byte_buffer::get_int40_be(int at) const -> int64_t
  {
    check_offset(5);

    auto here = advance(at, 5);
    auto sign = (int64_t)leading_byte(m_data[here]);
    auto res  = (sign << 16) | (sign << 8) | sign;

    for (int i=here; i<here+5; i++)
      res = (res << 8) + m_data[i]; 

    return res;
  }

  auto byte_buffer::get_int40_le(int at)  const -> int64_t
  {
    check_offset(5);

    auto here = advance(at, 5);
    auto sign = (int64_t)leading_byte(m_data[here+4]);
    auto res  = (sign << 16) | (sign << 8) | sign;

    for (int i=here+4; i>=here; i--)
      res = (res << 8) + m_data[i]; 

    return res;
  }

  auto byte_buffer::get_uint40_be(int at) const -> uint64_t
  {
    check_offset(5);

    auto here = advance(at, 5);
    auto res  = (uint64_t)0;

    for (int i=here; i<here+5; i++)
      res = (res << 8) + m_data[i]; 

    return res;
  }

  auto byte_buffer::get_uint40_le(int at) const -> uint64_t
  {
    check_offset(5);

    auto here = advance(at, 5);
    auto res  = (uint64_t)0;
    for (int i=here+4; i>=here; i--)
      res = (res << 8) + m_data[i]; 

    return res;
  }

  auto byte_buffer::get_int48_be(int at) const -> int64_t
  {
    check_offset(6);

    auto here = advance(at, 6);
    auto sign = (int64_t)leading_byte(m_data[here]);
    auto res  = (sign << 8) | sign;

    for (int i=here; i<here+6; i++)
      res = (res << 8) + m_data[i]; 

    return res;
  }

  auto byte_buffer::get_int48_le(int at) const -> int64_t
  {
    check_offset(6);

    auto here = advance(at, 6);
    auto sign = (int64_t)leading_byte(m_data[m_offset+5]);
    auto res  = (sign << 8) | sign;

    for (int i=here+5; i>=here; i--)
      res = (res << 8) + m_data[i]; 

    return res;
  }

  auto byte_buffer::get_uint48_be(int at) const -> uint64_t
  {
    check_offset(6);

    auto here = advance(at, 6);
    auto res  = (uint64_t)0;

    for (int i=here; i<here+6; i++)
      res = (res << 8) + m_data[i]; 

    return res;
  }

  auto byte_buffer::get_uint48_le(int at) const -> uint64_t
  {
    check_offset(6);

    auto here = advance(at, 6);
    auto res  = (uint64_t)0;

    for (int i=here+5; i>=here; i--)
      res = (res << 8) + m_data[i]; 

    return res;
  }

  auto byte_buffer::get_int56_be(int at) const -> int64_t
  {
    check_offset(7);

    auto here = advance(at, 7);
    auto res  = (int64_t)leading_byte(m_data[m_offset]);

    for (int i=here; i<here+7; i++)
      res = (res << 8) + m_data[i]; 

    return res;
  }

  auto byte_buffer::get_int56_le(int at) const -> int64_t
  {
    check_offset(7);

    auto here = advance(at, 7);
    auto res  = (int64_t)leading_byte(m_data[m_offset+6]);

    for (int i=here+6; i>=here; i--)
      res = (res << 8) + m_data[i]; 

    return res;
  }

  auto byte_buffer::get_uint56_be(int at) const -> uint64_t
  {
    check_offset(7);

    auto here = advance(at, 7);
    auto res  = (uint64_t)0;

    for (int i=here; i<here+7; i++)
      res = (res << 8) + m_data[i]; 

    m_offset += 7;

    return res;
  }

  auto byte_buffer::get_uint56_le(int at) const -> uint64_t
  {
    check_offset(7);

    auto here = advance(at, 7);
    auto res  = (uint64_t)0;

    for (int i=here+6; i>=here; i--)
      res = (res << 8) + m_data[i]; 

    return res;
  }

  auto byte_buffer::get_int64_be(int at) const -> int64_t
  {
    check_offset(8);

    auto here = advance(at, 8);
    auto res  = *(int64_t *)(uint8_t *)&m_data[here];

    //return byteswap(res);
    return detail::endian_swap_bytes<HOST_ENDIAN_ORDER, BIG_ENDIAN_ORDER>(res);
  }

  auto byte_buffer::get_int64_le(int at) const -> int64_t
  {
    check_offset(8);

    auto here = advance(at, 8);
    auto res  = *(int64_t *)(uint8_t *)&m_data[here];

    return res;
  }

  auto byte_buffer::get_int_be(int sz) const -> int64_t
  {
    int64_t res = 0;

    for (int i=0; i<sz; i++)
    {
      auto b = get_uint8();
      res |= int64_t(b) << ((sz - 1 - i) * 8);
    }

    return res;
  }

  auto byte_buffer::get_int_le(int sz) const -> int64_t
  {
    int64_t res = 0;

    for (int i=0; i<sz; i++)
    {
      auto b = get_uint8();
      res |= (int64_t(b) << (i*8));
    }

    return res;
  }

  auto byte_buffer::get_uint64_be(int at) const -> uint64_t
  {
    check_offset(8);

    auto here = advance(at, 8);
    auto res  = *(uint64_t *)(uint8_t *)&m_data[here];

    //return byteswap(res);
    return detail::endian_swap_bytes<HOST_ENDIAN_ORDER, BIG_ENDIAN_ORDER>(res);
  }

  auto byte_buffer::get_uint64_le(int at) const -> uint64_t
  {
    check_offset(8);

    auto here = advance(at, 8);
    auto res  = *(uint64_t *)(uint8_t *)&m_data[here];

    return res;
  }

  auto byte_buffer::get_double(int at) const -> double
  {
    auto res = get_int64_be(at);

    return *(double *)&res;
  }

  auto byte_buffer::get_bytes(int size, int at) const -> uint8_t*
  {
    check_offset(size);
    
    auto here = advance(at, size);
    auto res  = &m_data[here];

    return res;
  }

  auto byte_buffer::get_hex_string(int size, int at) -> const string
  {
    constexpr char hmap[] =
      { '0', '1', '2', '3', '4', '5', '6', '7',
        '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };

    auto here = advance(at, size);
    string res(size * 2, ' ');

    for (int i=0; i<size; i++)
    {
      res[2*i + 0] = hmap[(m_data[here + i] & 0xf0) >> 4];
      res[2*i + 1] = hmap[(m_data[here + i] & 0x0f)];
    }

    return res;
  }

  auto byte_buffer::get_varint() const -> int64_t
  {
    auto [res, size] = get_varint_with_size();

    return res;
  }

  auto byte_buffer::get_varint2() const -> pair<int64_t, int>
  {
    return get_varint_with_size();
  }

  auto byte_buffer::get_varint_with_size() const -> pair<int64_t, int>
  {
    auto complete = false;
    auto value_size = 0;
    auto value = 0L;

    while (value_size < 9 && !complete && m_offset + value_size < m_limit)
    {
      auto val = int64_t(m_data[m_offset + value_size]);
      if ((val & 0b10000000) == 0b10000000 && value_size < 8)
      {
        value = value << 7 | (val & 0b01111111);
      } 
      else if ((val & 0b10000000) == 0b10000000 && value_size == 8)
      {
        value = value << 8 | val;
        complete = true;
      } 
      else 
      {
        value = value << 7 | (val & 0b01111111);
        complete = true;
      }   

      value_size += 1;
    }

    if (!complete)
      throw logic_error("invalid varint");

    m_offset += value_size;

    return make_pair(value, value_size);
  }

  auto byte_buffer::has_remaining() const -> bool
  {
    return m_offset < m_limit;
  }

  auto byte_buffer::remained_size() const -> int
  {
    return m_limit - m_offset;
  }

  auto byte_buffer::advance(int count) -> byte_buffer&
  {
    m_offset += count;

    return *this;
  }

  auto byte_buffer::skip(int count) -> byte_buffer&
  {
    m_offset += count;

    return *this;
  }

  auto byte_buffer::unget(int count) -> byte_buffer&
  {
    m_offset -= count;

    return *this;
  }
    
  auto byte_buffer::take(int amount) const -> byte_buffer
  {
    byte_buffer subrange(m_data, m_offset, amount);
    m_offset += amount;

    return subrange;
  }

  auto byte_buffer::slice(int from, int count) -> byte_buffer
  {
    return byte_buffer(m_data, from, count);
  }
    
  auto byte_buffer::copy_slice(int from, int count) -> byte_buffer
  {
    auto data = new uint8_t[count];
    memcpy(data, m_data + from, count);
    
    return byte_buffer(data, count, true);
  }

  auto byte_buffer::compare_range(int from, int count, uint8_t value) -> bool
  {
    if (from + count >= m_limit) return false;

    for (int i=from; i<from+count; i++)
      if (m_data[i] != value) 
        return false;

    return true;
  }

  auto byte_buffer::first() const -> uint8_t
  {
    check_offset(1);

    return m_data[0];
  }

  auto byte_buffer::first(int amount) const -> byte_buffer
  {
    check_offset(amount);

    return byte_buffer(m_data, m_begin, amount);
  }

  auto byte_buffer::last() const -> uint8_t
  {
    check_offset(1);

    return m_data[m_limit - 1];
  }

  auto byte_buffer::last(int amount) const -> byte_buffer
  {
    byte_buffer sub_range(m_data, m_limit - amount, amount);

    return sub_range;
  }

  auto byte_buffer::starts_with(string const& str) const -> bool
  {
    if ((int)str.length() > m_count)
      throw out_of_range("starts_with: array out of index");
      
    auto len = str.length();
    string s((char*)&m_data[0], len);

    return str == s;
  }

  auto byte_buffer::reset() -> byte_buffer&
  {
    m_offset = m_begin;

    return *this;
  }
  
  auto byte_buffer::reset(initializer_list<uint8_t> l) -> void
  {
    m_data = new uint8_t[l.size()];
    int i = 0;
    for (auto it : l) m_data[i++] = it;

    m_offset = 0;
    m_begin  = 0;
    m_count  = (int)l.size();
    m_limit  = m_count;
    m_owner  = true;
  }

  auto byte_buffer::reset(uint8_t* buffer, size_t size) -> void
  {
    if (m_owner)
      delete [] m_data;

    m_data   = buffer;
    m_limit  = size;
    m_offset = 0;
    m_begin  = 0;
    m_count  = size;

    m_owner = true;
  }

  auto byte_buffer::get_ascii(int size) const -> string
  {
    if (m_offset + size > m_limit)
      throw out_of_range("get_ascii: out of range");
    
    string res((char*)&m_data[m_offset], size);

    m_offset += size;

    return res;
  }

  auto byte_buffer::get_ascii() const -> string
  {
    auto offset = m_offset;

    for (; offset < m_limit && m_data[offset] != 0; ++offset)
      ;

    auto size = offset - m_offset;

    string res((char*)&m_data[m_offset], size);

    m_offset += size + 1;

    return res;
  }

  auto byte_buffer::get_unicode16_le(int size) const -> string
  {
    if (m_offset + size*2 > m_limit)
      throw out_of_range("get_unicode16_le(sz): out of range");

    u16string u16;
    for (auto i=0; i<size; i++)
    {
      u16 += *(char16_t *)&m_data[m_offset];
      m_offset += 2;
    }

    string result;
#if _MSC_VER >= 1900
    wstring_convert<codecvt_utf8_utf16<int16_t>, int16_t> convert;
    auto p = reinterpret_cast<const int16_t *>(u16.data());
    result = convert.to_bytes(p, p + u16.size());
#else
    wstring_convert<codecvt_utf8_utf16<char16_t>, char16_t> convert;
    result = convert.to_bytes(u16);
#endif

    return result;

    // if (m_offset + size*2 > m_limit)
    // throw out_of_range("get_unicode16_le(sz): out of range");

    // using converter = wstring_convert<
    // codecvt_utf8_utf16<char16_t, 0x10ffff, (codecvt_mode)1>,
    // char16_t>;

    // converter c;
    // auto beg = (char16_t *)&m_data[m_offset];
    // auto res = c.to_bytes(beg, beg + size);
    // m_offset += size*2;

    // return res;
  }

  auto byte_buffer::from_hexcode(string const& s, bool is_be) -> byte_buffer
  {
    auto all_hex = all_of(s.cbegin(), s.cend(), 
        [](int c) { return isxdigit(c) != 0; });

    if (s.length() % 2 == 0 && all_hex)
    {
      auto size = int(s.length() / 2);
      auto data = new uint8_t[size];

      for (int i=0; i<size; ++i)
      {
        auto in = is_be 
          ? s.substr((size - (i + 1)) * 2, 2)
          : s.substr(i * 2, 2);

        istringstream strm(in);
        uint16_t val; strm >> hex >> val;
        data[i] = (uint8_t)val;
      }

      byte_buffer res(data, 0, size, true);
      return res;
    }
    
    return byte_buffer();
  }

  auto byte_buffer::to_s(int from, int to) const -> string
  {
    if (from == -1) from = m_offset;
    if (to == -1) to = m_limit;

    return string((char*)&m_data[from], to - from);
  }

  auto byte_buffer::check_offset(int count) const -> void
  {
    if (m_offset + count > m_limit)
      throw out_of_range("check_offset: array out of index");
  }

  auto byte_buffer::leading_byte(uint8_t b) const -> uint8_t 
  {
    return (b & 0x80) == 0 ? 0 : 0xff;
  }

  auto byte_buffer::advance(int at, int dist) const -> int
  {
    auto here = (at == -1) ? m_offset : m_begin + at;
    if (at == -1)
      m_offset += dist;

    return here;
  }

  auto byte_buffer::debug_it() const -> string
  {
    stringstream ss;

    ss << "=================="   << endl;
    ss << "offset: " << m_offset << endl;
    ss << "begin:  " << m_begin  << endl;
    ss << "count:  " << m_count  << endl;
    ss << "limit:  " << m_limit  << endl;
    ss << "=================="   << endl;

    return ss.str();
  }

}

////////////////////////////////////////////////////////////////////////////////
//
//
//
////////////////////////////////////////////////////////////////////////////////
