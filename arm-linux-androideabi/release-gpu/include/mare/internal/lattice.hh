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

#include <mutex>
#include <set>

#include <mare/common.hh>

namespace mare { namespace internal {

class group;

enum lattice_lock_policy {
  acquire_lattice_lock,
  do_not_acquire_lattice_lock
};

class lattice {
public:
  // We use a type to try different locks for lattice operations.
  typedef std::mutex lock_type;

private:

  typedef std::set<group*> group_set;
  static lock_type s_mutex;

public:
  static group* create_meet_node(group* a, group* b,
                                 group* current_group = nullptr);

  static lock_type& mutex() { return s_mutex;}

private:

  inline static void find_sup(group_signature& new_b, group* ancestor1,
                       group* ancestor2, group_set& sup)
  {
    find_sup(new_b, ancestor1, sup);
    find_sup(new_b, ancestor2, sup);
  };
  inline static void find_inf(group_signature& new_b, group* ancestor1,
                         group* ancestor2, group_set& inf)
  {
    find_inf(new_b, ancestor1, inf);
    find_inf(new_b, ancestor2, inf);
  }

  static void find_sup(group_signature& new_b, group* ancestor,
                       group_set& sup);
  static void find_sup_rec(group_signature& new_b, size_t order,
                           group* ancestor, group_set& sup, bool& found);
  static void find_inf(group_signature& new_b, group* ancestor,
                       group_set& inf);
  static void find_inf_rec(group_signature& new_b, size_t order, group* node,
                           group_set& inf);
  static void insert_in_lattice(group* new_group, group_set const& sup,
                                group_set const& inf);
  static void insert_in_lattice_childfree_leaves(group* new_group,
                                                 group* a, group* b);
  static void insert_in_lattice_childfree_parents(group* new_group,
                                                  group_set const& sup);
  static void insert_in_lattice_leaves(group* new_group, group* a,
                                              group* b, group_set const& inf);

};

} } //namespace mare::internal

