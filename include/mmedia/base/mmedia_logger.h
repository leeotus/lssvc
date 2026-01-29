#ifndef __MMEDIA_LOGGER_H__
#define __MMEDIA_LOGGER_H__

#include "utils/lssvc_logstream.h"

#define RTMP_DEBUG_LOG 1
#define HTTP_DEBUG_LOG 1

#if RTMP_DEBUG_LOG
#define RTMP_TRACE LSSVC_LOG_TRACE << "[RTMP]"
#define RTMP_DEBUG LSSVC_LOG_DEBUG << "[RTMP]"
#define RTMP_INFO LSSVC_LOG_INFO << "[RTMP]"
#else
#define RTMP_TRACE                                                             \
  if (0)                                                                       \
  LSSVC_LOG_TRACE
#define RTMP_DEBUG                                                             \
  if (0)                                                                       \
  LSSVC_LOG_DEBUG
#define RTMP_INFO                                                              \
  if (0)                                                                       \
  LSSVC_LOG_INFO
#endif

#define RTMP_WARN LSSVC_LOG_WARN
#define RTMP_ERROR LSSVC_LOG_ERROR

#if HTTP_DEBUG_LOG
#define HTTP_TRACE LSSVC_LOG_TRACE << "[HTTP]"
#define HTTP_DEBUG LSSVC_LOG_DEBUG << "[HTTP]"
#define HTTP_INFO LSSVC_LOG_INFO << "[HTTP]"
#else
#define HTTP_TRACE                                                             \
  if (0)                                                                       \
  LSSVC_LOG_TRACE
#define HTTP_DEBUG                                                             \
  if (0)                                                                       \
  LSSVC_LOG_DEBUG
#define HTTP_INFO                                                              \
  if (0)                                                                       \
  LSSVC_LOG_INFO
#endif

#define HTTP_WARN LSSVC_LOG_WARN
#define HTTP_ERROR LSSVC_LOG_ERROR

#endif
