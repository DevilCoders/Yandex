#pragma once

namespace NSnippets {

    enum ETextCutMethod {
        TCM_PIXEL,
        TCM_SYMBOL,
    };

    enum EInfoRequestType {
        INFO_NONE = 0,
        INFO_SNIPPETS = (1 << 0),
        INFO_SNIPPET_HITS = (1 << 1)
    };

    enum ETitleGeneratingAlgo {
        // choosing fragment which seems to be the most relevant to a given query
        TGA_SMART /* "smart" */,

        // always choosing title prefix modulo definition
        TGA_PREFIX /* "prefix" */,

        // titles which do not fit entirely on a single line are replaced by the empty ones,
        TGA_NAIVE /* "naive" */,

        // choosing relevant but yet readable fragment (only one ellipsis is allowed)
        TGA_READABLE /* "readable" */,

        // choosing fragment that can be shown without left ellipsis
        TGA_PREFIX_LIKE /* "prefix_like" */
    };

    enum ETitleDefinitionMode {
        TDM_IGNORE /* "ignore" */,
        TDM_USE /* "use" */,
        TDM_ELIMINATE /* "eliminate" */
    };

    enum EDefSearchResultType {
        NOT_FOUND = 0,
        BACKWARD,
        FORWARD
    };

    enum EPassageReplyFillerType {
        PRFT_DEFAULT = 0,
        PRFT_IMAGES = 1,
        PRFT_VIDEO = 2,

        PRFT_COUNT
    };

}
