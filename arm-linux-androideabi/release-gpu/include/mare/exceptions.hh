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
/** @file exceptions.hh */
#pragma once
#include <exception>
#include <stdint.h>
#include <sstream>
#include <typeinfo>
#include <mare/internal/strprintf.hh>

// Perform #ifdef's that control the various asserts
#ifdef MARE_DISABLE_CHECK_API
#undef MARE_CHECK_API
#else
#define MARE_CHECK_API
#endif // MARE_DISABLE_CHECK_API

#ifdef MARE_ENABLE_CHECK_API_DETAILED
#define MARE_CHECK_API_DETAILED
#endif // MARE_ENABLE_CHECK_API_DETAILED

namespace mare {

/** @addtogroup exceptions
@{ */
/**
    Superclass of all MARE-generated exceptions.
*/
class mare_exception : public std::exception
{
public:
  /** Destructor. */
  virtual ~mare_exception() MARE_NOEXCEPT {}
  /**
      Returns exception description.
      @returns
      C string describing the exception.
  */
  virtual const char* what() const throw() = 0;
};


/** Superclass of all MARE-generated exceptions that
    indicate internal or programmer errors.
*/
class error_exception : public mare_exception
{
public:
  error_exception(std::string msg,
                  const char* filename, int lineno, const char* funcname)
    : _message(msg),
      _long_message(mare::internal::strprintf(
                        "%s: %u %s(): %s", filename, lineno, funcname,
                        msg.c_str())),
      _file(filename),
      _line(lineno),
      _function(funcname),
      _classname(compiler_demangle(typeid(*this))) {}

  ~error_exception() throw() {};

  virtual const char* message() const throw() { return _message.c_str(); }
  virtual const char* what() const throw() { return _long_message.c_str(); }
  virtual const char* file() const throw() { return _file.c_str(); }
  virtual int         line() const throw() { return _line; }
  virtual const char* function() const throw() { return _function.c_str(); }
  virtual const char* type() const throw() { return _classname.c_str(); }

private:

  static std::string compiler_demangle(std::type_info const& mytype);

  std::string _message;
  std::string _long_message;
  std::string _file;
  int _line;
  std::string _function;
  std::string _classname;
};

/**
    Represents a misuse of the MARE API. For example, invalid values
    passed to a function. Should cause termination of the application
    (future releases will behave differently).
*/
class api_exception : public error_exception
{
public:
  api_exception(std::string msg,
                const char* filename, int lineno, const char* funcname)
    : error_exception(msg, filename, lineno, funcname)
  { }
};

/** @cond */
// MARE_API_ASSERT
// - Throws api_exception.
// - Enabled by default; disable with MARE_DISABLE_CHECK_API.
// - See debug.hh for macros that control how this is enabled
// Used for reporting invalid arguments passed in or returned via the API.
#ifdef MARE_CHECK_API
#define MARE_API_ASSERT(cond, format, ...) do {                         \
    if(!(cond)) { throw                                                 \
        ::mare::api_exception(::mare::internal::                        \
                              strprintf(format, ## __VA_ARGS__),        \
                              __FILE__, __LINE__, __FUNCTION__);        \
    } } while(false)
#else
#define MARE_API_ASSERT(cond, format, ...) do { \
    if(false) { if(cond){} if(format){} } } while(false)
#endif // MARE_CHECK_API

// MARE_API_DETAILED_ASSERT
// - Throws api_exception.
// - Disabled by default; enable with MARE_ENABLE_CHECK_API_DETAILED.
// - See debug.hh for macros that control how this is enabled
// Used for extra detailed and slower checking of the API calls.
// For example: circular dependency checks, deadlock detection.
#ifdef MARE_CHECK_API_DETAILED
#define MARE_API_DETAILED_ASSERT(cond, format, ...) do { if(!(cond)) { throw \
        ::mare::api_exception(::mare::internal::                        \
                              strprintf(format, ## __VA_ARGS__),        \
                              __FILE__, __LINE__, __FUNCTION__); } \
  } while(false)
#else
#define MARE_API_DETAILED_ASSERT(cond, format, ...) do { if(false) { \
      if(cond){} if(format){} } } while(false)
#endif // MARE_CHECK_API_DETAILED
/** @endcond */

/**
    Indicates that the thread TLS has been misused or become
    corrupted. Should cause termination of the application.
*/
class tls_exception : public error_exception
{
public:
  tls_exception(std::string msg,
                const char* filename, int lineno, const char* funcname)
    : error_exception(msg, filename, lineno, funcname)
  {}
};

/**
    Exception thrown to abort the current task.

    @sa mare::abort_on_cancel()
    @sa mare::abort_task()
*/
class abort_task_exception: public mare_exception
{
public:
  abort_task_exception(std::string msg="aborted task")
    :_msg(msg){
  }

  virtual ~abort_task_exception() MARE_NOEXCEPT {}

  virtual const char* what() const throw() {
    return _msg.c_str();
  }

private:
  std::string _msg;
};
/** @} */ /* end_addtogroup exceptions */

}; //namespace mare
