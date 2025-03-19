#include "seinfo_ut.h"

using namespace NSe;

void TestSeFunc66() {
    // ADVERT_SERP
    {
        TInfo info(SE_YANDEX, ST_ADV_SERP);
        KS_TEST_URL("http://yabs.yandex.ru/count/8ZvYRal-nP040X00gP800OwrC6yR1LAL0Pi2RaEt0II8iW\
EPu069eZxJV9X2dgGQlG6TfRzs1fAgSC6HfXsAgFk_EQe1fQtuXG6D0P6yJuhC2fE53Pa5GeoWQte1eA2WXW6le6jw0PIOSmUdc\
DaYgA1lAJ-k-p85IBpLtTu4iG6o1Bl-_Dwm8XkVD0Hu4000",
                    info);
        KS_TEST_URL("http://market-click2.yandex.ru/redir/1D3Z_cwGDsohfk7MbQwClGxvnhqYfmD_GbUUe\
I5b3fvW2Xn2bR5up8imOB-M3dWjNu4vlzFMp8JBe5G3F4JUlvYIyEC0YC2T0m8Mx6ySmglgF2wjWstBiUB6GApAWplv2liV8Fym\
De3cotAjxjpOhfzYGr0kC6V11E3zl08wuvSyirot7vfY82hphWKOLMnCkCzVM2qM9pqQ9JudEXVTz9eMm5VPdcpDg-3mTInmVRB\
GizTZtKgYHXWXo7r9DXHn7rIJCHZQrq8oIIyr7f7XDUWWE-I885p3zuVHYxrHPaMFkeY3on3ih6F6Dr5EEZfbxWxztfc0GjPLRl\
JkYlRmPg?data=QVyKqSPyGQwwaFPWqjjgNrKm6aF38eTtN4UR_lOwkuzQPPgN4x5BrwarlFAzkevVM2hH2D6hO87jNU-eV82j7\
S2-CST6bLqJbDyAakpQ5fECxPUtC5kIp97Avv-AidvUHLbk5o7KXtE&b64e=2&sign=a389b37791fc0e5dc722fb0adaef1911\
&keyno=1",
                    info);

        info.Query = "пластиковые окна";
        KS_TEST_URL("http://yabs.yandex.ru/count/AMM2jhm5W_K40Wm0gQB0022EjgfAP0M5Z_YL0Pi2RaEt0II…iz"
                    "VU2Pmd3CMF3zB__________m_J__________yFVHG0?q=%D0%BF%D0%BB%D0%B0%D1%81%D1%82"
                    "%D0%B8%D0%BA%D0%BE%D0%B2%D1%8B%D0%B5+%D0%BE%D0%BA%D0%BD%D0%B0",
                    info);

        info.Name = SE_GOOGLE;
        KS_TEST_URL("http://www.google.ru/aclk?sa=L&ai=CdhEZkPxoUsb2DrT57AbolIH4D4iMgdwG4NSJ7ZI\
BtKfzwQgIABABKANQ89_zwvn_____AWCEleyF3B3IAQGpAnLGOrYw92A-qgQhT9DoVLkJKz2ZuDe1tU5dFcuH-G5KfvCs5nWAxm\
3i7LO2uAYBgAeItb8-kAcC&sig=AOD64_1KFAqZdyDNniWXsK0U1e3qZbygWA&rct=j&q=%D0%BF%D0%BB%D0%B0%D1%81%D1%8\
2%D0%B8%D0%BA%D0%BE%D0%B2%D1%8B%D0%B5+%D0%BE%D0%BA%D0%BD%D0%B0&ved=0CCkQ0Qw&adurl=http://oknakonsal\
t.ru/metalloplastikovye-okna-ot-firmy-proizvoditelja",
                    info);
        info.Query = "";
        KS_TEST_URL("http://www.google.com/aclk?sa=l&ai=CPato5UG8Udm7MvPswQOJn4HgCcLGydUD8vDFwV\
C9681ECAAQASDijKkXUPC7lNb5_____wFghJXvhYweoAH4s47-A8gBAakCi5jugnxhtj6qBChP0DgfPW1yhd63MvxztjDu6ya1L\
ZSIId5ycpvmNJ8zq-9XJ95REAn7gAfwy_EB&sig=AOD64_2K3Mm-VOG9QH0_OtUltiJ9FkK0pQ&adurl=https://ableton.co\
m/en/trial/&ba=1",
                    info);
        info.Query = "ноутбуки";
        KS_TEST_URL("http://www.google.ru/aclk?sa=L&amp;ai=Cwn4SG-7wUfCuOa_37Aam4oGgBMDm3_QCwN2\
GhFSAu6DIAQgAEAEoA1D1m9TR-P____8BYISV7IXcHcgBAaoEIk_QIfeuT8yyxaiLydQryZJkXvMjMWlRBmYPgBNBcvMM5AyAB7\
DBuiA&amp;sig=AOD64_2JSzXllEQ1bOnXWzwhIyQgmf2vQA&amp;rct=j&amp;q=%D0%BD%D0%BE%D1%83%D1%82%D0%B1%D1%\
83%D0%BA%D0%B8&amp;ved=0CC0Q0Qw&amp;adurl=http://www.notik.ru",
                    info);
    }
}
