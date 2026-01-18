#ifndef __LIVE_LOGGER_H__
#define __LIVE_LOGGER_H__

#include "utils/lssvc_logstream.h"

#define LIVE_DEBUG_LOG 1

#if LIVE_DEBUG_LOG
#define LIVE_TRACE LSSVC_LOG_TRACE << "[LIVE]"
#define LIVE_DEBUG LSSVC_LOG_DEBUG << "[LIVE]"
#define LIVE_INFO LSSVC_LOG_INFO << "[LIVE]"
#else
#define LIVE_TRACE                                                             \
  if (0)                                                                       \
  LSSVC_LOG_TRACE
#define LIVE_DEBUG                                                             \
  if (0)                                                                       \
  LSSVC_LOG_DEBUG
#define LIVE_INFO                                                              \
  if (0)                                                                       \
  LSSVC_LOG_INFO
#endif

#define LIVE_WARN LSSVC_LOG_WARN
#define LIVE_ERROR LSSVC_LOG_ERROR

#endif
