#pragma once

namespace NFastRes2DataDelivery {
    namespace RTMR {
        static constexpr auto Table = "miniblender_wizards_data";

        static constexpr auto KeyPrefix = "miniblender";
        static constexpr auto KeyDelim = "\t";
        static constexpr auto KeyGtaName = "DataKeyRTMR";

        static constexpr auto RequestType = "fastres2-rtmr-data-request-group";
        static constexpr auto ResponseType = "fastres2-rtmr-get-records-response";
        static constexpr auto Client = "blender_miniblender_data";
    }
}