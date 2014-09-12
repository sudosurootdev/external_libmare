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

#include <string>

namespace mare { namespace internal {

class group;
class task;

/// This cache stores tasks that have been added to one or more groups
/// but haven't been launched yet.
///
/// We need this cache because groups do not keep a list of the tasks
/// in them. Without it, if a task hasn't been launched belongs to a
/// group that gets canceled, we have no way to notify the task that
/// it has been canceled.
namespace unlaunched_task_cache {
/// Calculates size of the unlaunched task cache.
///
/// @return
/// Number of distinct tasks in unlaunched task cache.
size_t get_size();

/// Inserts task t in cache if group g is not canceled.
///
/// @param t Task to insert.
/// @param g Target group.
///
/// @return
/// true - The task has been added to the cache. \n
/// false - The tash has not been added to the cache because g is canceled.
bool insert(task* t, group* g);

/// Removes task from cache.
///
/// @param t Task to remove.
void remove(task* t);

/// Removes and cancels any task from cache that belongs to group g.
///
/// @param g Canceled grup.
void cancel_tasks_from_group(group* g);

/// Returns a string that describes the state of the cache. Use this
/// for debugging purposes only.
///
/// @return String that describes the state of the cache.
std::string to_string();

};

/// This method isolates groups from knowing about the
/// unlaunched_task_cache.
///
/// @param g Group that has been
inline
void cancel_unlaunched_tasks(group* g) {
  unlaunched_task_cache::cancel_tasks_from_group(g);
}

}} //namespace mare::internal

