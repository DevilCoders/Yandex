#include <library/cpp/testing/unittest/registar.h>

#include "yx_searchprefs.h"

#include <library/cpp/http/cookies/cookies.h>

namespace NAntiRobot {
    const char* cookies01NoYx = "yandexuid=1816372711256732340; L=X30MD3dDVmJEQ3lgSGhEa0BBXQBmR197BAsnDyQFGggkNCkPWhN+BVo0JggjfkNEBjMjDTpNOh07cwENLgIiUQ==.1263902325.6577.230839.dd0fcd9b6dcad6cd742797c0e94da300; yabs-frequency=/2/Zhjt0Ce58LR3PS371IS00040/; my=YxoBASMCAQAnAgAALQECLgEBAA==; fuid01=4ae836b40ad39097.sdSxOQXYVFxrb2F_arBryRVZFWlTySH5Uv_ziVg7UgImKoZ6jn4aA1KXpfaSOXB9lKCgD_wrVUeNK5Jh1ilF1Vp9mbXFtdarj-9QislNDNYLbFJuIARrGsKOMS2DCpVX; Virtual_id=29; yandex_login=angersm; Session_id=1263902325.1054.3.18186394.2:43596182:310.8:1263902325935:1570676525:16.63889.13362.f8151857c49cbaf2df701595fa765a68; narod_login=angersm; t=w; yandex_gid=10002; S=14002.barff";

    const char* cookies02Lr0Numdoc50 = "yandexuid=1816372711256732340; L=X30MD3dDVmJEQ3lgSGhEa0BBXQBmR197BAsnDyQFGggkNCkPWhN+BVo0JggjfkNEBjMjDTpNOh07cwENLgIiUQ==.1263902325.6577.230839.dd0fcd9b6dcad6cd742797c0e94da300; yabs-frequency=/2/Zhjt0Ce58LR3PS371IS00040/; my=YxoBASMCAQAnAgAALQECLgEBAA==; fuid01=4ae836b40ad39097.sdSxOQXYVFxrb2F_arBryRVZFWlTySH5Uv_ziVg7UgImKoZ6jn4aA1KXpfaSOXB9lKCgD_wrVUeNK5Jh1ilF1Vp9mbXFtdarj-9QislNDNYLbFJuIARrGsKOMS2DCpVX; YX_SEARCHPREFS=favicons:1,shsd:,ton:1,banners:1,search_form:top,tose:1,numdoc:50,target:_blank,desc:sometimes,wstat:,lr:,imgflt:1,t:2,lang:all; Virtual_id=29; yandex_login=angersm; Session_id=1263902325.1054.3.18186394.2:43596182:310.8:1263902325935:1570676525:16.63889.13362.f8151857c49cbaf2df701595fa765a68; narod_login=angersm; t=w; yandex_gid=10002; S=14002.barff";

    const char* cookies03NoNumdoc = "yandexuid=1816372711256732340; L=X30MD3dDVmJEQ3lgSGhEa0BBXQBmR197BAsnDyQFGggkNCkPWhN+BVo0JggjfkNEBjMjDTpNOh07cwENLgIiUQ==.1263902325.6577.230839.dd0fcd9b6dcad6cd742797c0e94da300; yabs-frequency=/2/Zhjt0Ce58LR3PS371IS00040/; my=YxoBASMCAQAnAgAALQECLgEBAA==; fuid01=4ae836b40ad39097.sdSxOQXYVFxrb2F_arBryRVZFWlTySH5Uv_ziVg7UgImKoZ6jn4aA1KXpfaSOXB9lKCgD_wrVUeNK5Jh1ilF1Vp9mbXFtdarj-9QislNDNYLbFJuIARrGsKOMS2DCpVX; YX_SEARCHPREFS=favicons:1,shsd:,ton:1,banners:1,search_form:top,tose:1,target:_blank,desc:sometimes,wstat:,lr:,imgflt:1,t:2,lang:all; Virtual_id=29; yandex_login=angersm; Session_id=1263902325.1054.3.18186394.2:43596182:310.8:1263902325935:1570676525:16.63889.13362.f8151857c49cbaf2df701595fa765a68; narod_login=angersm; t=w; yandex_gid=10002; S=14002.barff";

