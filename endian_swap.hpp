#pragma once

#include <climits>
#include <stdexcept>

////////////////////////////////////////////////////////////////////////////////
//
//
//
////////////////////////////////////////////////////////////////////////////////
namespace sys::io::detail {

  enum EEndian
  {
    LITTLE_ENDIAN_ORDER,
    BIG_ENDIAN_ORDER,
    HOST_ENDIAN_ORDER = LITTLE_ENDIAN_ORDER
  };

  // this function swap the bytes of values given it's size as a template
  // parameter (could sizeof be used?).
  template <class T, unsigned int size>
  inline T swap_bytes(T value)
  {
    union
    {
      T value;
      char bytes[size];
    } in, out;

    in.value = value;
    out.value = T();

    // memset((void*)&out, NULL,size);
    for (unsigned int i = 0; i < size / 2; ++i)
    {
      out.bytes[i] = in.bytes[size - 1 - i];
      out.bytes[size - 1 - i] = in.bytes[i];
    }

    return out.value;
  }

  // Here is the function you will use. Again there is two compile-time assertion
  // that use the boost libraries. You could probably comment them out, but if you
  // do be cautious not to use this function for anything else than integers
  // types. This function need to be called like this :
  //
  //     int x = someValue;
  //     int i = endian_swap_bytes<HOST_ENDIAN_ORDER, BIG_ENDIAN_ORDER>(x);
  //
  template<EEndian from, EEndian to, class T>
  inline T endian_swap_bytes(T value)
  {
    static_assert (sizeof(T) == 2 || sizeof(T) == 4 || sizeof(T) == 8);
    static_assert (std::is_arithmetic<T>::value);

    if (from == to)
      return value;

    return swap_bytes<T, sizeof(T)>(value);
  }

  ////////////////////////////////////////////////////////////////////////////////
  //
  //
  //
  ////////////////////////////////////////////////////////////////////////////////
}