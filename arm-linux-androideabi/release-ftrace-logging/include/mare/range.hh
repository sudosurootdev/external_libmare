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

#include <mare/internal/range.hh>

namespace mare {

/** @addtogroup ranges_doc
@{ */

/** @cond HIDDEN */
#if defined _MSC_FULL_VER && (_MSC_FULL_VER == MARE_VS2012_NOV_CTP)
/** @endcond */

/**

  Methods common to 1D, 2D and 3D ranges are here.
*/
template<size_t DIMS>
class range
{
#ifdef ONLY_FOR_DOXYGEiN
public:
  /**
    Returns the size of the range.
    @return Size of  range, product of number of
            elements in each dimension
  */
  inline size_t size() const;

  /**
    Returns the dimensions of range.
    @return  Dimensions of range, between [1,3]
  */
  const size_t& dims() const;

  /**
    Returns the i-th coordinate of beginning of range (i < DIMS)
    @return i-th coordinate of beginning of range
  */
  const size_t& begin(const size_t i) const;

  /**
    Returns the i-th coordinate of end of range (i < DIMS)
    @return i-th coordinate of end of range
  */
  const size_t& end(const size_t i) const;

  /**
    Returns all the coordinates of beginning of range (i < DIMS)
    @return An array of all the coordinates beginning of range
  */
  const std::array<size_t, DIMS>& begin();

  /**
    Returns all the coordinates of end of range (i < DIMS)
    @return An array of all the coordinates of end of range
  */
  const std::array<size_t, DIMS>& end();

#endif
};

/**
  Describes a 1D range.
*/
template<>
class range<1> : public internal::range<1>
{
public:
 /**
     Creates an empty 1D range.
  */
  range() : internal::range<1>() { }

  /**
     Creates a 1D range, spans from [b0, e0).

     @param b0 Beginning of 1D range.
     @param e0 End of 1D range.
  */
  range(size_t b0, size_t e0) : internal::range<1>(b0, e0) { }

  /**
     Creates a 1D range, spans from [0, e0).

     @param e0 End of 1D range.
  */
  range(size_t e0) : internal::range<1>(e0) { }
};

/**
  Describes a 2D range.
*/
template<>
class range<2> : public internal::range<2>
{
public:
  /**
     Creates an empty 2D range.
  */
  range() : range<2>() { }

  /**
     Creates a 2D range, comprising of points from
     cross product [b0, e0) x [b1, e1).

     @param b0 First coordinate of beginning of 2D range
     @param e0 First coordinate of end of 2D range
     @param b1 Second coordinate of beginning of 2D range
     @param e1 Second coordinate of end of 2D range
  */
  range(size_t b0, size_t e0, size_t b1, size_t e1) :
    internal::range<2>(b0, e0, b1, e1) { }

  /**
     Creates a 2D range, comprising of points from
     cross product [0, e0) x [0, e1).

     @param e0 First coordinate of end of 2D range
     @param e1 Second coordinate of end of 2D range
  */
  range(size_t e0, size_t e1) : internal::range<2>(e0, e1) { }
};

/**
  Describes a 3D range.
*/
template<>
class range<3> : public internal::range<3>
{
public:
  /**
     Creates an empty 3D range.
  */
  range() : range<3>() { }

  /**
     Creates a 3D range, comprising of points from
     cross product [b0, e0) x [b1, e1) x [b2, e2)

     @param b0 First coordinate of beginning of 3D range
     @param e0 First coordinate of end of 3D range
     @param b1 Second coordinate of beginning of 3D range
     @param e1 Second coordinate of end of 3D range
     @param b2 Third coordinate of beginning of 3D range
     @param e2 Third coordinate of end of 3D range
  */
  range(size_t b0, size_t e0, size_t b1, size_t e1, size_t b2, size_t e2) :
    internal::range<3>(b0, e0, b1, e1, b2, e2) { }

  /**
     Creates a 3D range, comprising of points from
     cross product [0, e0) x [0, e1) x [0, e2)

     @param e0 First coordinate of end of 3D range
     @param e1 Second coordinate of end of 3D range
     @param e2 Third coordinate of end of 3D range
  */
  range(size_t e0, size_t e1, size_t e2) : internal::range<3>(e0, e1, e2) { }
};

/** @cond HIDDEN **/
#else

template<size_t DIMS>
using range = internal::range<DIMS>;

#endif // _MSC_FULL_VER == MARE_VS2012_NOV_CTP
/** @endcond **/
/** @} */ /* end_addtogroup ranges_doc */
}; // namespace
