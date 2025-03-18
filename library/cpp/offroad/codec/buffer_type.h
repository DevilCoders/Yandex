#pragma once

namespace NOffroad {
    // TODO:
    // rename
    // EEofMode
    // AutomaticEof
    // BlockLevelEof

    enum EBufferType {
        /**
         * Buffer with automatic EOF detection. Generally doesn't allow zero
         * elements except in special cases.
         */
        AutoEofBuffer,

        /**
         * Plain old buffer without any automagic. Takes any input.
         */
        PlainOldBuffer
    };

}
