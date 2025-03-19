#pragma once

#include <util/system/defaults.h>

enum ESwitchType {
    SW_TR   = 0,      // 0
    SW_FADE = 1,      // 1
    SW_FADR = 2,      // 2
    SW_FAST = 3,      // 3
    SW_960 = 4,       // 4
    SW_5 = 5,         // 5
    SW_IMAGES = 6,    // 6
    SW_VIDEO = 7,     // 7
    SW_IMAGES_RQ = 8, // 8 (images related queries search type)
    SW_COLLECTIONS_BOARDS = 9, // 9 (collections boards search type)
    SW_FADR_SAAS = 10, // 10 (saas related search type)
    SW_LAST = 11,      // 11
};
