#pragma once

#define XPATHMARKER_LOG_LEVEL_NONE 0
#define XPATHMARKER_LOG_LEVEL_ERROR 1
#define XPATHMARKER_LOG_LEVEL_DEBUG 2
#define XPATHMARKER_LOG_LEVEL_ALL 3

#define XPATHMARKER_LOG_LEVEL XPATHMARKER_LOG_LEVEL_NONE

#if XPATHMARKER_LOG_LEVEL == XPATHMARKER_LOG_LEVEL_ALL
   #define XPATHMARKER_ERROR(x) Clog << "[TXPathMarker@" << __FILE__ << ":" << __LINE__ << "] ERROR: " << x << Endl;
   #define XPATHMARKER_DEBUG(x) Clog << "[TXPathMarker@" << __FILE__ << ":" << __LINE__ << "] DEBUG: " << x << Endl;
   #define XPATHMARKER_INFO(x) Clog << "[TXPathMarker@" << __FILE__ << ":" << __LINE__ << "] INFO: " << x << Endl;
#endif

#if XPATHMARKER_LOG_LEVEL == XPATHMARKER_LOG_LEVEL_DEBUG
    #define XPATHMARKER_ERROR(x) Clog << "[TXPathMarker@" << __FILE__ << ":" << __LINE__ << "] ERROR: " << x << Endl;
    #define XPATHMARKER_DEBUG(x) Clog << "[TXPathMarker@" << __FILE__ << ":" << __LINE__ << "] DEBUG: " << x << Endl;
    #define XPATHMARKER_INFO(x)
#endif

#if XPATHMARKER_LOG_LEVEL == XPATHMARKER_LOG_LEVEL_ERROR
    #define XPATHMARKER_ERROR(x) Clog << "[TXPathMarker@" << __FILE__ << ":" << __LINE__ << "] ERROR: " << x << Endl;
    #define XPATHMARKER_DEBUG(x)
    #define XPATHMARKER_INFO(x)
#endif

#if XPATHMARKER_LOG_LEVEL == XPATHMARKER_LOG_LEVEL_NONE
    #define XPATHMARKER_ERROR(x)
    #define XPATHMARKER_DEBUG(x)
    #define XPATHMARKER_INFO(x)
#endif

#if XPATHMARKER_LOG_LEVEL >= XPATHMARKER_LOG_LEVEL_DEBUG
#pragma message("TXPathMarker is compiled with debug output on, which will dramatically decrease efficiency. This mode must not be used in production")
#endif

