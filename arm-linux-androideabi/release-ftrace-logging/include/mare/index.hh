// --~--~--~--~----~--~--~--~----~--~--~--~----~--~--~--~----~--~--~--~--
// Copyright 2013 Qualcomm Technologies, Inc.  All rights reserved.
// Confidential & Proprietary – Qualcomm Technologies, Inc. ("QTI")
// 
// The party receiving this software directly from QTI (the "Recipient")
// may use this software as reasonably necessary solely for the purposes
// set forth in the agreement between the Recipient and QTI (the
// "Agreement").  The software may be used in source code form solely by
// the Recipient's employees (if any) authorized by the Agreement.
// Unless expressly authorized in the Agreement, the Recipient may not
// sublicense, assign, transfer or otherwise provide the source code to
// any third party.  Qualcomm Technologies, Inc. retains all ownership
// rights in and to the software.
// --~--~--~--~----~--~--~--~----~--~--~--~----~--~--~--~----~--~--~--~--
#pragma once

#include <mare/internal/index.hh>

namespace mare {

/** @addtogroup ranges_doc
@{ */

/** @cond HIDDEN */
#if defined _MSC_FULL_VER && (_MSC_FULL_VER == MARE_VS2012_NOV_CTP)
/** @endcond */

/**
  Methods common to 1D, 2D and 3D index objects are listed here, the
  value for DIMS can be 1, 2 or 3.
*/
template<size_t DIMS>
class index : public internal::index<DIMS>
{
public:
  /**
    Constructs an index object from another index object.

    @param rhs index object to be used for constructing a new object
  */
  index(const internal::index<DIMS>& rhs) : internal::index<DIMS>(rhs) { }

  /**
    Replaces the contents of current index object with an other index object.

    @param rhs index object to be used for replacing the contents
               of current object
  */
  index<DIMS>& operator = (const index<DIMS>& rhs) {
    internal::index<DIMS>::operator=(rhs);
    return (*this);
  }

  /**
    Sums the corresponding values of current index object and another
    index object and returns a reference to current index object.

    @param rhs index object to be used for summing with the values
               of current object
  */
  index<DIMS>& operator += (const index<DIMS>& rhs) {
    internal::index<DIMS>::operator+=(rhs);
    return (*this);
  }

  /**
    Subtracts the corresponding values of current index object and another
    index object and returns a reference to current index object.

    @param rhs index object to be used for subtraction with the values
               of current object
  */
  index<DIMS>& operator -= (const index<DIMS>& rhs) {
    internal::index<DIMS>::operator-=(rhs);
    return (*this);
  }

  /**
    Subtracts the corresponding values of current index object and another
    index object and returns a new index object.

    @param rhs index object to be used for subtraction with the values
               of current object
  */
  index<DIMS> operator - (const index<DIMS>& rhs) {
    return index<DIMS>(internal::index<DIMS>::operator-(rhs));
  }

  /**
    Sums the corresponding values of current index object and another
    index object and returns new index object.

    @param rhs index object to be used for summing with the values
               of current object
  */
  index<DIMS> operator + (const index<DIMS>& rhs) {
    return index<DIMS>(internal::index<DIMS>::operator+(rhs));
  }

#ifdef ONLY_FOR_DOXYGEN
  /**
      Compares this with another index object.

      @param rhs Reference to index to be compared with this

      @return
      TRUE -- The two indices have same values\n
      FALSE -- The two indices have different values

  */
  bool operator == (const index_base<DIMS>& rhs) const;

  /**
      Checks for inequality of this with another index object.

      @param rhs Reference to index to be compared with this

      @return
      TRUE -- The two indices have different values\n
      FALSE -- The two indices have same values

  */
  bool operator != (const index_base<DIMS>& rhs) const;

  /**
      Checks if this object is less than another index object.
      Does a lexicographical comparison of two index objects,
      similar to std::lexicographical_compare().

      @param rhs Reference to index to be compared with this

      @return
      TRUE -- If this is lexicographically smaller than rhs\n
      FALSE -- If this is lexicographically larger or equal to rhs
  */
  bool operator < (const index_base<DIMS>& rhs) const;

  /**
      Checks if this object is less than or equal to another index object.
      Does a lexicographical comparison of two index objects,
      similar to std::lexicographical_compare().

      @param rhs Reference to index to be compared with this

      @return
      TRUE -- If this is lexicographically smaller or equal than rhs\n
      FALSE -- If this is lexicographically larger than rhs
  */
  bool operator <= (const index_base<DIMS>& rhs) const;

  /**
      Checks if this object is greater than another index object.
      Does a lexicographical comparison of two index objects,
      similar to std::lexicographical_compare().

      @param rhs Reference to index to be compared with this

      @return
      TRUE -- If this is lexicographically larger than rhs\n
      FALSE -- If this is lexicographically smaller or equal than rhs\n
  */
  bool operator > (const index_base<DIMS>& rhs) const;

  /**
      Checks if this object is greater or equal to another index object.
      Does a lexicographical comparison of two index objects,
      similar to std::lexicographical_compare().

      @param rhs Reference to index to be compared with this

      @return
      TRUE -- If this is lexicographically larger or equal to rhs\n
      FALSE -- If this is lexicographically smaller than rhs\n
  */
  bool operator >= (const index_base<DIMS>& rhs) const;

  /**
    Returns a reference to i-th coordinate of index object.
    No bounds checking is performed.

    @param i Specifies which coordinate to return
    @return Reference to i-th coordinate of the index object
  */
  size_t& operator [] (size_t i);

  /**
    Returns a const reference to i-th coordinate of index object.
    No bounds checking is performed.

    @param i Specifies which coordinate to return
    @return Const reference to i-th coordinate of the index object
  */
  const size_t& operator [] (size_t i) const;

  /**
    Returns a reference to an std::array of all coordinates of index object.
    @return Const reference to an std::array of all
             coordinates of an index object.
  */
  const std::array<size_t, DIMS>& data() const;
};

/**
  Defines a 1D index object
*/
template<>
class index<1>
{
public:
  /**
    Constructs a 1D index object, equivalent to index<1>(0).
  */
  index();

  /**
    Creates an index object from std::array.
    @param rhs std::array of size_t with one element
  */
  index(const std::array<size_t, 1>& rhs);

  /**
    Creates a 1D index object.
    @param i Value for first coordinate of index object
  */
  explicit index(size_t i);
};

/**
  Defines a 2D index object
*/
template<>
class index<2>
{
public:
  /**
    Constructs a 2D index object, equivalent to index<2>(0, 0).
  */
  index();

  /**
    Creates an index object from std::array.
    @param rhs std::array of size_t with two elements.
  */
  index(const std::array<size_t, 2>& rhs);

  /**
    Creates a 2D index object.
    @param i Value for first coordinate of index object.
    @param j Value for second coordinate of index object.
  */
  index(size_t i, size_t j);
};

/**
  Defines a 3D index object
*/
template<>
class index<3>
{
public:
  /**
    Constructs a 3D index object, equivalent to index<3>(0, 0, 0).
  */
  index();

  /**
    Creates an index object from std::array.
    @param rhs std::array of size_t with three elements.
  */
  index(const std::array<size_t, 3>& rhs);

  /**
    Creates a 3D index object.
    @param i Value for first coordinate of index object.
    @param j Value for second coordinate of index object.
    @param k Value for third coordinate of index object.
  */
  index(size_t i, size_t j, size_t k);
};

/** @cond HIDDEN **/
#endif
#else

template<size_t DIMS>
using index = internal::index<DIMS>;

#endif
/** @endcond **/

/** @} */ /* end_addtogroup ranges_doc */
}; // namespace
