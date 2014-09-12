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

#ifdef __cplusplus
extern "C" {
#endif

int dgemm(const char *transa, const char *transb, const int *m, const int *n,
          const int *k, const double *alpha, const double *a, const int *lda,
          const double *b, const int *ldb, const double *beta,
          double *c, const int *ldc);

int sgemm(const char *transa, const char *transb, const int *m, const int *n,
          const int *k, const float *alpha, const float *a, const int *lda,
          const float *b, const int *ldb, const float *beta,
          float *c, const int *ldc);

#ifdef __cplusplus
}
#endif


