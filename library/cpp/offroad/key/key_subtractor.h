#pragma once

namespace NOffroad {
    /**
     * Subtraction mode for subsequent keys in key encoder/decoder.
     */
    enum EKeySubtractor {
        /** Keys are delta-encoded. */
        DeltaKeySubtractor,

        /** Keys are stored as is. */
        IdentityKeySubtractor
    };

}
