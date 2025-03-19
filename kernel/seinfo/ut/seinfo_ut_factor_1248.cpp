#include "seinfo_ut.h"

#include <util/system/tempfile.h>

using namespace NSe;

void TestSeFunc83() {
    {
        TTempFileHandle keysFile("keys.xml");
        constexpr TStringBuf keysXml =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<keys>\n"
    "<key id=\"1306\" timestamp=\"1484625674\" sig=\"00F807F1988DD7B10602501355B43014\">A39C101FCB6FC1AD9B13298116A624AB</key>\n"
    "<key id=\"1307\" timestamp=\"1484712161\" sig=\"3CCB9D98F4692BF24F2841F7D1D0217E\">D1A9C037280AAEF54A2AFE3804413E10</key>\n"
"</keys>"sv;

        keysFile.Write(keysXml.data(), keysXml.size());
        keysFile.FlushData();

        const TEtextKeys keys(keysFile.Name());

        TInfo info(SE_YANDEX, ST_WEB, "счастье по-французски", SF_SEARCH);
        // correct urls
        KS_TEST_URL_WITH_KEYS("http://yandex.ru/clck/jsredir?from=yandex.ru%3Bsearch%2F%3Bweb%3B%3B&text=\
&etext=1306.wHuW4DzWULLM-KNIr-0030R6ECCFFcn-_-AlI1PZiJmZhtwKxONNbJ__2y_2vVeSGjU5yo4SdHIh9D_glUyq9g.c2798f8ba1511ad6db7b06bff6216a4cf168ded0\
&uuid=&state=PEtFfuTeVD5kpHnK9lio9WCnKp0DidhELg_I3jU5bBngJTgpBPU5btT9QV3fgmuCQFiXO1L7TKVChQ7_jSvbGF\
Fgmjai1oVo&data=UlNrNmk5WktYejR0eWJFYk1LdmtxaVNNVGtkNEJYSHloV3pjWHoweUYzdndsS3BCbEdaZXkxUG1ObWhpNVd\
UMEJkRFo4aTFFYTJINUY1ZU1Oc1VLYTBOX0hzMFVEYkpSbHdFdWFwQkN2dTEyYTFwMkFkLU44MjcyRGpHWWV1SDk&b64e=2&sig\
n=ffa66e406c1493cc95a207cab71f536b&keyno=0&cst=AiuY0DBWFJ5fN_r-AEszk7ZsMCg4ImCqTxB_GKFqHJWADXa3G6p1\
_cUfXZ52z3WfomCXdAam8HfLXss4cADIUA9hai5LRBGd0JVBNRPZiBqJJzeL9kDZxNxZcAZCsn4-7X6ctOSwo9ac-vrZOjxIr5F\
x8EcWizeti0sodUtZlBHmyOPVH4RYGMxeakzAKd7Zbu751gMo_kiru5O_dsAEvAKoPKD3I_N7np0uoi1sjnjoAPNsm7iKQ0dOE5\
ERZ-lZXdNW-nq7N0SMx7xP0fojAJUtuEVW08ou8y6YMZwSwcAiHGNN4nZ1GKSS0gABgLgdEI1Qt8SQW__oL-b8bVfN991WZHdBa\
gdnrnLTZM2dyyZLZax5MaJTnxwoJ1lDRI8LXzW8YfqKmgGslKvo7GZHyU_YoDeldz4M88u-pQc76ruXieWa1H7xBDKxy7XtfYzh\
&ref=orjY4mGPRjk5boDnW0uvlrrd71vZw9kp5uQozpMtKCW4RgmAu5ryJJrPwhrSNxFgPK4ENCeQ6JnGrGFK1PtfbC7PLk-eVw\
8qExoRC7KqwFVqwHnolSKC9mkMXec2Pkyd83qSJk5XyWkCKs8tED2oRp5bZ78UL5UIcDBPP_L2RCG-V7WY2rLcS3m5CqlXpcMuX\
5HWmav9BMidofK8JzG5NmEqgVNVVCn4QjdaCW5CM_1dYUuBQ3i4pw5dZDPxEGTvjrQULvDPFyHBxBhsKAsz6jIM4SBtO7Xz&l10\
n=ru&cts=1484839060175&mc=1", info, keys);

        info.Query = "vk";
        KS_TEST_URL_WITH_KEYS("http://yandex.ru/clck/jsredir?from=yandex.ru%3Bsearch%2F%3Bweb%3B%3B&text=\
&etext=1307.QsrR04OMVcvgFTfbBKX2AKaz1x_ArBbvFNglnYY2Wu0.6b04e02d28f453780389abb2cfb70d349a17d417\
&uuid=&state=PEtFfuTeVD5kpHnK9lio9WCnKp0DidhE9rs5TGtBySwiRXKUtOaYc_CcYwClH-bY7Fd6cgFsfxVrbJIyvKrrbg\
&data=UlNrNmk5WktYejY4cHFySjRXSWhXRkNYS1ZJTW9paUxXcG1iNmR6N2JoZk40Mmg5MTJwSElHOXNid3B4b0p2TmdWOExRe\
jJhdmQ3aU9wOFdna2Q4OEE&b64e=2&sign=bf2069394518dc3fa939752785816771&keyno=0&cst=AiuY0DBWFJ5fN_r-AEs\
zk7ZsMCg4ImCqTxB_GKFqHJWADXa3G6p1_cUfXZ52z3WfomCXdAam8HdN9IsUllLo1vrVJWt_cNC2kq6BlAknbHvi4XLCdDmd7c\
o1jx2HSUT_ZZ_ymwF01FsTK4Z_Mb1ly7kk55SL-U_GDFZt62mZ_pAG7uCMszxmeoCppE4UOpATxW_Vlfr9-mcIOLc-3KbV86Hzt\
lKEf9DsvhtFpAVrXwGxWEIesBN_Nwug7MaHFY4CnKLAAteahWSjjnzSgzLYoZUjstMquEB54mBNKeaukvUFAbV6C8WfQXrbch1s\
9Wurnf8MztS7IQ1i1NQQOytMvSfpJa4elgdKTYrJVmb-ZF_tggGG50E1eIh0baf8OQxzE9j3bgiwSTGEQU28NRmPp1SxINS6Se3\
5rIlXOwkpW6NtrV5r57fdvS481nyf3OzlRDIhtISnmys&ref=orjY4mGPRjk5boDnW0uvlrrd71vZw9kptjyJN8vfhd2nemAbVs\
wsnTCcPIMJxEzLP_QL2UjGhDZDq0yLnwbFmQ&l10n=ru&cts=1484916812647&mc=2.7321588913645707", info, keys);

        // malformed etext
        info.Query = "";
        KS_TEST_URL_WITH_KEYS("http://yandex.ru/clck/jsredir?from=yandex.ru%3Bsearch%2F%3Bweb%3B%3B&text=\
&etext=qwertyuiopasdfghjklzxcvbnmQWERTYUIOPASDFGHJKLZXCVBNM1234567890\
&uuid=&state=PEtFfuTeVD5kpHnK9lio9WCnKp0DidhE9rs5TGtBySwiRXKUtOaYc_CcYwClH-bY7Fd6cgFsfxVrbJIyvKrrbg\
&data=UlNrNmk5WktYejY4cHFySjRXSWhXRkNYS1ZJTW9paUxXcG1iNmR6N2JoZk40Mmg5MTJwSElHOXNid3B4b0p2TmdWOExRe\
jJhdmQ3aU9wOFdna2Q4OEE&b64e=2&sign=bf2069394518dc3fa939752785816771&keyno=0&cst=AiuY0DBWFJ5fN_r-AEs\
zk7ZsMCg4ImCqTxB_GKFqHJWADXa3G6p1_cUfXZ52z3WfomCXdAam8HdN9IsUllLo1vrVJWt_cNC2kq6BlAknbHvi4XLCdDmd7c\
o1jx2HSUT_ZZ_ymwF01FsTK4Z_Mb1ly7kk55SL-U_GDFZt62mZ_pAG7uCMszxmeoCppE4UOpATxW_Vlfr9-mcIOLc-3KbV86Hzt\
lKEf9DsvhtFpAVrXwGxWEIesBN_Nwug7MaHFY4CnKLAAteahWSjjnzSgzLYoZUjstMquEB54mBNKeaukvUFAbV6C8WfQXrbch1s\
9Wurnf8MztS7IQ1i1NQQOytMvSfpJa4elgdKTYrJVmb-ZF_tggGG50E1eIh0baf8OQxzE9j3bgiwSTGEQU28NRmPp1SxINS6Se3\
5rIlXOwkpW6NtrV5r57fdvS481nyf3OzlRDIhtISnmys&ref=orjY4mGPRjk5boDnW0uvlrrd71vZw9kptjyJN8vfhd2nemAbVs\
wsnTCcPIMJxEzLP_QL2UjGhDZDq0yLnwbFmQ&l10n=ru&cts=1484916812647&mc=2.7321588913645707", info, keys);

        // wrong signature
        KS_TEST_URL_WITH_KEYS("http://yandex.ru/clck/jsredir?from=yandex.ru%3Bsearch%2F%3Bweb%3B%3B&text=\
&etext=1307.QsrR04OMVcvgFTfbBKX2AKaz1x_ArBbvFNglnYY2Wu0.0000000000000000000000000000000000000000\
&uuid=&state=PEtFfuTeVD5kpHnK9lio9WCnKp0DidhE9rs5TGtBySwiRXKUtOaYc_CcYwClH-bY7Fd6cgFsfxVrbJIyvKrrbg\
&data=UlNrNmk5WktYejY4cHFySjRXSWhXRkNYS1ZJTW9paUxXcG1iNmR6N2JoZk40Mmg5MTJwSElHOXNid3B4b0p2TmdWOExRe\
jJhdmQ3aU9wOFdna2Q4OEE&b64e=2&sign=bf2069394518dc3fa939752785816771&keyno=0&cst=AiuY0DBWFJ5fN_r-AEs\
zk7ZsMCg4ImCqTxB_GKFqHJWADXa3G6p1_cUfXZ52z3WfomCXdAam8HdN9IsUllLo1vrVJWt_cNC2kq6BlAknbHvi4XLCdDmd7c\
o1jx2HSUT_ZZ_ymwF01FsTK4Z_Mb1ly7kk55SL-U_GDFZt62mZ_pAG7uCMszxmeoCppE4UOpATxW_Vlfr9-mcIOLc-3KbV86Hzt\
lKEf9DsvhtFpAVrXwGxWEIesBN_Nwug7MaHFY4CnKLAAteahWSjjnzSgzLYoZUjstMquEB54mBNKeaukvUFAbV6C8WfQXrbch1s\
9Wurnf8MztS7IQ1i1NQQOytMvSfpJa4elgdKTYrJVmb-ZF_tggGG50E1eIh0baf8OQxzE9j3bgiwSTGEQU28NRmPp1SxINS6Se3\
5rIlXOwkpW6NtrV5r57fdvS481nyf3OzlRDIhtISnmys&ref=orjY4mGPRjk5boDnW0uvlrrd71vZw9kptjyJN8vfhd2nemAbVs\
wsnTCcPIMJxEzLP_QL2UjGhDZDq0yLnwbFmQ&l10n=ru&cts=1484916812647&mc=2.7321588913645707", info, keys);

        // non-existent key
        KS_TEST_URL_WITH_KEYS("http://yandex.ru/clck/jsredir?from=yandex.ru%3Bsearch%2F%3Bweb%3B%3B&text=\
&etext=0000.QsrR04OMVcvgFTfbBKX2AKaz1x_ArBbvFNglnYY2Wu0.6b04e02d28f453780389abb2cfb70d349a17d417\
&uuid=&state=PEtFfuTeVD5kpHnK9lio9WCnKp0DidhE9rs5TGtBySwiRXKUtOaYc_CcYwClH-bY7Fd6cgFsfxVrbJIyvKrrbg\
&data=UlNrNmk5WktYejY4cHFySjRXSWhXRkNYS1ZJTW9paUxXcG1iNmR6N2JoZk40Mmg5MTJwSElHOXNid3B4b0p2TmdWOExRe\
jJhdmQ3aU9wOFdna2Q4OEE&b64e=2&sign=bf2069394518dc3fa939752785816771&keyno=0&cst=AiuY0DBWFJ5fN_r-AEs\
zk7ZsMCg4ImCqTxB_GKFqHJWADXa3G6p1_cUfXZ52z3WfomCXdAam8HdN9IsUllLo1vrVJWt_cNC2kq6BlAknbHvi4XLCdDmd7c\
o1jx2HSUT_ZZ_ymwF01FsTK4Z_Mb1ly7kk55SL-U_GDFZt62mZ_pAG7uCMszxmeoCppE4UOpATxW_Vlfr9-mcIOLc-3KbV86Hzt\
lKEf9DsvhtFpAVrXwGxWEIesBN_Nwug7MaHFY4CnKLAAteahWSjjnzSgzLYoZUjstMquEB54mBNKeaukvUFAbV6C8WfQXrbch1s\
9Wurnf8MztS7IQ1i1NQQOytMvSfpJa4elgdKTYrJVmb-ZF_tggGG50E1eIh0baf8OQxzE9j3bgiwSTGEQU28NRmPp1SxINS6Se3\
5rIlXOwkpW6NtrV5r57fdvS481nyf3OzlRDIhtISnmys&ref=orjY4mGPRjk5boDnW0uvlrrd71vZw9kptjyJN8vfhd2nemAbVs\
wsnTCcPIMJxEzLP_QL2UjGhDZDq0yLnwbFmQ&l10n=ru&cts=1484916812647&mc=2.7321588913645707", info, keys);

        // malformed key number
        KS_TEST_URL_WITH_KEYS("http://yandex.ru/clck/jsredir?from=yandex.ru%3Bsearch%2F%3Bweb%3B%3B&text=\
&etext=ZZZZ.QsrR04OMVcvgFTfbBKX2AKaz1x_ArBbvFNglnYY2Wu0.6b04e02d28f453780389abb2cfb70d349a17d417\
&uuid=&state=PEtFfuTeVD5kpHnK9lio9WCnKp0DidhE9rs5TGtBySwiRXKUtOaYc_CcYwClH-bY7Fd6cgFsfxVrbJIyvKrrbg\
&data=UlNrNmk5WktYejY4cHFySjRXSWhXRkNYS1ZJTW9paUxXcG1iNmR6N2JoZk40Mmg5MTJwSElHOXNid3B4b0p2TmdWOExRe\
jJhdmQ3aU9wOFdna2Q4OEE&b64e=2&sign=bf2069394518dc3fa939752785816771&keyno=0&cst=AiuY0DBWFJ5fN_r-AEs\
zk7ZsMCg4ImCqTxB_GKFqHJWADXa3G6p1_cUfXZ52z3WfomCXdAam8HdN9IsUllLo1vrVJWt_cNC2kq6BlAknbHvi4XLCdDmd7c\
o1jx2HSUT_ZZ_ymwF01FsTK4Z_Mb1ly7kk55SL-U_GDFZt62mZ_pAG7uCMszxmeoCppE4UOpATxW_Vlfr9-mcIOLc-3KbV86Hzt\
lKEf9DsvhtFpAVrXwGxWEIesBN_Nwug7MaHFY4CnKLAAteahWSjjnzSgzLYoZUjstMquEB54mBNKeaukvUFAbV6C8WfQXrbch1s\
9Wurnf8MztS7IQ1i1NQQOytMvSfpJa4elgdKTYrJVmb-ZF_tggGG50E1eIh0baf8OQxzE9j3bgiwSTGEQU28NRmPp1SxINS6Se3\
5rIlXOwkpW6NtrV5r57fdvS481nyf3OzlRDIhtISnmys&ref=orjY4mGPRjk5boDnW0uvlrrd71vZw9kptjyJN8vfhd2nemAbVs\
wsnTCcPIMJxEzLP_QL2UjGhDZDq0yLnwbFmQ&l10n=ru&cts=1484916812647&mc=2.7321588913645707", info, keys);

        // altered b64 section
        KS_TEST_URL_WITH_KEYS("http://yandex.ru/clck/jsredir?from=yandex.ru%3Bsearch%2F%3Bweb%3B%3B&text=\
&etext=ZZZZ.QsrR04OMVcvgFTfQQQQQQQQQQQ_ArBbvFNglnYY2Wu0.6b04e02d28f453780389abb2cfb70d349a17d417\
&uuid=&state=PEtFfuTeVD5kpHnK9lio9WCnKp0DidhE9rs5TGtBySwiRXKUtOaYc_CcYwClH-bY7Fd6cgFsfxVrbJIyvKrrbg\
&data=UlNrNmk5WktYejY4cHFySjRXSWhXRkNYS1ZJTW9paUxXcG1iNmR6N2JoZk40Mmg5MTJwSElHOXNid3B4b0p2TmdWOExRe\
jJhdmQ3aU9wOFdna2Q4OEE&b64e=2&sign=bf2069394518dc3fa939752785816771&keyno=0&cst=AiuY0DBWFJ5fN_r-AEs\
zk7ZsMCg4ImCqTxB_GKFqHJWADXa3G6p1_cUfXZ52z3WfomCXdAam8HdN9IsUllLo1vrVJWt_cNC2kq6BlAknbHvi4XLCdDmd7c\
o1jx2HSUT_ZZ_ymwF01FsTK4Z_Mb1ly7kk55SL-U_GDFZt62mZ_pAG7uCMszxmeoCppE4UOpATxW_Vlfr9-mcIOLc-3KbV86Hzt\
lKEf9DsvhtFpAVrXwGxWEIesBN_Nwug7MaHFY4CnKLAAteahWSjjnzSgzLYoZUjstMquEB54mBNKeaukvUFAbV6C8WfQXrbch1s\
9Wurnf8MztS7IQ1i1NQQOytMvSfpJa4elgdKTYrJVmb-ZF_tggGG50E1eIh0baf8OQxzE9j3bgiwSTGEQU28NRmPp1SxINS6Se3\
5rIlXOwkpW6NtrV5r57fdvS481nyf3OzlRDIhtISnmys&ref=orjY4mGPRjk5boDnW0uvlrrd71vZw9kptjyJN8vfhd2nemAbVs\
wsnTCcPIMJxEzLP_QL2UjGhDZDq0yLnwbFmQ&l10n=ru&cts=1484916812647&mc=2.7321588913645707", info, keys);

        // empty text and etext
        KS_TEST_URL_WITH_KEYS("http://yandex.ru/clck/jsredir?from=yandex.ru%3Bsearch%2F%3Bweb%3B%3B&text=\
&etext=\
&uuid=&state=PEtFfuTeVD5kpHnK9lio9WCnKp0DidhE9rs5TGtBySwiRXKUtOaYc_CcYwClH-bY7Fd6cgFsfxVrbJIyvKrrbg\
&data=UlNrNmk5WktYejY4cHFySjRXSWhXRkNYS1ZJTW9paUxXcG1iNmR6N2JoZk40Mmg5MTJwSElHOXNid3B4b0p2TmdWOExRe\
jJhdmQ3aU9wOFdna2Q4OEE&b64e=2&sign=bf2069394518dc3fa939752785816771&keyno=0&cst=AiuY0DBWFJ5fN_r-AEs\
zk7ZsMCg4ImCqTxB_GKFqHJWADXa3G6p1_cUfXZ52z3WfomCXdAam8HdN9IsUllLo1vrVJWt_cNC2kq6BlAknbHvi4XLCdDmd7c\
o1jx2HSUT_ZZ_ymwF01FsTK4Z_Mb1ly7kk55SL-U_GDFZt62mZ_pAG7uCMszxmeoCppE4UOpATxW_Vlfr9-mcIOLc-3KbV86Hzt\
lKEf9DsvhtFpAVrXwGxWEIesBN_Nwug7MaHFY4CnKLAAteahWSjjnzSgzLYoZUjstMquEB54mBNKeaukvUFAbV6C8WfQXrbch1s\
9Wurnf8MztS7IQ1i1NQQOytMvSfpJa4elgdKTYrJVmb-ZF_tggGG50E1eIh0baf8OQxzE9j3bgiwSTGEQU28NRmPp1SxINS6Se3\
5rIlXOwkpW6NtrV5r57fdvS481nyf3OzlRDIhtISnmys&ref=orjY4mGPRjk5boDnW0uvlrrd71vZw9kptjyJN8vfhd2nemAbVs\
wsnTCcPIMJxEzLP_QL2UjGhDZDq0yLnwbFmQ&l10n=ru&cts=1484916812647&mc=2.7321588913645707", info, keys);

        // only text
        info.Query = "vk";
        KS_TEST_URL_WITH_KEYS("http://yandex.ru/clck/jsredir?from=yandex.ru%3Bsearch%2F%3Bweb%3B%3B&text=vk\
&etext=\
&uuid=&state=PEtFfuTeVD5kpHnK9lio9WCnKp0DidhE9rs5TGtBySwiRXKUtOaYc_CcYwClH-bY7Fd6cgFsfxVrbJIyvKrrbg\
&data=UlNrNmk5WktYejY4cHFySjRXSWhXRkNYS1ZJTW9paUxXcG1iNmR6N2JoZk40Mmg5MTJwSElHOXNid3B4b0p2TmdWOExRe\
jJhdmQ3aU9wOFdna2Q4OEE&b64e=2&sign=bf2069394518dc3fa939752785816771&keyno=0&cst=AiuY0DBWFJ5fN_r-AEs\
zk7ZsMCg4ImCqTxB_GKFqHJWADXa3G6p1_cUfXZ52z3WfomCXdAam8HdN9IsUllLo1vrVJWt_cNC2kq6BlAknbHvi4XLCdDmd7c\
o1jx2HSUT_ZZ_ymwF01FsTK4Z_Mb1ly7kk55SL-U_GDFZt62mZ_pAG7uCMszxmeoCppE4UOpATxW_Vlfr9-mcIOLc-3KbV86Hzt\
lKEf9DsvhtFpAVrXwGxWEIesBN_Nwug7MaHFY4CnKLAAteahWSjjnzSgzLYoZUjstMquEB54mBNKeaukvUFAbV6C8WfQXrbch1s\
9Wurnf8MztS7IQ1i1NQQOytMvSfpJa4elgdKTYrJVmb-ZF_tggGG50E1eIh0baf8OQxzE9j3bgiwSTGEQU28NRmPp1SxINS6Se3\
5rIlXOwkpW6NtrV5r57fdvS481nyf3OzlRDIhtISnmys&ref=orjY4mGPRjk5boDnW0uvlrrd71vZw9kptjyJN8vfhd2nemAbVs\
wsnTCcPIMJxEzLP_QL2UjGhDZDq0yLnwbFmQ&l10n=ru&cts=1484916812647&mc=2.7321588913645707", info, keys);

        // both text and etext
        KS_TEST_URL_WITH_KEYS("http://yandex.ru/clck/jsredir?from=yandex.ru%3Bsearch%2F%3Bweb%3B%3B&text=vk\
&etext=1307.QsrR04OMVcvgFTfbBKX2AKaz1x_ArBbvFNglnYY2Wu0.6b04e02d28f453780389abb2cfb70d349a17d417\
&uuid=&state=PEtFfuTeVD5kpHnK9lio9WCnKp0DidhE9rs5TGtBySwiRXKUtOaYc_CcYwClH-bY7Fd6cgFsfxVrbJIyvKrrbg\
&data=UlNrNmk5WktYejY4cHFySjRXSWhXRkNYS1ZJTW9paUxXcG1iNmR6N2JoZk40Mmg5MTJwSElHOXNid3B4b0p2TmdWOExRe\
jJhdmQ3aU9wOFdna2Q4OEE&b64e=2&sign=bf2069394518dc3fa939752785816771&keyno=0&cst=AiuY0DBWFJ5fN_r-AEs\
zk7ZsMCg4ImCqTxB_GKFqHJWADXa3G6p1_cUfXZ52z3WfomCXdAam8HdN9IsUllLo1vrVJWt_cNC2kq6BlAknbHvi4XLCdDmd7c\
o1jx2HSUT_ZZ_ymwF01FsTK4Z_Mb1ly7kk55SL-U_GDFZt62mZ_pAG7uCMszxmeoCppE4UOpATxW_Vlfr9-mcIOLc-3KbV86Hzt\
lKEf9DsvhtFpAVrXwGxWEIesBN_Nwug7MaHFY4CnKLAAteahWSjjnzSgzLYoZUjstMquEB54mBNKeaukvUFAbV6C8WfQXrbch1s\
9Wurnf8MztS7IQ1i1NQQOytMvSfpJa4elgdKTYrJVmb-ZF_tggGG50E1eIh0baf8OQxzE9j3bgiwSTGEQU28NRmPp1SxINS6Se3\
5rIlXOwkpW6NtrV5r57fdvS481nyf3OzlRDIhtISnmys&ref=orjY4mGPRjk5boDnW0uvlrrd71vZw9kptjyJN8vfhd2nemAbVs\
wsnTCcPIMJxEzLP_QL2UjGhDZDq0yLnwbFmQ&l10n=ru&cts=1484916812647&mc=2.7321588913645707", info, keys);

        // both text and etext, text is different and has priority
        info.Query = "вк";
        KS_TEST_URL_WITH_KEYS("http://yandex.ru/clck/jsredir?from=yandex.ru%3Bsearch%2F%3Bweb%3B%3B&text=%D0%B2%D0%BA\
&etext=1307.QsrR04OMVcvgFTfbBKX2AKaz1x_ArBbvFNglnYY2Wu0.6b04e02d28f453780389abb2cfb70d349a17d417\
&uuid=&state=PEtFfuTeVD5kpHnK9lio9WCnKp0DidhE9rs5TGtBySwiRXKUtOaYc_CcYwClH-bY7Fd6cgFsfxVrbJIyvKrrbg\
&data=UlNrNmk5WktYejY4cHFySjRXSWhXRkNYS1ZJTW9paUxXcG1iNmR6N2JoZk40Mmg5MTJwSElHOXNid3B4b0p2TmdWOExRe\
jJhdmQ3aU9wOFdna2Q4OEE&b64e=2&sign=bf2069394518dc3fa939752785816771&keyno=0&cst=AiuY0DBWFJ5fN_r-AEs\
zk7ZsMCg4ImCqTxB_GKFqHJWADXa3G6p1_cUfXZ52z3WfomCXdAam8HdN9IsUllLo1vrVJWt_cNC2kq6BlAknbHvi4XLCdDmd7c\
o1jx2HSUT_ZZ_ymwF01FsTK4Z_Mb1ly7kk55SL-U_GDFZt62mZ_pAG7uCMszxmeoCppE4UOpATxW_Vlfr9-mcIOLc-3KbV86Hzt\
lKEf9DsvhtFpAVrXwGxWEIesBN_Nwug7MaHFY4CnKLAAteahWSjjnzSgzLYoZUjstMquEB54mBNKeaukvUFAbV6C8WfQXrbch1s\
9Wurnf8MztS7IQ1i1NQQOytMvSfpJa4elgdKTYrJVmb-ZF_tggGG50E1eIh0baf8OQxzE9j3bgiwSTGEQU28NRmPp1SxINS6Se3\
5rIlXOwkpW6NtrV5r57fdvS481nyf3OzlRDIhtISnmys&ref=orjY4mGPRjk5boDnW0uvlrrd71vZw9kptjyJN8vfhd2nemAbVs\
wsnTCcPIMJxEzLP_QL2UjGhDZDq0yLnwbFmQ&l10n=ru&cts=1484916812647&mc=2.7321588913645707", info, keys);
    }
}
