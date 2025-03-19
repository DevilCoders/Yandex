#pragma once

namespace NGeoDB {
    class TGeoKeeper;

    /* Return default instance of GeoDB.
     *
     * Please USE IT ONLY FOR PROTOTYPING, there is no guarantee that data will be fresh and sane.
     */
    const TGeoKeeper& DefaultGeoDB();
}
