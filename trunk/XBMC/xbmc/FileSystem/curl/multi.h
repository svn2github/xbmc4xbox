#ifndef __CURL_MULTI_H
#define __CURL_MULTI_H
/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) 1998 - 2005, Daniel Stenberg, <daniel@haxx.se>, et al.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution. The terms
 * are also available at http://curl.haxx.se/docs/copyright.html.
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the COPYING file.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 * $Id$
 ***************************************************************************/
/*
  This is an "external" header file. Don't give away any internals here!

  GOALS

  o Enable a "pull" interface. The application that uses libcurl decides where
    and when to ask libcurl to get/send data.

  o Enable multiple simultaneous transfers in the same thread without making it
    complicated for the application.

  o Enable the application to select() on its own file descriptors and curl's
    file descriptors simultaneous easily.

*/
#if defined(_WIN32) && !defined(WIN32)
/* Chris Lewis mentioned that he doesn't get WIN32 defined, only _WIN32 so we
   make this adjustment to catch this. */
#define WIN32 1
#endif

#if defined(WIN32) && !defined(_WIN32_WCE) && !defined(__GNUC__) || \
  defined(__MINGW32__)
#if !(defined(_WINSOCKAPI_) || defined(_WINSOCK_H))
/* The check above prevents the winsock2 inclusion if winsock.h already was
   included, since they can't co-exist without problems */
#include <winsock2.h>
#endif
#else

/* HP-UX systems version 9, 10 and 11 lack sys/select.h and so does oldish
   libc5-based Linux systems. Only include it on system that are known to
   require it! */
#if defined(_AIX) || defined(NETWARE)
#include <sys/select.h>
#endif

#ifndef _WIN32_WCE
#include <sys/socket.h>
#endif
#include <sys/time.h>
#include <sys/types.h>
#endif

#include "curl.h"

