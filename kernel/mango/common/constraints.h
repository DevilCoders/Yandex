#pragma once

#include <util/system/defaults.h>

//#define ENABLE_MANGO_1_0_OPTIMISATION
#define ENABLE_MANGO_NEWS_AUTHORITY_PREDICTION
//#define MORPHO_DEBUG_OUT
//#define RELEV_DEBUG_OUT
//#define QUALITY_DEBUG_OUT

const size_t MAX_ANNOTATION_OUTCOMING_URLS    = 10;
const size_t MAX_HOST_LENGTH                  = 32;

const size_t MAX_RELEVANCE_WEIGHTY_WORDS      = 4;

const float  ZERO_EPS                         = 1e-20f;
const float  MATCH_EPS                        = 1e-6f;
const float  MATCH_ALL_WORD_QUORUM            = 1.0f-MATCH_EPS;
const float  DEFAULT_FADING                   = 0.2f;
const float  THEMATIC_DELTA                   = 3.0;
const size_t LANG_COUNT_BARRIER               = 3;

const int    DEFAULT_TIME_FROM                = 1262304000; // 01 Jan 2010 00:00:00
const int    DEFAULT_DECAY_PERIOD             = (365 * 24 * 3600); // 1 year

const size_t MAX_PROTOBUF_SIZE                = 64 * 1024 * 1024;
const size_t MAX_MR_KEY_SIZE                  = 4096;
