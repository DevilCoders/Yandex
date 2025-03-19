#include "seinfo_ut.h"

using namespace NSe;

void TestSeFunc56() {
    // Yandex 2.0. BUKI-1749
    {
        TInfo info(SE_YANDEX, ST_WEB, "зимнее пальто купить в москве", SF_SEARCH);

        KS_TEST_URL(GenerateYandex2_0SearchUrl("yandex.ru", "search", "web"), info);
        KS_TEST_URL(GenerateYandex2_0SearchUrl("yandex.kz", "search", "web"), info);
        KS_TEST_URL(GenerateYandex2_0SearchUrl("yandex.ru", "xmlsearch", "web"), info);
        KS_TEST_URL(GenerateYandex2_0SearchUrl("yandex.ru", "jsonsearch", "web"), info);

        info.Flags = ESearchFlags(SF_MOBILE | SF_SEARCH);
        KS_TEST_URL(GenerateYandex2_0SearchUrl("yandex.ru", "search", "searchapp"), info);
        KS_TEST_URL(GenerateYandex2_0SearchUrl("yandex.ru", "search", "searchapp", "", "133"), info);
        KS_TEST_URL(GenerateYandex2_0SearchUrl("yandex.ru", "msearch", "web"), info);
        KS_TEST_URL(GenerateYandex2_0SearchUrl("yandex.ru", "msearchpart", "web"), info);
        KS_TEST_URL(GenerateYandex2_0SearchUrl("yandex.ru", "padsearch", "web"), info);
        info.Platform = P_ANDROID;
        KS_TEST_URL(GenerateYandex2_0SearchUrl("yandex.ru", "msearch", "searchapp", "android"), info);
        info.Platform = P_IPHONE;
        KS_TEST_URL(GenerateYandex2_0SearchUrl("yandex.ru", "msearch", "searchapp", "iphone"), info);

        info.Query = "вконтакте добро пожаловать";
        info.Platform = P_ANDROID;
        KS_TEST_URL("http://clck.yandex.ru/jsredir?from=yandex.ru%3Bjsonsearch%3Bsearchapp%3Ban\
droid%3B311%3B1377381452392579-934113842993900858706836-ws37-918-APPJS&text=%D0%B2%D0%BA%D0%BE%D0%B\
D%D1%82%D0%B0%D0%BA%D1%82%D0%B5%20%D0%B4%D0%BE%D0%B1%D1%80%D0%BE%20%D0%BF%D0%BE%D0%B6%D0%B0%D0%BB%D\
0%BE%D0%B2%D0%B0%D1%82%D1%8C&state=AiuY0DBWFJ4ePaEse6rgeBRtUKZphe4qTCt8_XtUCJZyp2TIE2d9hWHybJRys6ml\
QbBG2KqCOr-TM0RS9cjbQCs8zEn_02M44xduINO-VaQ0k4GTNCLvWRTOifIFRaU7hgYYeMtiDIeDzjnZVVxqWEbWdXDnQngLKy6\
fUu4swCtcWl8wNJVEY9YzK2ZxDn0jsJPEkneVDhaBpzFl6MwFphF_FJ0kxeMIaTdyvMzWPPhoUCt8AcyuIvkaRJP6DaTtpq2AkM\
JDX5f5wlI-Qxqo4VfWsMfZNoRFqbWZRnNtRvW5z-qLEShZt_xAGPpOcyaQJxMN99TzKjg&data=UlNrNmk5WktYejR0eWJFYk1L\
dmtxcXdTelJEd2ZVZDZzX0JROGdLR3ZFOXdIMm5pNkUzTFVZTktOVVVTdURPelRNQzBoUjV1VDRN&b64e=2&sign=14b3f6ccc7\
a22005f434c6df45963c5c&keyno=",
                    info);

        info.Query = "фонтанка происшествия";
        info.Flags = SF_SEARCH;
        info.Platform = P_IPAD;
        KS_TEST_URL("http://yandex.ru/clck/jsredir?from=yandex.ru%3Bjsonsearch%3Bbrowser%3Bipad\
%3B220&text=%D1%84%D0%BE%D0%BD%D1%82%D0%B0%D0%BD%D0%BA%D0%B0%20%D0%BF%D1%80%D0%BE%D0%B8%D1%81%D1%88\
%D0%B5%D1%81%D1%82%D0%B2%D0%B8%D1%8F",
                    info);

        info.Query = "кино зеленоград";
        info.Platform = P_WINDOWS_PHONE;
        info.Flags = ESearchFlags(SF_SEARCH | SF_MOBILE);
        KS_TEST_URL("http://clck.yandex.ru/jsredir?from=yandex.ru%3Bjsonsearch%3Bsearchapp%3Bwp\
%3B108%3B1377425762378800-230179511353868156780489-ws36-867-APPJS&text=%D0%BA%D0%B8%D0%BD%D0%BE%20%\
D0%B7%D0%B5%D0%BB%D0%B5%D0%BD%D0%BE%D0%B3%D1%80%D0%B0%D0%B4&state=AiuY0DBWFJ4ePaEse6rgeCPM-MH1jM4y6\
4AgvS4JXu8jAcYMEzjRuPwqDaLUNzAdu1zaRH7bsCvj7dsRXjBTWqG633iN1GBhY7iWf0XzX0dAnJ6JC9PYdXJWMcj5zkcsMjZY\
71jAAMMwP52U18vzW9H6tH8uCN1pmsih1UghjxRulMBIM05VvswG0sd2k3hPDno_w4Yz7BPzJq3SnwXoiLt-Io1uisRyvd3okQR\
gQ8z9tVJnvVvB9RiHTOYyFStJlHDmj8VSzpojFRk-L",
                    info);

        info.Query = "химчистка диана";
        info.Platform = P_WINDOWS_RT;
        KS_TEST_URL("http://clck.yandex.ru/jsredir?from=yandex.ru%3Bjsonsearch%3Bsearchapp%3Bwi\
nrt%3B100%3B1377445420864368-1726630775847641670227559-7-038-APPJS&text=%D1%85%D0%B8%D0%BC%D1%87%D0\
%B8%D1%81%D1%82%D0%BA%D0%B0%20%D0%B4%D0%B8%D0%B0%D0%BD%D0%B0&state=AiuY0DBWFJ4ePaEse6rgeO09RXtez1Xe\
XG9-jzM8x7sRCxiFjpzmJlv8tVmwYURmzBNgpBD22ulqdHhdpEMPReAG34xO00O5UqUselFceDPO9-bguPx-DDoGmipbrGttSS8\
b7gE-w3mDfjZrdFZQvx4r6oRS1UDR4QWvyQmCsioOSJgI8uOnspqtTyQ6C--qT3zzwxqS4AjAIWjliahBFaKvj3JAesPu1D36HC\
f3ODq8RKqnHqQM3Ts3SGkP0gYf3t3WALDepEARVUS6wnehJ7C7aTH-xnLEWxFRCG_KLSY1Ua3PH5iI_riGDVVI8sMBwyLIAuNyx\
4_QZ19ZQN6SAadixUvILikQ2yBkNkw7aIBUNXRrm8NE_g&data=UlNrNmk5WktYejR0eWJFYk1LdmtxZzhoeUw4dWF5Y3ZQcjAz\
RFZrN0oxRzRGNXRaNWsySG00SFJhaDlpSVVDSlNjSTlwdktjZjFZ&b64e=2&sign=c0be81809ad0744fa9272d56b00d018a&k\
eyno=0",
                    info);

        info.Query = "карта белорусского вокзала в москве";
        info.Platform = P_IPHONE;
        KS_TEST_URL("http://yandex.ru/clck/jsredir?from=yandex.ru%3Bjsonsearch%3Bsearchapp%3Bip\
hone%3B202&text=%D0%BA%D0%B0%D1%80%D1%82%D0%B0%20%D0%B1%D0%B5%D0%BB%D0%BE%D1%80%D1%83%D1%81%D1%81%D\
0%BA%D0%BE%D0%B3%D0%BE%20%D0%B2%D0%BE%D0%BA%D0%B7%D0%B0%D0%BB%D0%B0%20%D0%B2%20%D0%BC%D0%BE%D1%81%D\
0%BA%D0%B2%D0%B5",
                    info);

        info.Query = "ford focus";
        info.Platform = P_UNKNOWN;
        info.Flags = ESearchFlags(SF_SEARCH);
        KS_TEST_URL("http://www.yandex.com/clck/jsredir?from=www.yandex.com%3Bsearch%2F%3Bweb%3B%3B&text=ford%20focus&uuid=&state=PEtFfuTeVD5kpHnK9lio9cUEbBkAQk_OhAzHcv9PulIpF4ylKJ2-qA&data=UlNrNmk5WktYejR0eWJFYk1LdmtxaE9HTmxnVlNlS282UUY4dWRlRnNQOFMzN3kySmRTYl9najZiVWRnT1ZGSlZaXzBBN3dldjAzcHhGRTQtRHduMHY3YUszM25xVmR2amlnTGRhall5NV9zckJheE9JQnh0LUxFZkE2bGdSZUdJelY5X00xNkN2NA&b64e=2&sign=0d3e20f11dda0067aa6bd410ae590576&keyno=0&cst=AiuY0DBWFJ5fN_r-AEszk0k9I6Q_xrdKuW1GVRRAlB7gKcwf4t4P37FCeO2jqnqkMMIpdQwQCsI60JbEbXCiFPlbJ3vbhpOSbsuOngI3TSu3C3AYO2YVfHUgXK6DPuYBp5ifWVuN-Q35Y-iS7WeoBnc5X5M_8FHPNhTrctnFqaGHD-429xuBVyj57kPl15F2&ref=orjY4mGPRjlSKyJlbRuxUnU39LXnsQVO9uEByGOz3J76ZeRg9281hXmU9NPcxB18Yqvnm4zjPdWVMDWbkd28GxaISOVA0ZOJSpT9bSt725m1LhUfrCBoxN0JtJLu4EMKudg1_P-XmFdmvEd4BPLeYzOTbwGpcN3WrRW1wmlk9y4Xn2Lz2PP6-5NFymI_Aevjbn13wa1KmKQ&l10n=en&cts=1440541077050&mc=4.9779168746936335", info);
    }
}
