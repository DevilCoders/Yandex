#pragma once

#include <util/generic/vector.h>

#include <iterator>
#include <type_traits>


namespace NAntiRobot {


constexpr size_t DEFAULT_MAX_SCANNER_SIZE = 80000;


template <typename T>
struct TGlueScannersDefaultMerge {
    void operator()(T* src, T* dst) const {
        Y_UNUSED(src, dst);
    }
};


template <typename T, typename TGetScanner, typename TMerge = TGlueScannersDefaultMerge<T>>
void GlueScanners(
    TVector<T>* scanners,
    TGetScanner getScanner = {},
    TMerge merge = {},
    size_t maxScannerSize = DEFAULT_MAX_SCANNER_SIZE
) {
    using TScanner = std::remove_cvref_t<decltype(getScanner(scanners[0]))>;

    bool didGlue = true;

    while (didGlue && scanners->size() > 1) {
        didGlue = false;

        size_t readIndex = 0;
        size_t writeIndex = 0;

        while (readIndex + 1 < scanners->size()) {
            TScanner glued = TScanner::Glue(
                getScanner((*scanners)[readIndex]),
                getScanner((*scanners)[readIndex + 1]),
                maxScannerSize
            );

            if (glued.Empty()) {
                (*scanners)[writeIndex] = (*scanners)[readIndex];
                (*scanners)[writeIndex + 1] = (*scanners)[readIndex + 1];
                writeIndex += 2;
            } else {
                T scanner = (*scanners)[readIndex];
                getScanner(scanner) = std::move(glued);
                merge(&(*scanners)[readIndex + 1], &scanner);
                (*scanners)[writeIndex] = std::move(scanner);

                ++writeIndex;
                didGlue = true;
            }

            readIndex += 2;
        }

        if (readIndex + 1 == scanners->size()) {
            (*scanners)[writeIndex] = (*scanners)[readIndex];
            ++writeIndex;
        }

        scanners->resize(writeIndex);
    }

    scanners->shrink_to_fit();
}


} // namespace NAntiRobot
