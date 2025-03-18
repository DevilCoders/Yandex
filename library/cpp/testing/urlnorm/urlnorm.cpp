#include "urlnorm.h"

#include <library/cpp/testing/unittest/registar.h>

using namespace NUrlNormTest;

void NUrlNormTest::TestProjectorProperty(std::function<bool(const TString& url, TString& result)> normalizer) {
    const static TString urlsSample[] = {
        // Some random urls for sanity checks
        "ya.ru",
        "https://ya.ru:80/",
        "http://ya.ru:80",
        "http://YA.ru/anything?a=b=c",
        "vk.com/video_ext.php?oid=15942939&id=164342673&hash=040fa51a817c9a0b",
        "vk.com/video_ext.php?oid=15942939&id=164342673&hash=040fa51a817c9a0b",
        "http://vk.com/video?q=как%20доить%20корову&section=search&z=video199538437_169725330",
        "vk.com/video199538437_169725330",
        "http://vk.com/video?section=search&z=video199538437_169725330",
        "http://vk.com/video199538437_169725330",
        "vk.com/video?section=search&z=video199538437_169725330",
        "vk.com/video199538437_169725330",
        "http://vk.com/video_ext.php?oid=10562425&id=157041324&hash=b2528adebdc4e955",
        "http://vk.com/video10562425_157041324",
        "http://vk.com/video_ext.php?id=157041324&oid=10562425&hash=b2528adebdc4e955",
        "http://vk.com/video10562425_157041324",
        "http://vk.com/video_ext.php?oid=10562425&hash=b2528adebdc4e955&id=157041324",
        "http://vk.com/video10562425_157041324",
        "vk.com/video_ext.php?oid=10562425&id=157041324&hash=b2528adebdc4e955",
        "vk.com/video_ext.php?id=157041324&oid=10562425&hash=b2528adebdc4e955",
        "http://VK.com/video10562425_157041324",
        "vk.com/video_ext.php?hash=b2528adebdc4e955&oid=10562425&id=157041324",
        "vk.com/video10562425_157041324",
        "http://vk.com/video_ext.php?oid=-%0D%0A%0D%0A21680775&id=159356282&hash=3fcbe6daf3edd484&hd=3",
        "http://vk.com/video_ext.php?mrc217nyy0na",
    };

    for (const auto& data : urlsSample) {
        TString norm;
        if (normalizer(data, norm)) {
            TString normalizedTwice;
            UNIT_ASSERT(normalizer(norm, normalizedTwice));
            UNIT_ASSERT_VALUES_EQUAL(norm, normalizedTwice);
        }
    }
}
