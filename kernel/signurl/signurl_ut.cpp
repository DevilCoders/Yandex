#include "signurl.h"

#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/unittest/tests_data.h>
#include <library/cpp/testing/unittest/env.h>
#include <util/generic/string.h>
#include <util/generic/ptr.h>
#include "keyholder.h"
#include <util/datetime/base.h>
#include <util/system/datetime.h>

#define KEYS_PATH "kernel/signurl/ut/clickdaemon.keys"

using namespace NSignUrl;

Y_UNIT_TEST_SUITE(TOtherParams) {
    Y_UNIT_TEST(UrlEscapeCopy) {
        TString from = "Hello\10";
        TString text = "World\10";

        TOtherParams params;
        params.From = from;
        params.Text = text;

        TOtherParams escaped = params.UrlEscapeCopy();
        UNIT_ASSERT_UNEQUAL(&params, &escaped);
        UNIT_ASSERT_UNEQUAL(params.From, escaped.From);
        UNIT_ASSERT_UNEQUAL(params.Text, escaped.Text);

        UrlEscape(from);
        UrlEscape(text);
        UNIT_ASSERT_EQUAL(from, escaped.From);
        UNIT_ASSERT_EQUAL(text, escaped.Text);
        UNIT_ASSERT_UNEQUAL(from, params.From);
        UNIT_ASSERT_UNEQUAL(text, params.Text);
    }
}