    const char* cookies04NoLr = "yandexuid=1816372711256732340; L=X30MD3dDVmJEQ3lgSGhEa0BBXQBmR197BAsnDyQFGggkNCkPWhN+BVo0JggjfkNEBjMjDTpNOh07cwENLgIiUQ==.1263902325.6577.230839.dd0fcd9b6dcad6cd742797c0e94da300; yabs-frequency=/2/Zhjt0Ce58LR3PS371IS00040/; my=YxoBASMCAQAnAgAALQECLgEBAA==; fuid01=4ae836b40ad39097.sdSxOQXYVFxrb2F_arBryRVZFWlTySH5Uv_ziVg7UgImKoZ6jn4aA1KXpfaSOXB9lKCgD_wrVUeNK5Jh1ilF1Vp9mbXFtdarj-9QislNDNYLbFJuIARrGsKOMS2DCpVX; YX_SEARCHPREFS=favicons:1,shsd:,ton:1,banners:1,search_form:top,tose:1,numdoc:50,target:_blank,desc:sometimes,wstat:,imgflt:1,t:2,lang:all; Virtual_id=29; yandex_login=angersm; Session_id=1263902325.1054.3.18186394.2:43596182:310.8:1263902325935:1570676525:16.63889.13362.f8151857c49cbaf2df701595fa765a68; narod_login=angersm; t=w; yandex_gid=10002; S=14002.barff";

    const char* cookies05Spaces = "yandexuid=1816372711256732340; L=X30MD3dDVmJEQ3lgSGhEa0BBXQBmR197BAsnDyQFGggkNCkPWhN+BVo0JggjfkNEBjMjDTpNOh07cwENLgIiUQ==.1263902325.6577.230839.dd0fcd9b6dcad6cd742797c0e94da300; yabs-frequency=/2/Zhjt0Ce58LR3PS371IS00040/; my=YxoBASMCAQAnAgAALQECLgEBAA==; fuid01=4ae836b40ad39097.sdSxOQXYVFxrb2F_arBryRVZFWlTySH5Uv_ziVg7UgImKoZ6jn4aA1KXpfaSOXB9lKCgD_wrVUeNK5Jh1ilF1Vp9mbXFtdarj-9QislNDNYLbFJuIARrGsKOMS2DCpVX; YX_SEARCHPREFS=favicons:1,shsd:,ton:1,banners:1,search_form:top,tose:1,numdoc: 40,target:_blank,desc:sometimes,wstat:,lr: 123,imgflt:1,t:2,lang:all; Virtual_id=29; yandex_login=angersm; Session_id=1263902325.1054.3.18186394.2:43596182:310.8:1263902325935:1570676525:16.63889.13362.f8151857c49cbaf2df701595fa765a68; narod_login=angersm; t=w; yandex_gid=10002; S=14002.barff";

    static void SetCookies(THttpCookies& reqParams, const char* cookies) {
        reqParams.Scan(cookies);
    }

    Y_UNIT_TEST_SUITE(TTestYxSearchPrefs) {
        Y_UNIT_TEST(TestYxSearchPrefs) {
            THttpCookies reqParams;

            {
                SetCookies(reqParams, cookies01NoYx);
                TYxSearchPrefs yx;
                yx.Init(reqParams);
                UNIT_ASSERT_EQUAL(yx.NumDoc, 10);
                UNIT_ASSERT_EQUAL(yx.Lr, -1);
            }

            {
                SetCookies(reqParams, cookies02Lr0Numdoc50);
                TYxSearchPrefs yx;
                yx.Init(reqParams);
                UNIT_ASSERT_EQUAL(yx.NumDoc, 50);
                UNIT_ASSERT_EQUAL(yx.Lr, 0);
            }

            {
                SetCookies(reqParams, cookies03NoNumdoc);
                TYxSearchPrefs yx;
                yx.Init(reqParams);
                UNIT_ASSERT_EQUAL(yx.NumDoc, 10);
                UNIT_ASSERT_EQUAL(yx.Lr, 0);
            }

            {
                SetCookies(reqParams, cookies04NoLr);
                TYxSearchPrefs yx;
                yx.Init(reqParams);
                UNIT_ASSERT_EQUAL(yx.NumDoc, 50);
                UNIT_ASSERT_EQUAL(yx.Lr, -1);
            }

            {
                SetCookies(reqParams, cookies05Spaces);
                TYxSearchPrefs yx;
                yx.Init(reqParams);
                UNIT_ASSERT_EQUAL(yx.NumDoc, 40);
                UNIT_ASSERT_EQUAL(yx.Lr, 123);
            }
        }
    }
}
