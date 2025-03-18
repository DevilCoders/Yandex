#pragma once

namespace NOffroad {
    enum EFlatType {
        /** Only the last occurrence of equal elements will be written to FlatWriter */
        UniqueFlatType,

        /** All instances will be written to FlatWriter */
        DefaultFlatType
    };

} //namespace NOffroad
