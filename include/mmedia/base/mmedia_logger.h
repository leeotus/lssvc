#ifndef __MMEDIA_LOGGER_H__
#define __MMEDIA_LOGGER_H__

#include "utils/lssvc_logstream.h"

#define RTMP_DEBUG_LOG 1

#if RTMP_DEBUG_LOG
#define RTMP_TRACE LSSVC_LOG_TRACE
#define RTMP_DEBUG LSSVC_LOG_DEBUG
#define RTMP_INFO LSSVC_LOG_INFO
#endif

#define RTMP_WARN LSSVC_LOG_INFO
#define RTMP_ERROR LSSVC_LOG_ERROR

#endif