#ifdef  __cplusplus
extern "C" {
#endif

typedef void CURLM;

#ifdef HAVE_CURL_MULTI_SOCKET /* this is not set by anything yet */

#ifndef curl_socket_typedef
/* Public socket typedef */
#ifdef WIN32
typedef SOCKET curl_socket_t;
#define CURL_SOCKET_BAD INVALID_SOCKET
#else
typedef int curl_socket_t;
#define CURL_SOCKET_BAD -1
#endif
#define curl_socket_typedef
#endif /* curl_socket_typedef */

#endif /* HAVE_CURL_MULTI_SOCKET */

typedef enum {
  CURLM_CALL_MULTI_PERFORM=-1, /* please call curl_multi_perform() soon */
  CURLM_OK,
  CURLM_BAD_HANDLE,      /* the passed-in handle is not a valid CURLM handle */
  CURLM_BAD_EASY_HANDLE, /* an easy handle was not good/valid */
  CURLM_OUT_OF_MEMORY,   /* if you ever get this, you're in deep sh*t */
  CURLM_INTERNAL_ERROR,  /* this is a libcurl bug */
  CURLM_LAST
} CURLMcode;

typedef enum {
  CURLMSG_NONE, /* first, not used */
  CURLMSG_DONE, /* This easy handle has completed. 'result' contains
                   the CURLcode of the transfer */
  CURLMSG_LAST /* last, not used */
} CURLMSG;

struct CURLMsg {
  CURLMSG msg;       /* what this message means */
  CURL_HANDLE *easy_handle; /* the handle it concerns */
  union {
    void *whatever;    /* message-specific data */
    CURLcode result;   /* return code for transfer */
  } data;
};
typedef struct CURLMsg CURLMsg;

/*
 * Name:    curl_multi_init()
 *
 * Desc:    inititalize multi-style curl usage
 *
 * Returns: a new CURLM handle to use in all 'curl_multi' functions.
 */
CURL_EXTERN CURLM *curl_multi_init(void);

/*
 * Name:    curl_multi_add_handle()
 *
 * Desc:    add a standard curl handle to the multi stack
 *
 * Returns: CURLMcode type, general multi error code.
 */
CURL_EXTERN CURLMcode curl_multi_add_handle(CURLM *multi_handle,
                                            CURL_HANDLE *curl_handle);

 /*
  * Name:    curl_multi_remove_handle()
  *
  * Desc:    removes a curl handle from the multi stack again
  *
  * Returns: CURLMcode type, general multi error code.
  */
CURL_EXTERN CURLMcode curl_multi_remove_handle(CURLM *multi_handle,
                                               CURL_HANDLE *curl_handle);

 /*
  * Name:    curl_multi_fdset()
  *
  * Desc:    Ask curl for its fd_set sets. The app can use these to select() or
  *          poll() on. We want curl_multi_perform() called as soon as one of
  *          them are ready.
  *
  * Returns: CURLMcode type, general multi error code.
  */
CURL_EXTERN CURLMcode curl_multi_fdset(CURLM *multi_handle,
                                       fd_set *read_fd_set,
                                       fd_set *write_fd_set,
                                       fd_set *exc_fd_set,
                                       int *max_fd);

 /*
  * Name:    curl_multi_perform()
  *
  * Desc:    When the app thinks there's data available for curl it calls this
  *          function to read/write whatever there is right now. This returns
  *          as soon as the reads and writes are done. This function does not
  *          require that there actually is data available for reading or that
  *          data can be written, it can be called just in case. It returns
  *          the number of handles that still transfer data in the second
  *          argument's integer-pointer.
  *
  * Returns: CURLMcode type, general multi error code. *NOTE* that this only
  *          returns errors etc regarding the whole multi stack. There might
  *          still have occurred problems on invidual transfers even when this
  *          returns OK.
  */
CURL_EXTERN CURLMcode curl_multi_perform(CURLM *multi_handle,
                                         int *running_handles);

 /*
  * Name:    curl_multi_cleanup()
  *
  * Desc:    Cleans up and removes a whole multi stack. It does not free or
  *          touch any individual easy handles in any way. We need to define
  *          in what state those handles will be if this function is called
  *          in the middle of a transfer.
  *
  * Returns: CURLMcode type, general multi error code.
  */
CURL_EXTERN CURLMcode curl_multi_cleanup(CURLM *multi_handle);

/*
 * Name:    curl_multi_info_read()
 *
 * Desc:    Ask the multi handle if there's any messages/informationals from
 *          the individual transfers. Messages include informationals such as
 *          error code from the transfer or just the fact that a transfer is
 *          completed. More details on these should be written down as well.
 *
 *          Repeated calls to this function will return a new struct each
 *          time, until a special "end of msgs" struct is returned as a signal
 *          that there is no more to get at this point.
 *
 *          The data the returned pointer points to will not survive calling
 *          curl_multi_cleanup().
 *
 *          The 'CURLMsg' struct is meant to be very simple and only contain
 *          very basic informations. If more involved information is wanted,
 *          we will provide the particular "transfer handle" in that struct
 *          and that should/could/would be used in subsequent
 *          curl_easy_getinfo() calls (or similar). The point being that we
 *          must never expose complex structs to applications, as then we'll
 *          undoubtably get backwards compatibility problems in the future.
 *
 * Returns: A pointer to a filled-in struct, or NULL if it failed or ran out
 *          of structs. It also writes the number of messages left in the
 *          queue (after this read) in the integer the second argument points
 *          to.
 */
CURL_EXTERN CURLMsg *curl_multi_info_read(CURLM *multi_handle,
                                          int *msgs_in_queue);

/*
 * Name:    curl_multi_strerror()
 *
 * Desc:    The curl_multi_strerror function may be used to turn a CURLMcode
 *          value into the equivalent human readable error string.  This is
 *          useful for printing meaningful error messages.
 *
 * Returns: A pointer to a zero-terminated error message.
 */
CURL_EXTERN const char *curl_multi_strerror(CURLMcode);

#ifdef HAVE_CURL_MULTI_SOCKET
/*
 * Name:    curl_multi_socket() and
 *          curl_multi_socket_all()
 *
 * Desc:    An alternative version of curl_multi_perform() that allows the
 *          application to pass in one of the file descriptors that have been
 *          detected to have "action" on them and let libcurl perform. This
 *          allows libcurl to not have to scan through all possible file
 *          descriptors to check for this. The app is recommended to pass in
 *          the 'easy' argument (or set it to CURL_EASY_NONE) to make libcurl
 *          figure out the internal structure even faster and easier.  If the
 *          easy argument is set to something else than CURL_EASY_NONE, the
 *          's' (socket) argument will be ignored by libcurl.
 *
 *          It also informs the application about updates in the socket (file
 *          descriptor) status by doing none, one or multiple calls to the
 *          curl_socket_callback. It thus updates the status with changes
 *          since the previous time this function was used. If 'callback' is
 *          NULL, no callback will be called. A status change may also be a
 *          new timeout only, having the same IN/OUT status as before.
 *
 *          If a previous wait for socket action(s) timed out, you should call
 *          this function with the socket argument set to
 *          CURL_SOCKET_TIMEOUT. If you want to force libcurl to (re-)check
 *          all its internal sockets, and call the callback with status for
 *          all sockets no matter what the previous state is, you call
 *          curl_multi_socket_all() instead.
 *
 *          curl_multi_perform() is thus the equivalent of calling
 *          curl_multi_socket_all(handle, NULL, NULL);
 *
 *          IMPLEMENTATION: libcurl will need an internal hash table to map
 *          socket numbers to internal easy handles for the cases when 'easy'
 *          is set to CURL_EASY_NONE.
 *
 *          Regarding the timeout argument in the callback: it is the timeout
 *          (in milliseconds) for waiting on action on this socket (and the
 *          given time period starts when the callback is called) until you
 *          should call curl_multi_socket() with the timeout stuff mentioned
 *          above. If "actions" happens on the socket before the timeout
 *          happens, remember that the timout timer keeps ticking until told
 *          otherwise.
 *
 *          The "what" argument has one of five values:
 *
 *            0 CURL_POLL_NONE (0)   - register, not interested in readiness
 *            1 CURL_POLL_IN         - register, interested in read readiness
 *            2 CURL_POLL_OUT        - register, interested in write readiness
 *            3 CURL_POLL_INOUT      - register, interested in both
 *            4 CURL_POLL_REMOVE     - deregister
 */
#define CURL_POLL_NONE   0
#define CURL_POLL_IN     1
#define CURL_POLL_OUT    2
#define CURL_POLL_INOUT  3
#define CURL_POLL_REMOVE 4

#define CURL_EASY_NONE (CURL *)0
#define CURL_EASY_TIMEOUT (CURL *)0
#define CURL_SOCKET_TIMEOUT CURL_SOCKET_BAD

typedef int (*curl_socket_callback)(CURL *easy,      /* easy handle */
                                    curl_socket_t s, /* socket */
                                    int what,        /* see above */
                                    long ms,         /* timeout for wait */
                                    void *userp);    /* "private" pointer */

CURLMcode curl_multi_socket(CURLM *multi_handle,
                            curl_socket_t s,
                            CURL *easy,
                            curl_socket_callback callback,
                            void *userp); /* passed to callback */

CURLMcode curl_multi_socket_all(CURLM *multi_handle,
                                curl_socket_callback callback,
                                void *userp); /* passed to callback */

/*
 * Name:    curl_multi_timeout()
 *
 * Desc:    Returns the maximum number of milliseconds the app is allowed to
 *          wait before curl_multi_socket() or curl_multi_perform() must be
 *          called (to allow libcurl's timed events to take place).
 *
 * Returns: CURLM error code.
 */
CURLMcode curl_multi_timeout(CURLM *multi_handle, long *milliseconds);

#endif /* HAVE_CURL_MULTI_SOCKET */

#ifdef __cplusplus
} /* end of extern "C" */
#endif

#endif