Y_UNIT_TEST_SUITE(SignUrlTest) {
    Y_UNIT_TEST(SignVerifyTest) {
        THolder<NSignUrl::TSignKeys> SignKeys;
        try {
            SignKeys = MakeHolder<NSignUrl::TSignKeys>(ArcadiaSourceRoot() + "/" + KEYS_PATH);
        }
        catch (const yexception e) {
            Cerr << "Err while reading keys!\n";
            Cerr << e.what();
        }

        NSignUrl::TKey zero_key = SignKeys->GetKeyData("0");
        NSignUrl::TKey first_key = SignKeys->GetKeyData("1");

        TString to_sign = "My Test String";
        char signbuf[SU_SIGN_SZ];

        Sign(to_sign.data(), to_sign.size(), signbuf, zero_key);
        TString sign = TString(signbuf);

        UNIT_ASSERT_EQUAL(Verify(sign.data(), to_sign.data(), to_sign.size(), zero_key), true);

        Sign(to_sign.data(), to_sign.size(), signbuf, first_key);
        sign = TString(signbuf);

        UNIT_ASSERT_UNEQUAL(Verify(sign.data(), to_sign.data(), to_sign.size(), zero_key), true);

        TString incorrect_url = "/redir/zRrSeA3aY6jDJai6l6dG4NempTAnFQ6F?data=UlNrNmk5WktYejR0eWJFYk1LdmtxdTdpcGcxUm44S1NFNndpaktDRXdUYXFfX1hOMTVPWHBVcGVRQjJoQ29maWRJbVNuTTh2aGJPMmFHMGNPNFBpSEpkN1JKeG41cU51dFdsQzVpWjlUYWxHWEdCTVpYSll2ZlREclBUazVqZFROaG9HRU1mOTZJWnd1emNZX2JDNzk5bWJMOXppT3hxYWxnd1B5SDJyRk56aG5IR3k0eXJHX3ViaDdGa0Ryc2Y3Nmk4RHE4SldST1JtQllVSG1kU1ZFQS1YWmY2c0I0ZTJEcHBmTzgxWVo1cFh3bnZHNkpiWUJKQzBrZVNUd1c4MmduZTdScU9MT2J0ZGZ5d2VDX2xscFE&b64e=2&sign=2d40be61d11bade26bde1e0f7e96c1d3&keyno=0&cst=AiuY0DBWFJ5fN_r-AEszkwR-Yb62DjKSukXEdl-SoUGifwfgmYymRAS-l72Uf_rjhjlqVCLCis4ZG_Gnb-uguZLJ33jB5d4183C6s0MLpVuXdsRSKzeMc398fSXgRQ7QuV6XS-elYQfpzflhO6foSJXy7x7H6vO66asTyBTEEli54gujafpnxdQHpJxjNx2BxC8IxJJHUgc&ref=orjY4mGPRjl998yCZXrGI_HReRfhPfhTVQJf90xTCYEW3typ3KrNJNPKhdUHiIf3eFB2Kd2nOeKso_lNq2Azpw";
        TUnsignedUrl un_u = UnsignUrl(incorrect_url, SignKeys.Get());
        un_u.Dump();
        UNIT_ASSERT_EQUAL(un_u.Verified, 1);
        UNIT_ASSERT_EQUAL(un_u.Url, "http://www.example.com/%0D%0ASet-Cookie:injected=CRLF;%20Expires=Fri,%2005-Mar-2027%2011:33:21%20GMT;%20Domain=.yandex.ru;%20Path=/");

        TString goog_cpp_url = "/jsredir?from=gas.serp.yandex.ru%3Bsearch%2F%3Bweb%3B%3B&text=&etext=1150.Cs4IJzRo6bk4R3jtFfwV4vs7QV3y_aE26mx6kR0rIII.7ed6960fdc87955daebb64d096186c52f9728b7e&uuid=&state=zRrSeA3aY6h77jydLxkYiPMTcIdGYjUu&data=UlNrNmk5WktYejR0eWJFYk1LdmtxdXhXMmR3bGZ5cW51SlVKblgwaTEzLXhaSkRmbDA2bTlsYUlpS3BROEhmaWNzNlNBR2ZSdzQ2RWVuRDk3MlFiYlg1QVJGS2F2QmlHemNGV0w2R3JsUUJTU08wS2gwYXVHc1ZwcndFeUVSZE9iM3hQUHhONmJNbzNoRU8wWEpMZUNGVVdHYTFJY256WA&b64e=2&sign=db43442fd8f33b92a9761a7f5b1e8fcb&keyno=0&cst=AiuY0DBWFJ5fN_r-AEszkwR-Yb62DjKSukXEdl-SoUGifwfgmYymRAS-l72Uf_rjhjlqVCLCis4ZG_Gnb-uguZLJ33jB5d4183C6s0MLpVuXdsRSKzeMcyHqTBCdIP7JRupeyOFDz1S2P73AYdTG-NEICYinCHagyFqHJGukt-jAU2X0JPQrhhBmN-HNakxMZxU4_3z24_E&ref=orjY4mGPRjl998yCZXrGI_HReRfhPfhTVQJf90xTCYEW3typ3KrNJJ6a4P7jMiwpa6F3TzmHrWg";
        TUnsignedUrl uns_gg = UnsignUrl(goog_cpp_url, SignKeys.Get());
        uns_gg.Dump();
        UNIT_ASSERT_EQUAL(uns_gg.Verified, 1);
        UNIT_ASSERT_EQUAL(uns_gg.Url, "http://www.google.ru/search?ie=UTF-8&hl=ru&q=c%2B%2B");

        TUnsignedUrl uns2;
        TString yabs_url = "/redir/dtype=stred/sid=1478168202.42225.20955.22657/pid=198/cid=198/path=tech.portal-ads.teaser/vars=-reqid=1478168202433-596506-sas1-0249-ATOMS-distr_portal,-showid=020134301478168202433596506102,-device=desktop,-bannerid=147789410273,-score=4,-lr=20716,-product=browser,-adata=dzQDs3BxojRsrEnonQrT_mj6Doq_cac_Rr9ob2S5E1s=,-eventtype=show/*data=url%3Dhttps%253A%252F%252Fyabs.yandex.ru%252Fresource%252FnZv_s3mAYUOlyvfhi_jYE_banana_20141031_logolock.png%26ts%3D1478168202%26uid%3D6849989501478006532&sign=3a7a3c84946b59a713b5d458da31fd3d&keyno=12";
        uns2 = UnsignUrl(yabs_url, SignKeys.Get());
        uns2.Dump();
        UNIT_ASSERT_EQUAL(uns2.Verified, 1);

        TString b64e1_url = "/redir/V3Y0P+MO/zUp7n9WDxXf6YiE2kT8Sb5STTPuAKSJsGIalyKh0lP5nSOXf8fwpYOfCCoWEAigZXA=data=RSk6i9ZKXz4tybEbMKvkqrJk5ux3usP5Bd2p4X5W9K7EhFFMarcCfeTFaFPS6FvYxTK%2FtjyQnLWgy1TxqOwrC3zDeGTTUPMLzBY0Ia4qJVm2IRI1FlxPwCSYxmBQBGWMjWu3AB2h7tjF4QBKlNkrVAbcllSNxJLo7NM54yWlUoLMvqWP4fsNgQ%3D%3D&b64e=1&sign=2b5c98b730f67b66054b9321d58d77f9&keyno=0";
        uns2 = UnsignUrl(b64e1_url, SignKeys.Get());
        uns2.Dump();
        UNIT_ASSERT_EQUAL(uns2.Verified, 1);

        TString dv_url = "/redir/dv/*data=url%3Dhttp%253A%252F%252Fwww.sberbank-ast.ru%252FpurchaseList.aspx%26ts%3D1478092391%26uid%3D1893785571477737829&sign=b60a314a8e14573fb423fad391920700&keyno=1";
        uns2 = UnsignUrl(dv_url, SignKeys.Get());
        UNIT_ASSERT_EQUAL(uns2.Verified, 1);
        UNIT_ASSERT_EQUAL(uns2.Url, "http://www.sberbank-ast.ru/purchaseList.aspx");

        TString suggest2_url = "/jsredir?text=credit%20suisse&from=yandex.ru%3Bsuggest%3Bweb&state=dtype%3Dstred%2Fpid%3D1%2Fcid%3D71613%2Fpart%3Dcredit%2Fsuggestion%3Dcredit%2520suisse%2Fregion%3D1%2F%2A&data=url%3Dhttps%253A%252F%252Fwww.credit-suisse.com%252Fru%252Fru%252F%26ts%3D1477648033%26uid%3D9460222871467791855&sign=e445318095e4e63c4066f5b5f64349be&ref=https%3A%2F%2Fwww.yandex.ru%2F&keyno=0&l10n=ru";
        uns2 = UnsignUrl(suggest2_url, SignKeys.Get());
        uns2.Dump();
        UNIT_ASSERT_EQUAL(uns2.Verified, 1);

        TUnsignedUrl uns;

        TString jsredirred_string = "http://yandex.ru/clck/jsredir?from=yandex.ru%3Bsearch%2F%3Bweb%3B%3B&text=&etext=1169.jLQy_5j7K7fQRnz3eKjgX3fW1amq2PBzfzOXPYlIQxw.7f295b168ee07dfa17c3f0a2650c51e81924a86a&uuid=&state=PEtFfuTeVD5kpHnK9lio9bb4iM1VPfe4W5x0C0-qwflIRTTifi6VAA&data=UlNrNmk5WktYejR0eWJFYk1LdmtxaTBPYWVmV3pfYThKMUI5aVlYYmctalBHVW5XYWpxLXlwOUxNWEZRYXE5RUF2UkloYWhOamlGMC1HUGNnNnJnRktKZFUwNE0tZFc0&b64e=2&sign=55406497522e5489d572e460b61b460d&keyno=8&cst=AiuY0DBWFJ5fN_r-AEszkwR-Yb62DjKSukXEdl-SoUGifwfgmYymRAS-l72Uf_rjzSEDIHNP-WKpk54cMScDjaYQ5lavxmQGZbAy32JB4voNv5OHHyOnfeBbQ84ww96FYirIOcrnm1cbasygUUPjPPFzIN-VmniecabH9pM1iklhA8XNTUIjCFktoQ-iQYC1G0eyNuihjjGmtTf47VzoLTU3qnHI71rvPwR20Bp_-uHD4Q6iyitUV3LD8q-eDSSQiBbz9bB6JI_IiWExBlwLSoOJf6kn_6tscSAvTb_d_vntMiUyI6YD__FrPdJ3e0b38mLNI5bx_0kPfH99-U1jaV_1odhZdPU-wmnVjNW1miJeJ291jKM6tODSZtX5SH537smbvKBb6m8kGAKIlHKO4JawGVpxw0X0uFTRQ-dlCk0WEGm3iZGCjurobYA5NUuiVjVGh6kMSrrKhKnRMdrvFCh1SVg7SRDWnzjbkp5O9idAYMW-4o06TZONbAk_0uGG90XURFI8tohu8g6vDEYmBkMtC2eNQjVGGgM4tJipP_yFNtFaPZT3m9Qa1hUJhvigvLXtACXnIZzRYggI4uuGijamwarQ9V2h5jszDAL9Y_m4b8r7AoXs_3lR1U1grzMKXDmaCbPg14gDvYkumT2XaXCcj8juaJWOrpwcGOEKvYDt4iisyedUkniIdhaguxnQEotf93Pjj_XxL1XVeUcaE0hGnXovntSxRWecZyfehAzFX5Y4rOmxcS2Bom6pWuLc1xB693odHoFoFMBLb2WeZDMpggUjjh74jsPSg5FIiz7kAsneBoAft6spWfUATixxvWSgHxNCeeVYqhy88dO1BnGFhunmTt0n&ref=orjY4mGPRjk5boDnW0uvlrrd71vZw9kp5uQozpMtKCUqbGEGmzdoDngrF7K9RCPv&l10n=ru";
        uns = UnsignUrl(jsredirred_string, SignKeys.Get());
        uns.Dump();
        UNIT_ASSERT_EQUAL(uns.Verified, 1);

        TString httpredirred_string = "http://yandex.ru/clck/httpredir?from=yandex.ru%3Bsearch%2F%3Bweb%3B%3B&text=&etext=1169.jLQy_5j7K7fQRnz3eKjgX3fW1amq2PBzfzOXPYlIQxw.7f295b168ee07dfa17c3f0a2650c51e81924a86a&uuid=&state=PEtFfuTeVD5kpHnK9lio9bb4iM1VPfe4W5x0C0-qwflIRTTifi6VAA&data=UlNrNmk5WktYejR0eWJFYk1LdmtxaTBPYWVmV3pfYThKMUI5aVlYYmctalBHVW5XYWpxLXlwOUxNWEZRYXE5RUF2UkloYWhOamlGMC1HUGNnNnJnRktKZFUwNE0tZFc0&b64e=2&sign=55406497522e5489d572e460b61b460d&keyno=8&cst=AiuY0DBWFJ5fN_r-AEszkwR-Yb62DjKSukXEdl-SoUGifwfgmYymRAS-l72Uf_rjzSEDIHNP-WKpk54cMScDjaYQ5lavxmQGZbAy32JB4voNv5OHHyOnfeBbQ84ww96FYirIOcrnm1cbasygUUPjPPFzIN-VmniecabH9pM1iklhA8XNTUIjCFktoQ-iQYC1G0eyNuihjjGmtTf47VzoLTU3qnHI71rvPwR20Bp_-uHD4Q6iyitUV3LD8q-eDSSQiBbz9bB6JI_IiWExBlwLSoOJf6kn_6tscSAvTb_d_vntMiUyI6YD__FrPdJ3e0b38mLNI5bx_0kPfH99-U1jaV_1odhZdPU-wmnVjNW1miJeJ291jKM6tODSZtX5SH537smbvKBb6m8kGAKIlHKO4JawGVpxw0X0uFTRQ-dlCk0WEGm3iZGCjurobYA5NUuiVjVGh6kMSrrKhKnRMdrvFCh1SVg7SRDWnzjbkp5O9idAYMW-4o06TZONbAk_0uGG90XURFI8tohu8g6vDEYmBkMtC2eNQjVGGgM4tJipP_yFNtFaPZT3m9Qa1hUJhvigvLXtACXnIZzRYggI4uuGijamwarQ9V2h5jszDAL9Y_m4b8r7AoXs_3lR1U1grzMKXDmaCbPg14gDvYkumT2XaXCcj8juaJWOrpwcGOEKvYDt4iisyedUkniIdhaguxnQEotf93Pjj_XxL1XVeUcaE0hGnXovntSxRWecZyfehAzFX5Y4rOmxcS2Bom6pWuLc1xB693odHoFoFMBLb2WeZDMpggUjjh74jsPSg5FIiz7kAsneBoAft6spWfUATixxvWSgHxNCeeVYqhy88dO1BnGFhunmTt0n&ref=orjY4mGPRjk5boDnW0uvlrrd71vZw9kp5uQozpMtKCUqbGEGmzdoDngrF7K9RCPv&l10n=ru";
        uns = UnsignUrl(httpredirred_string, SignKeys.Get());
        uns.Dump();
        UNIT_ASSERT_EQUAL(uns.Verified, 1);

        TString jsred2 = "http://yandex.ru/clck/jsredir?from=yandex.ru%3Bsearch%2F%3Bweb%3B%3B&text=&etext=1170.u5qMG4P22R4abn4kqBKTdq6JdU2H108Sg1JLyaaeBZI.1ec58198385713d21a8e7ee720b7991f6db2bb6c&uuid=&state=PEtFfuTeVD5kpHnK9lio9WCnKp0DidhEnJgmlcOG45rpVexZT92IDjgxNeXhzJ37FOWlaNDkK6k&data=UlNrNmk5WktYejR0eWJFYk1LdmtxZ2k5WG5pWlg1UzRwM1ZDTVRmRWZQbEhyV3VVcV9seXRwSVNadDRoenpucHVMbGV1Vjg0VmQtaVlvLXVzTVVmYXFsYm55bHJGci1s&b64e=2&sign=a17d1d0fb164935772e80c2e0cb6de95&keyno=0&cst=AiuY0DBWFJ5fN_r-AEszkwR-Yb62DjKSukXEdl-SoUGifwfgmYymRAS-l72Uf_rjzSEDIHNP-WK6-6g-qV8IpVbTnJVbF_a970pBY7FnnIGGp4ZERRAldkKFAP1TtnpficVAmXzxpQ4yE5tDceMw-vWR0FG05x1pNDNbp_G5GTo14MYMWL-q9WmHIvA1oVVew5l7h3XLPnYSneA-yPzbk5BevLMRO6-BIeIt6j0_C2a1FFj8GrmgXIJV2o4-OXH-9xGpXZkpqCC1ATinOoHxiirkN_y9e2s5-8_QHp2tKkUSPqC6bxx0CqQDeNGEE5NUSkpe-LZ-vr-0IMvpRSPqW7qCbAc6dmmB-pY8MZ1mNJhqI7bCnJo4-rj3H20_macTvwwV65T2KC3BEalENY9x0EK1aGnGDLgWhdCZ-GlLDW2LjupiN6j9aSFpWROzedFHDZaPZ7xiOOxk4hAOtlS07AQ1yWnPfnhEhC9BBXl5Jdw0euiEF7OZdtCqnzmH7LEK9_Kl9B6iX6s7uAa9DowuFV-1mG0qSzRN0ChHC_vZV3dpjv5mZhAwZkSgb5obexKonvj_U3yCP1A61sepb6ZKHZ1sHo9ccB9kUutjhZVXySk1nGm9Y2ePY6-Nbe7BQh-r5H4dj_mPkNcr9nvOdZDktE5RwaN2_xEwb6XzkYp4V53X7SvZzufhX9cxDhfaSeE-mSE7fyz9f432F25UIruxBp1yLIwdv0LuorUXWJvcbP4525H-iRNDvHpdN50iAZPFf_oDyOxWCoiJKvj9GAhZV1hhfVXLmj9iTRfE1zPkqNPS8d6h2RE4jA&ref=orjY4mGPRjk5boDnW0uvlrrd71vZw9kp5uQozpMtKCUqbGEGmzdoDsD46DxfHphq6zzdeQjNs9kBNWuIqJmW4ppn7KEGuoEe&l10n=ru&cts=1473157293909&mc=2.75";
        uns = UnsignUrl(jsred2, SignKeys.Get());
        uns.Dump();
        UNIT_ASSERT_EQUAL(uns.Verified, 1);

        TString redir = "yandex.ru/redir/GAkkM7lQwz7vv7M_pnW8mWMh1IIXJ2UiCT3Dq6kklq2ypvC7E7B8t6Po_vF05I4boTYF3pQbirtyJ0mtw7kipLkNQZRvieIzvfxHsGyqNm7JAfKyB-aOwl9GoucztmxElXL-ZPU3ki-ZRva7KTBReB28tcI4Cfou95WdhlMMjA9o7pn4wim1_omYBui84nqGSBno3mKlYfpsx7jxLfDGwdDozd5rs453vH5wJq2VDnjRXTISaYWrwz71HIIyrwnP9hTmcdT4aldGvNXCWn4EXafn6FZ3zM5xiPOivbgTLRqJ6fUD_e-TILcLUrjUGp0GvM0fakmO_7SKqiPFPrPQdJZ5LdhP07Vubq2rdcTje8l1awoDQH1kdG3Nxue2uqaNOUUarKlUs67spUYgQRF-h5slTkRf5d1lBTxD1Q-sQ8Pu24V3qnxcCbb82EEzi2cXVArUBBrcd9ZxqCReV8jS8sZsCsl-p_2c5vFxqYbWaSr9g4gZityJL9yWJyEmMJ3ObHYx22WBRwlmhDbAduQXgkyOt2CFd5oasnCnMysXH4_dJ5LHQSbKuFelYKORbBiPXjymZY042zVXKQxynHm9xrjvdQHIToUso0HmAPteXXNOqznOk-iXf8WRdm6AzCe9aANiLwtgbSe3dkwsmYsbXsSaQzKZXPSRwBQF4cAdYoKDwhX_xUE5vwnJ8dsnNwAwc1QiCMO8XA2nl4AuhkU0HRG7yHPR3tueWYBaFZ5eX6iDJfBtIVItrxsJpCXXr8nu8CwZpWemMh_BQmKT6veJ-Ok89ebMj_mD1FqZdznKcW7_qD98xAmRng8h7QMjcxzM3C3jV5murLfFe6xgK9KqErryD8fWJ_1QR6MyL_rbvFSBLDKvDYIFwDtmarFSj48qoGAlIDAviIfSk06Y7lNl106l9Jt1iW629hHGYAYn9tSRqx2TX0M-vWn8VemlI7PUt7mAAO5gce44kn6pBaVPiw,,?data=QVyKqSPyGQwwaFPWqjjgNh4zw5HJ3a8ytUcat4LnM7hYG3dQpLPYHVU8Qx5PzerkO9XcWiVYd-LZOHEKr2k5ERRqgFuxYuPCmwE7Wo-N-_NHrrkP8wC8GyuYXSFxXI3TZ0H4qP0rmy99Tqg1mHKQkG6RDWOHs4uUxrWQU-Q3oeIg0F3FkE5qVAG8U2AZ7EAf1R4pbdijIOp158PcNwK_lxJ0JzshSQKr_cD-xDTDI-JvZV4iwXnBfijEjZ1-Qu3Hmo2nvibpUkbUcsx35KDcibrXAy0vei8fDVAAW9KwhYoV3vNh4UwZoLTeYxCsLh-PD0N4wOd3qqNeHj1-2mdzU5F67B-Z2FRodr-oLSyRC8zLqtWwMRNGWl5Vf6NWvdEbXjECaHKfeWcwc3K0Jz7jOeTKq_ZUdFXOhgzAktv82T6xDn8BCmhoczRw3g3wI9kco982FdqR0tboYoktRxMv3BRagHtQC0x0pe2I3hgOedr0pa7tI2OAif5TVDaVh_z3kCYblAxMxXeG4XUaVoA8ryAfHQq1rUPU3k_Wm-EzyZyDaPYqbMds9X6xOpUzqN7ETZyYRkMnIzJbCnIZlc0gezQniLweWtvvIu5AslMHuXTJ5D7g7q3Jckynf7Ro-XrmO1ljlWTLlsUHG4_yT3vWZg5lKjFF4RaxczCYHQ2eUxFsb6J22TIm7YGtUoLWR7R-i-gEiiNnV5N17V5r17LcEqd6DILVbn0C-Fw9V3cgr4XLQNjbNXdOZso9OchkSTniDo0P6n-AudjESFAhd7Px6DIAweOP-TZonMl4BV_qFJb7jULCO9wFOsoQ0npE_6c32NUl1xIf7f99wZ-10jM8BG7s1woSFC-QMIcJFQ5nc0fI55U_YgQuNmjsxHfp-bDY&b64e=1&sign=130a910fcf04d5cffecd5339ac8ca43b&keyno=1&track=srchlink";
        uns = UnsignUrl(redir, SignKeys.Get());
        uns.Dump();
        UNIT_ASSERT_EQUAL(uns.Verified, 1);

        //LOGSTAT-6672 add /ref= with crypted http_referer to /safeclick
        TString crypted_ref = Enpercent(Crypt("http://yandex.com.tr", zero_key));
        //
        TString safeclick = "//yandex.ru/clck/safeclick/data=AiuY0DBWFJ5fN_r-AEszkwR-Yb62DjKSukXEdl-SoUGifwfgmYymRAS-l72Uf_rjzSEDIHNP-WIOpRYDENM8tIxQCgi9a6CdnUQVpfnaBlfhnBu7WUB1vE6Ja8w2gaTX16WimZ4NK9sD-69kg4O0w7FQZoI6U-eTzAyt4BmXs56L916IGcdkfiJybSl3ewNDd5Ih5sH4kpbdjrnIFUt7RFfZV7Fd4mJr1n3_CFD6nkBp_fhCxVV4AQNNZKvEGimf984f7YvNu_-MZ3LSnGhZqY1mY4lKgSw53KuX9jnSzc2BoFWyHyVUvsdbVvw0b0-8ikfTnbtdE39g8BiZltvlAuZdS1Xl72rRzd-qTdkI8MiQuu6ml24_Q9Yltz92CDqnK-bnApZdKOVeEv_W1PGmM5O8_X9VWVSDxm03JftGhte4yv7Bsy7mPjGfuI-gRhEMBv678LefWBmvlDVrBSignHv1T-6vzTJrk10j4GweL-zQE-RuvTmkk2wfyB_uJ0OB_ZG7RFKaTOjAcRm5Xnt-FxTMdDmc1Q3Sz7sTz6HzjZYaog2QeFIY1Rg0_RkhG4oe6AYtbbQSY564tLSC_dnop4ZXa1b91BKRfHbfo2LTPzCzeCNadhC-I0_BKRsaE-H2XITRZr1vDcH6brQT5ajJuvf3Y8ZlO0g5bpLJ1SJMbjYLWCCn2aJdStVYA7rNjqzZNeAl0IyhGi-u3c1_1CIZrxbDjg-Qdk7-MvPV4-yvsJRWYjn03k4DACdl6bs2f98biixiydlT0bObE4LQi9NOow/sign=26f951e15b45d627f11a41558d44e814/keyno=0/ref=" + crypted_ref + "&rp=1";
        uns = UnsignUrl(safeclick, SignKeys.Get());
        uns.Dump();
        UNIT_ASSERT_EQUAL(uns.Verified, 1);
        UNIT_ASSERT_EQUAL(uns.OtherParams.Ref, "http://yandex.com.tr");
        UNIT_ASSERT_EQUAL(uns.OtherParams.RefEnc, "cM777e4sMOAXKbraARXX1FXhdLdqh1up");
        UNIT_ASSERT(uns.OtherParams.PreferRef);

        TString safeclickSuggest = "//yandex.ru/clck/safeclick/data=_lAfmgRBPT3cYiVJtCsp2DXv51ffktCe3DxjgLDCbTzRft2lZ7_cn6wylN_3dr2lydyiL-r8cX9jfeIGSejYXi5CX9EgO6dv3hLGTT34eoLSaFuMYwuu64d7DCE8iP1L9JB5RkPPQUTiACIfPin-JgjM6QCurb6U-qxXqkJXS0DIYpnDrQ1lf_Xnimj6mUzALbOTi89HqeVHL_45mIx2y_fqV0OKBrv9ROuIxSzPc3w,/sign=8d85ae88f42d122076ffa65fb14d76ec/keyno=16/*/from=yandex.ru%3Bsuggest%3Bweb/etext=1678.fBri8Lf7vI0BPGlAsFX-xrMDiN7i4Qw7vPi5jk1bFIsxgT3OPS4t0_5MEeYBJ3-4.529c31963039ff2b559b0e0967e261a0c431f322";
        uns = UnsignUrl(safeclickSuggest, SignKeys.Get());
        uns.Dump();
        UNIT_ASSERT_EQUAL(uns.Verified, 1);
        UNIT_ASSERT_EQUAL(uns.Prefix, "dtype=stred/pid=1/cid=71613/part=%D0%B2%D0%BA%D0%BE%D0%BD/suggestion=%D0%B2%D0%BA%D0%BE%D0%BD%D1%82%D0%B0%D0%BA%D1%82%D0%B5/region=1/url=https%3A//vk.com/ts=1517334587/uid=/*");


        TString jsredir_bad = "http://yandex.ru/clck/jsredir?from=ERRyandex.ru%3Bsearch%2F%3Bweb%3B%3B&text=&etext=1169.jLQy_5j7K7fQRnz3eKjgX3fW1amq2PBzfzOXPYlIQxw.7f295b168ee07dfa17c3f0a2650c51e81924a86a&uuid=&state=PEtFfuTeVD5kpHnK9lio9bb4iM1VPfe4W5x0C0-qwflIRTTifi6VAA&data=UlNrNmk5WktYejR0eWJFYk1LdmtxaTBPYWVmV3pfYThKMUI5aVlYYmctalBHVW5XYWpxLXlwOUxNWEZRYXE5RUF2UkloYWhOamlGMC1HUGNnNnJnRktKZFUwNE0tZFc0&b64e=2&sign=55406497522e5489d572e460b61b460d&keyno=8&cst=AiuY0DBWFJ5fN_r-AEszkwR-Yb62DjKSukXEdl-SoUGifwfgmYymRAS-l72Uf_rjzSEDIHNP-WKpk54cMScDjaYQ5lavxmQGZbAy32JB4voNv5OHHyOnfeBbQ84ww96FYirIOcrnm1cbasygUUPjPPFzIN-VmniecabH9pM1iklhA8XNTUIjCFktoQ-iQYC1G0eyNuihjjGmtTf47VzoLTU3qnHI71rvPwR20Bp_-uHD4Q6iyitUV3LD8q-eDSSQiBbz9bB6JI_IiWExBlwLSoOJf6kn_6tscSAvTb_d_vntMiUyI6YD__FrPdJ3e0b38mLNI5bx_0kPfH99-U1jaV_1odhZdPU-wmnVjNW1miJeJ291jKM6tODSZtX5SH537smbvKBb6m8kGAKIlHKO4JawGVpxw0X0uFTRQ-dlCk0WEGm3iZGCjurobYA5NUuiVjVGh6kMSrrKhKnRMdrvFCh1SVg7SRDWnzjbkp5O9idAYMW-4o06TZONbAk_0uGG90XURFI8tohu8g6vDEYmBkMtC2eNQjVGGgM4tJipP_yFNtFaPZT3m9Qa1hUJhvigvLXtACXnIZzRYggI4uuGijamwarQ9V2h5jszDAL9Y_m4b8r7AoXs_3lR1U1grzMKXDmaCbPg14gDvYkumT2XaXCcj8juaJWOrpwcGOEKvYDt4iisyedUkniIdhaguxnQEotf93Pjj_XxL1XVeUcaE0hGnXovntSxRWecZyfehAzFX5Y4rOmxcS2Bom6pWuLc1xB693odHoFoFMBLb2WeZDMpggUjjh74jsPSg5FIiz7kAsneBoAft6spWfUATixxvWSgHxNCeeVYqhy88dO1BnGFhunmTt0n&ref=orjY4mGPRjk5boDnW0uvlrrd71vZw9kp5uQozpMtKCUqbGEGmzdoDngrF7K9RCPv&l10n=ru";
        uns = UnsignUrl(jsredir_bad, SignKeys.Get());
        uns.Dump();
        UNIT_ASSERT_EQUAL(uns.Verified, 0);

        TString padded_url = "/jsredir?from=gas.serp.yandex.ru%3Bsearch%2F%3Bweb%3B%3B&text=&etext=1150.EaYgbgu8mkRKWVWgVwl081cyaAizxx_LH1LvpCUL708.8da833c202a17dbf95f1b866ff003415a4502bd7&uuid=&state=ZpOnm2f2Yydx2nY8eUKfHX8Z8FX64Ch3chI4OmL70ZWpqANJ8alY6t7lYdJcWqQ7clXITdSm-2_0ysy5h-wMTK7gFcOq9B2JOvoYCb7yA533SIF2c_8APGDe6PKHCYk8LmkUCwCK0Cw&data=UlNrNmk5WktYejR0eWJFYk1LdmtxczJadmZYWW50TTNVVGJJay0wM2VYNkhHNW5mRnc2NXJzOWhVbkdSa1lrT0dTMUhBSE9mQmlXOXJjYS1NMFp5NEVmTkRqdDl5RWdvckNsM21iOU1tb21ESWUyZjZSOUhlYVVEb3hyTjg5QktiOHplandSdHRucFhlNUhaekstcmhHXzduNGpMdHpuMGs5LTZXLWllMEdTN0lNVnA3eDZSRDB6RTFETHN4U0ZnQl9oeFB4SXR2XzktbERNLXVaamItbHVsdnVVQmpuOW5WQTBmdVNGdUFLeG5RaVdBM094YzVTSEJHY1ZOa0FwNnp0RDVYcVBHbEs3WjFKNE5CVW1FYTFhYkwzd1hHdGlPcDliUFhQVGRuUVpSOUhpOFN4dmpqWTd5TEtZeFVZUDN3R0h2eG5zN1JHUGQ0ZGVEUzdzSWMxOUNMSW14R2t0UFYyd0FVdG1pZVZEWTU1NnJ5cUZfdmFLUE5JVWROTzV3R2NzblZ4dHpZVTJoUHdreG5LZlRUU0JlcU5iM3lyVG9yaGpUMkhiUDBGbWwyeU5pZjdEMXQwbEhhMGUwb3VwQnVONUc0ZERwUFBYRnpGN0RkTkVqa09HbE1YUE1iWVBvVk1sUGdCQ2lZS0d0dXltdlpYdFBJbFBTWnItWEhfc0VQeUNNVWJ3alRhWUFWOWpCUU1sNVN0SHhZTVJsSlVzN0NPQi1xWmJsbUdz&b64e=2&sign=53249927c16ef079b21a6265fa8f80ba&keyno=0&cst=AiuY0DBWFJ5fN_r-AEszkwR-Yb62DjKSukXEdl-SoUGifwfgmYymRAS-l72Uf_rjhjlqVCLCis4ZG_Gnb-uguZLJ33jB5d4183C6s0MLpVsdg_jHx34ev4pUNvSIMDe19ZvA2yhLPG-cjn6WzzgFHTUItXsCg_9Iii490oj0pcO6UsGhZKxyfiSMjoo7ARWq&ref=orjY4mGPRjl998yCZXrGI_HReRfhPfhTVQJf90xTCYEW3typ3KrNJNPKhdUHiIf3c2PwuQBy8fjAR2PGSA2VBeWI1dHG8iCNLIfUtyT6vKI&l10n=ru&cts=1474301762427&mc=3.0850551027564768";
        uns = UnsignUrl(padded_url, SignKeys.Get());
        uns.Dump();

        TString b64e3_url = "/jsredir?from=power.balancer.serp.yandex.ru%3Byandsearch%3Bweb%3B%3B&text=&etext=779.8_dceE8x90La-HiQcUMh_wke6ikdB1ZRavibE7HAass.aad917755ea073126f06f337a42e163faa52f740&url=http%3A%2F%2Fwww.youtube.com%2Fchannel%2FUCqINdqn8js90qj5yOcql5tw&uuid=&state=PEtFfuTeVD5kpHnK9lio9bb4iM1VPfe4W5x0C0%2BqwflIRTTifi6VAA%3D%3D&data=&b64e=3&sign=6d34f8cb881e93e40c4a0231ccc56e38&keyno=8&cst=AiuY0DBWFJ5fN_r-AEszkwR-Yb62DjKSukXEdl-SoUGifwfgmYymRAS-l72Uf_rjzSEDIHNP-WKPEt4tDdth6S8E24w52vH8YreFt1Yo6FQJSb4SACfEA4iryVOgE1dZykzrFTPIxc9qYyiSdaAHSld0gZwa5XNhG3GP8hsRj7sQFhXDei7BzYtA4bwWAcz2IGj5W7DBmh_qvQ4A8drQwBmdPHFMq4AT&ref=orjY4mGPRjmxP858FjHtVpDoLf7TbDkvXqmWiajDZQS-ulC62_eh227YhF7jnIv26gxn6-KdYFFSPMvSB5sQ78KKBvLcV5ydvhaIVM2H5yih-YF7wXgq90dXfmAseZIC5gb7VNzVF9gAG17km9AE1utiNI9EvSH3gN9lGrmYaVwSewzZUwefP6tgOSWeBta1_LsdsDzw5vw&l10n=ru";
        uns = UnsignUrl(b64e3_url, SignKeys.Get());
        uns.Dump();

        TString js_url = "http://yandex.com.tr/clck/jsredir?from=yandex.com.tr%3Bsearch%2F%3Bweb%3B%3B&text=banka kredi&etext=&uuid=&state=51K3y_vgF0gZn2k8mmakuhS284CzcyahLyIuxTmeThI,&&cst=AiuY0DBWFJ5Hyx_fyvalFBs81Vdf_LE0K56WWU8fvE4u_0wYGLYA75i7t4EqHhJE10Up6jG9avjZUrCEcOsm5qkZpnZGrVh8d3ychmMSENg21769wPCtoZF_JFOxC548EUriOAWaeL74yVt2S5vf9smMqPwgHBy-czg5cgM2dukVZNIeGGHmINm-mzIo_4GyXToiLXE69y4vG8flDfKuUgI3fcmzJCXu1Dfg7q-g3pi31SWrhw_8XKwigT7D_wT7mUnbcSyiD86vZIX8Zw3v3DaWGjPOylzC26gWkLRmJOaNoPOjLWPV6RqhKsnIYWjum6M4MZeuxWAYek9r26sjy5JTGG67ugoyQqweeFDOWtdPxYqTZ9ulnfwKhXrRK0mI&data=UlNrNmk5WktYejVnRjZxOG04ZWdBeVg2bUxHX3RJT1NwTFFuWG1URGxSNUJhRlN0RVh3UEc3dVFkWXZjMGYxc2ZTb2tuVzBqTlptWmx2c1llWTRHSndfTWc0MEJpcklVLUlaaXhJeVlXQ3ZOaHR2eWJOd0x0X3B4WnVMT2JaRllHWGI4YUR6czFoQkRxekdROFRlY09sajM3Qm5kbHZsN2NQZjhYVjdOOExzRmZGMlFtWnYyeWpmWi1JbHpsZWwtWnR4X29ZcVY4QVBjanRnUVpnR1RiUW1mNzlOR2ZqcGNCYnMzMEdZWnBQd1BDTTl3WjJ1S1g2MkF1OVpQMGsyWHAyc0N3TmtpSnllRXRNb3lxWF9tendMNHVGaXBvSW9YWDllTEJDR3BoZnlsekJueE5pRXNDdXFSMjhIeERWSllqX0cxMFVoUURySjV3SEFtTXN4bmZwR1BjSDNXbGt6bF8wMEdGSFMwS0d6STdsNGp3RzQ0WHVjdFUwNE15Q1p5ajNHbWlkUzlFdWl0LWNyVnhJWG9nZXpOc1ZRYmp1OGFZV2xvcTF6b2NxMll2aHBRTTI4c0lVZ3ZrNlFpT0xrdXdaRVdIYzhrd3V4UXZLeVhIZ3dUSEJVb0hQRmJkX1E1cThZbEF4SWVPM1VIVlRmNW9JUWxHM1plOG5jLVhwZ1ZxRGx5Y3lqYlVoT3lieFI4cGVjX1ZOUktneHhrSER2WG1lTEpKVWVQdXVMY09LUFh0YUw2MF81RXp2azcxU0I3RWFEOW9oSlp2Rncs&sign=b0ac6700da7531638d01843f8e7832b6&keyno=8&b64e=2&ref=orjY4mGPRjkh5N2Mxdt9IqMz5sJwv1Xfj5GS9j5zDjinR-7rPOWAeHeogCtS0wLZwnPIirmnHRfJW0UZkvRfO3sqJ7qodhQ78P8ACxCM0tT0texk1pXCfIledI7EKoWmoVA_vvVpcFPsuEocVyHd_sTcMJuwTyStoq4XcaHq63E,&l10n=tr";
        uns = UnsignUrl(js_url, SignKeys.Get());
        uns.Dump();

        TString url_test = "/redir/dtype=stred/sid=1477564200.32736.20947.6086/pid=198/cid=198/path=tech.portal-ads.teaser/vars=-reqid=1477564200346-636761-vsearch33-24-ATOMS-distr_portal,-showid=020117301477564200346636761332,-device=desktop,-bannerid=1793710917_firefox_ctrl,-score=21,-lr=235,-product=browser,-eventtype=show/*data=url%3Dhttps%253A%252F%252Fyabs.yandex.ru%252Fresource%252FB4_LwOqnl6akXYQRO4jj2R_banana_20141031_1.png%26ts%3D1477564200%26uid%3D3736124491224829108&sign=621f6c4b8a7139098f31119338bf0b36&keyno=12";
        uns = UnsignUrl(url_test, SignKeys.Get());
        uns.Dump();

        TString wrong_url = "/jsredir?text=%D1%85%D0%BA%20%D1%86%D1%81%D0%BA%D0%B0&from=yandex.ru%3Bsuggest%3Bweb&state=dtype%3Dstred%2Fpid%3D1%2Fcid%3D71613%2Fpart%3D%25D1%2585%25D0%25BA%2520%25D1%2586%25D1%2581%2Fsuggestion%3D%25D1%2585%25D0%25BA%2520%25D1%2586%25D1%2581%25D0%25BA%25D0%25B0%2Fregion%3D11146%2F%2A&data=url%3Dhttp%253A%252F%252Fcska-hockey.ru%26ts%3D1477564243%26uid%3D1938326971460401019&sign=9042ac48799fc6af94dfb322ff75c93d&ref=https%3A%2F%2Fyandex.ru%2F&keyno=0&l10n=ru";
        uns = UnsignUrl(wrong_url, SignKeys.Get());
        uns.Dump();

        TString suggest_url = "/jsredir?text=%D0%BD%D0%B0%D1%82%D0%B0%D0%BB%D0%B8%20%D1%82%D1%83%D1%80%D1%81&from=yandex.ru%3Bsuggest%3Bweb&state=dtype%3Dstred%2Fpid%3D1%2Fcid%3D71613%2Fpart%3Dyfnfkb%2520nehc%2Fsuggestion%3D%25D0%25BD%25D0%25B0%25D1%2582%25D0%25B0%25D0%25BB%25D0%25B8%2520%25D1%2582%25D1%2583%25D1%2580%25D1%2581%2Fregion%3D11029%2F%2A&data=url%3Dhttp%253A%252F%252Fwww.natalie-tours.ru%26ts%3D1477648033%26uid%3D1712661391475769748&sign=fc2220ae02bf8c7f10728d5ea666c262&ref=https%3A%2F%2Fwww.yandex.ru%2Fchrome%2Fnewtab&keyno=0&l10n=ru";
        uns = UnsignUrl(suggest_url, SignKeys.Get());
        uns.Dump();

    }

    Y_UNIT_TEST(SignUrlTest) {
        THolder<NSignUrl::TSignKeys> SignKeys;
        try {
            SignKeys.Reset(new NSignUrl::TSignKeys(ArcadiaSourceRoot() + "/" + KEYS_PATH));
        }
        catch (const yexception e) {
            Cerr << "Err while reading keys!\n";
            Cerr << e.what();
        }

        NSignUrl::TKey zero_key = SignKeys->GetKeyData("0");

        TOtherParams params;
        params.Text = "qq";
        params.From = "oil.serp.yandex.ru;yandsearch;web;;";
        params.Uid = "";
        params.Etext = "1035.HjNDM39--zRGmtpO0qEKzh-CkWg9EXVvCpH44XgQifQ.319d7a779a043972963e6c8df8717e53614493bf";
        params.Ref  = "https://oil.serp.yandex.ru/yandsearch?text=qq&redircnt=1454584342.1'";
        params.PrefixConst = "dtype=iweb/reg=213/u=5522875711426069279/ruip=2a02:6b8:b010:901a::133/ids=/slots=/ver=-1/reqid=1463493500662007-1383889219159218109315094-oil/";

        TString SignedUrl1 = NSignUrl::SignUrl("path=65.176/vars=71=72/*", "//go.mail.ru/search?mailru=1&q=qq", zero_key, "0", params, 1463493500, "5522875711426069279", FULL);

        UNIT_ASSERT_EQUAL(SignedUrl1, TString("from=oil.serp.yandex.ru%3Byandsearch%3Bweb%3B%3B&text=&etext=1035.HjNDM39--zRGmtpO0qEKzh-CkWg9EXVvCpH44XgQifQ.319d7a779a043972963e6c8df8717e53614493bf&uuid=&state=zRrSeA3aY6jDJai6l6dG4NempTAnFQ6F&&cst=AiuY0DBWFJ5fN_r-AEszkwR-Yb62DjKSukXEdl-SoUGifwfgmYymRAS-l72Uf_rjhjlqVCLCis4ZG_Gnb-uguV2Vc61SheevvdaAl7iNkeLZ33-v_YhJMvrU-0WLujA8GL79rFN4_MnhW1vrONOAaVlfTz3s1UsXDADGnVYUb0RfnA1YXMVzCfOnXJ-3o6Kt&data=QzBxTFJjdEFVLUQ0M3E4NFBWZkM3ZjQxUGtNUWxXSUZMa3Y1TXg2YnBNOUl4OGJQMVdLSGlacUo0VWY5QjhyVWduT3ZJM3JFcXQ1TEtUaUJ6dTZlaDVzNFVJZE1zRkw4S1lFLW0xNnp0QnNneDNUSnFveUNZSENUYU1sU2hVcWE,&sign=4f758837cde9414fd43637c1167ec686&keyno=0&b64e=2&ref=orjY4mGPRjnZ10LBDxUBdJj68BX0ryBHrzzr9znRGAX5ISoE0Q8unqD0DAziKWWi1zprXjTRNtilCAeTE2dw75j3T-4dV6R5"));
        TString SignedUrl2 = NSignUrl::SignUrl("path=65.176/vars=71=72/events=%5B%7B%22event%22%3A%22click%22%2C%22id%22%3A%22uniq15000254166945947%22%2C%22cts%22%3A1500550783105%2C%22/*", "//go.mail.ru/search?mailru=1&q=qq", zero_key, "0", params, 1463493500, "5522875711426069279", UNCIPHERED);

Cerr << "\n\n\n**********************\n\n\n";
Cerr << SignedUrl2;
Cerr << "\n\n\n**********************\n\n\n";

        params.Text = "cats";
        params.From = "yandex.ru;search/;web;;";
        params.Uid = "";
        params.Etext = "1207.LbRrC_tkoSdkCtMV_AGys5UobwIdLKckCgXgqDPmUpE.c13ea6ab6fde7ef3be713d6cb10c2039c9e11f58";
        params.Ref  = "https://yandex.ru/search/?text=cats&lr=2&srcrwr=TEMPLATES%3Aflacksand2.haze.yandex.net%3A9780%3A5000&exp_flags=json_template_external%3Dby_name";
        params.PrefixConst = "dtype=iweb/reg=2/u=143129921414761058/ruip=2a02:6b8:0:2309:a40b:dcf5:7109:efe5/ids=32324,32049,32663,31493,28158,32348,32444,25466/slots=32324,0,51;32049,0,53;32663,0,37;31493,0,98;28158,0,24;32348,0,42;32444,0,56;25466,0,14;32355,0,43/ver=5294/reqid=1476294500683372-668565134401623660672614-man1-3086/";

        TString TEST_URL = "http://www.google.ru/search?ie=UTF-8&hl=ru&q=cats";

        TString SignedUrl = NSignUrl::SignUrl("path=65.66/vars=71=72/*", TEST_URL, zero_key, "0", params, 1476288252, "143129921414761058", FULL);

        TUnsignedUrl UnsignedUrl = UnsignUrl("http://yandex.ru/clck/jsredir?" + SignedUrl, SignKeys.Get());

        UNIT_ASSERT_EQUAL(UnsignedUrl.Url, TEST_URL);

        params.Text = "cats & мышки";
        params.From = "yandex.ru;search/;web;;";
        params.Etext = "";
        params.Uid = "";
        params.Ref  = "https://yandex.ru/search/?text=cats&lr=2&srcrwr=TEMPLATES%3Aflacksand2.haze.yandex.net%3A9780%3A5000&exp_flags=json_template_external%3Dby_name";
        params.PrefixConst = "dtype=iweb/reg=2/u=143129921414761058/ruip=2a02:6b8:0:2309:a40b:dcf5:7109:efe5/ids=32324,32049,32663,31493,28158,32348,32444,25466/slots=32324,0,51;32049,0,53;32663,0,37;31493,0,98;28158,0,24;32348,0,42;32444,0,56;25466,0,14;32355,0,43/ver=5294/reqid=1476294500683372-668565134401623660672614-man1-3086/";

        TString TEST_URL2 = "abyrvalg";

        SignedUrl = NSignUrl::SignUrl("path=65.66/vars=71=72/*", TEST_URL2, zero_key, "0", params, 1476288252, "143129921414761058", FULL);

        TUnsignedUrl UnsignedUrl2 = UnsignUrl("http://yandex.ru/clck/jsredir?" + SignedUrl, SignKeys.Get());
        UnsignedUrl2.Dump();

        UNIT_ASSERT_EQUAL(UnsignedUrl2.Url, TEST_URL2);

        params.Text = "c%2B%2B";
        params.From = "yandex.ru;search/;web;;";
        params.Uid = "";
        params.Etext = "etext";
        params.Ref = "https://yandex.ru/search/?text=c%2B%2B";
        params.PrefixConst = "dtype=iweb/reg=2/u=143129921414761058/ruip=2a02:6b8:0:2309:a40b:dcf5:7109:efe5/ids=32324,32049,32663,31493,28158,32348,32444,25466/slots=32324,0,51;32049,0,53;32663,0,37;31493,0,98;28158,0,24;32348,0,42;32444,0,56;25466,0,14;32355,0,43/ver=5294/reqid=1476294500683372-668565134401623660672614-man1-3086/";

        TString TEST_URL_G = "http://www.google.ru/search?ie=UTF-8&hl=ru&q=c%2B%2B";
        SignedUrl = NSignUrl::SignUrl("path=65.66/vars=71=72/*", TEST_URL_G, zero_key, "0", params, 1476288252, "143129921414761058", FULL);
        TUnsignedUrl UnsignedUrl_G = UnsignUrl("http://yandex.ru/clck/jsredir?" + SignedUrl, SignKeys.Get());

        UnsignedUrl_G.Dump();
        UNIT_ASSERT_EQUAL(UnsignedUrl_G.Url, TEST_URL_G);
        UNIT_ASSERT_EQUAL(UnsignedUrl_G.Verified, 1);

        TUnsignedUrl UnsignedUrl_ref = UnsignUrl("/jsredir?from=yandex.ua%3Byandsearch%3Bweb%3B%3B&text=&etext=1294.OAbwg8leBeJR698inxpQ5-WCG4LcgYno_7RyuuUQidI.dc27de92c6dde6fd68f12a597acd521ae8d520d1&uuid=&state=PEtFfuTeVD5kpHnK9lio9dFa2ePbDzX7fH_cbK-eu2V8J4cbFpzDXVHZJKQvpytSIkjSZ4Cc6zu0NMQINCoRdw&data=UlNrNmk5WktYejR0eWJFYk1Ldmtxa2NPaEd4cDY2WktUcHdhNlgzZ0RKMGFnVWhZTmc1YnE0RkktQUh5VGFZbmQ0WVI3Z1VwRXE5SGR0Sm9GcnRwLUthdzRoeERveG4xNEMtM3REY1VnNWktTmJtVmoyd1Q2NDdJaUU1VTBoQ3hxOFNmTVlCaVJJTQ&b64e=2&sign=17e04efc8dfa47a14586f17e96c3db1a&keyno=0&cst=AiuY0DBWFJ5Hyx_fyvalFBH55pabWwasuqj-zjPVos0ZadjQrkXqm2KccmsrZsiI5yM2wQ_UIg75dlEoWar2TQEjb5-Z_jk4ES9nY7iVPE-m75kdcIM82uUjYgyyd7BcA4AOrZ2cBx9T4LUstSakpOw2q1dyI7iAyyCob43b-F7xp_4ZwaFsZcVnaxZKdGPSXMpRTT5GGtvvL7te_aeg6ZQfZKLwQ2G4MGBvwDlcPxTdCAsG5KpQXrKzz6Ng_EmlsJWu9um1L-I4G3k60MtsMwQfjl5a_Uje5v-HvDsbuh1Yh-ORrI_s5hk28rpRFNf8dkH8GY7jDE51kbjfo-3p35J-h-Wu-UngRkuK-xTKLKViz6W13kAZbzsfefq4mbSmzgKsJyKWZQ8Zn_hRmamhSiLz8FqXBHZcyiScZh3s0Almlrj2DG6kWVQmIkQsu5n8sH5oM-ea3W9458btqisSFFjKgReK8DSMz4XlsJLZ-v-Q4TvhGY8tY_Xp_fejsB5FxQ-X72YBY_A&ref=\");window.stop();alert(document.cookie%2b", SignKeys.Get());

        UNIT_ASSERT_EQUAL(UnsignedUrl_ref.OtherParams.RefEnc, "");
        UNIT_ASSERT_EQUAL(UnsignedUrl_ref.OtherParams.Ref, "_WRONG_REF_NOT_HTTP_PREFIXED_");
        UNIT_ASSERT(!UnsignedUrl_ref.OtherParams.PreferRef);
    }
}
