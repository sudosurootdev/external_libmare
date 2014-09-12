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
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cassert>
#include <cstring>
#include <mare/mare.h>
#include <mare/patterns.hh>
#include <mare/blas/blas.h>

using namespace std;
template<typename T>
static void print_matrix(const char *name, T *a, const int m, const int n)
{
  printf("Matrix %s:\n", name);
  for(int i = 0; i < m; i++) {
    for(int j = 0; j < n; j++) printf("%3.0f ", a[i*n+j]);
    printf("\n");
  }
  printf("\n");
}

int main(int argc, char **argv)
{
  const float alpha = 1.0;
  const float beta = 0.0;
  size_t nVal = 0;
  int mVal, kVal;

  if(argc < 2) {
    mVal = 29; nVal = 13; kVal = 15;
  } else {
    nVal = atoi(argv[1]);
    mVal = nVal; kVal = nVal;
  }

  mare::runtime::init();

  const int m = mVal;
  const int n = static_cast<int>(nVal);
  const int k = kVal;

  // allocate the matrices
  float *a = static_cast<float *>(malloc(m*n*sizeof(float)));
  float *b = static_cast<float *>(malloc(n*k*sizeof(float)));
  float *c = static_cast<float *>(malloc(m*k*sizeof(float)));

  assert(a && b && c);

  // initialize a and b: a random a and a unit matrix b
  for(int i = 0; i < m; i++)
    for(int j = 0; j < n; j++)
      a[i*n+j] = static_cast<float>(i*n+j+1); // random();

  for(int i = 0; i < n; i++)
    for(int j = 0; j < k; j++) {
      if(i == j) b[i*k+j] = 1.0;
      else       b[i*k+j] = 0.0;
    }

  bool success = true;

  int rc = sgemm("N", "N", &m, &n, &k, &alpha, a, &n, b, &k, &beta, c, &k);
  if (rc != 0) {
    MARE_ALOG("SGEMM failed with error code %d!\n", rc);
    success = false;
  }

  // check: c must be equal to a (alpha == 1, beta = 0, b unit
#if 0
  print_matrix("a", a, m, n);
  print_matrix("b", b, n, k);
  print_matrix("c", c, m, k);
#endif
  if( alpha == 1.0 && beta == 0.0 && n == k) {
    for(int i = 0; i < m; i++ ) {
      for(int j = 0; j < k; j++) {
        if(a[i*k+j] != c[i*k+j])
          success = false;
      }
    }
  } else {
    MARE_ALOG("can not verify: please set other parameters");
  }

  if(!success)
    MARE_ALOG("SGEMM failed!\n");

  free(c);
  free(b);
  free(a);

  mare::runtime::shutdown();

  return success ? 0 : 1;
}

