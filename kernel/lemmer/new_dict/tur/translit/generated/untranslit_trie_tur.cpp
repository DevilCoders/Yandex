#include <kernel/lemmer/untranslit/trie/trie.h>

TTrieNode<wchar16> TrieUntranslitTur[] = {
    {0, 1U, 28U, 4294967295U},      //0
    {39, 4294967295U, 0U, 0U},      //1
    {45, 4294967295U, 0U, 1U},      //2
    {97, 4294967295U, 0U, 2U},      //3
    {98, 4294967295U, 0U, 3U},      //4
    {99, 43U, 1U, 4U},      //5
    {100, 42U, 1U, 6U},     //6
    {101, 4294967295U, 0U, 8U},     //7
    {102, 4294967295U, 0U, 9U},     //8
    {103, 4294967295U, 0U, 10U},        //9
    {104, 4294967295U, 0U, 11U},        //10
    {105, 41U, 1U, 12U},        //11
    {106, 4294967295U, 0U, 14U},        //12
    {107, 4294967295U, 0U, 15U},        //13
    {108, 4294967295U, 0U, 16U},        //14
    {109, 4294967295U, 0U, 17U},        //15
    {110, 39U, 1U, 18U},        //16
    {111, 37U, 2U, 20U},        //17
    {112, 4294967295U, 0U, 22U},        //18
    {113, 4294967295U, 0U, 23U},        //19
    {114, 4294967295U, 0U, 24U},        //20
    {115, 34U, 3U, 25U},        //21
    {116, 33U, 1U, 28U},        //22
    {117, 31U, 2U, 30U},        //23
    {118, 4294967295U, 0U, 32U},        //24
    {119, 4294967295U, 0U, 33U},        //25
    {120, 4294967295U, 0U, 34U},        //26
    {121, 29U, 1U, 35U},        //27
    {122, 4294967295U, 0U, 37U},        //28
    {111, 30U, 1U, 4294967295U},        //29
    {114, 4294967295U, 0U, 36U},        //30
    {101, 4294967295U, 0U, 31U},        //31
    {117, 4294967295U, 0U, 31U},        //32
    {104, 4294967295U, 0U, 29U},        //33
    {99, 4294967295U, 0U, 26U},     //34
    {104, 4294967295U, 0U, 27U},        //35
    {115, 4294967295U, 0U, 27U},        //36
    {101, 4294967295U, 0U, 21U},        //37
    {111, 4294967295U, 0U, 21U},        //38
    {100, 40U, 1U, 4294967295U},        //39
    {97, 4294967295U, 0U, 19U},     //40
    {105, 4294967295U, 0U, 13U},        //41
    {104, 4294967295U, 0U, 7U},     //42
    {99, 4294967295U, 0U, 5U},      //43
};

static const wchar16 st0[] = {39, 65535, 629, 8217, 65535, 762, 0};
static const wchar16 st1[] = {45, 65535, 1, 0};
static const wchar16 st2[] = {97, 65535, 492, 227, 65535, 946, 0};
static const wchar16 st3[] = {98, 65535, 1, 0};
static const wchar16 st4[] = {99, 65535, 681, 231, 65535, 707, 0};
static const wchar16 st5[] = {231, 65535, 2501, 0};
static const wchar16 st6[] = {100, 65535, 1, 0};
static const wchar16 st7[] = {287, 65535, 1340, 0};
static const wchar16 st8[] = {101, 65535, 1, 0};
static const wchar16 st9[] = {102, 65535, 1, 0};
static const wchar16 st10[] = {103, 65535, 676, 287, 65535, 711, 0};
static const wchar16 st11[] = {104, 65535, 1, 0};
static const wchar16 st12[] = {105, 65535, 982, 305, 65535, 1017, 238, 65535, 1335, 0};
static const wchar16 st13[] = {305, 65535, 2501, 0};
static const wchar16 st14[] = {106, 65535, 1, 0};
static const wchar16 st15[] = {107, 65535, 1, 0};
static const wchar16 st16[] = {108, 65535, 1, 0};
static const wchar16 st17[] = {109, 65535, 1, 0};
static const wchar16 st18[] = {110, 65535, 1, 0};
static const wchar16 st19[] = {305, 110, 100, 97, 65535, 3501, 0};
static const wchar16 st20[] = {111, 65535, 657, 246, 65535, 732, 0};
static const wchar16 st21[] = {246, 65535, 2501, 0};
static const wchar16 st22[] = {112, 65535, 1, 0};
static const wchar16 st23[] = {113, 65535, 1, 0};
static const wchar16 st24[] = {114, 65535, 1, 0};
static const wchar16 st25[] = {115, 65535, 669, 351, 65535, 719, 0};
static const wchar16 st26[] = {351, 231, 65535, 1340, 0};
static const wchar16 st27[] = {351, 65535, 2501, 0};
static const wchar16 st28[] = {116, 65535, 1, 0};
static const wchar16 st29[] = {351, 65535, 1340, 0};
static const wchar16 st30[] = {117, 65535, 983, 252, 65535, 1013, 251, 65535, 1340, 0};
static const wchar16 st31[] = {252, 65535, 1340, 0};
static const wchar16 st32[] = {118, 65535, 1, 0};
static const wchar16 st33[] = {119, 65535, 1, 0};
static const wchar16 st34[] = {120, 65535, 1, 0};
static const wchar16 st35[] = {121, 65535, 1, 305, 65535, 2501, 0};
static const wchar16 st36[] = {305, 121, 111, 114, 65535, 2501, 0};
static const wchar16 st37[] = {122, 65535, 1, 0};

const wchar16 *TrieUntranslitTurData[] = {
    st0, st1, st2, st3, st4, st5, st6, st7, st8, st9, st10, st11, st12, st13, st14, st15, st16,
    st17, st18, st19, st20, st21, st22, st23, st24, st25, st26, st27, st28, st29, st30, st31, st32,
    st33, st34, st35, st36, st37
};
