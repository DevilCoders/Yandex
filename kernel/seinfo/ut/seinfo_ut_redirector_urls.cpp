#include "seinfo_ut.h"

using namespace NSe;

void TestSeFunc59() {
    // Redirector urls
    {
        // Google redirector urls
        {
            TInfo info(SE_GOOGLE, ST_WEB, "", ESearchFlags(SF_SEARCH | SF_REDIRECT));

            info.Query = "grey court school";
            KS_TEST_URL("http://www.google.co.uk/url?sa=t&rct=j&q=grey%20court%20school&source=web&cd=1&ved=0CDAQFjAA&url=http%3A%2F%2Fwww.greycourt.richmond.sch.uk%2F&ei=Ru3pUJWXLIHJ0QXE1IHYAg&usg=AFQjCNFoaGLKTlepEU_7kvGb7qid0kkrow&bvm=bv.1355534169,d.d2k", info);

            info.Query = "kommerzienrat krämer, st. ingbert";
            KS_TEST_URL("http://www.google.de/url?sa=t&rct=j&q=kommerzienrat%20kr%C3%A4mer%2C%20st.%20ingbert&source=web&cd=2&ved=0CDcQFjAB&url=http%3A%2F%2Fpersonensuche.dastelefonbuch.de%2FNachnamen%2FBay%2FSt_Ingbert.html&ei=ZLP-UOjnBsrltQb-4YDgDw&usg=AFQjCNEGF2oZkoqP1PMbrYcAKgsx9U-ACw", info);

            // no query info (https redirects)
            info.Query = "";
            KS_TEST_URL("http://www.google.com/url?sa=t&rct=j&q=&esrc=s&source=web&cd=1&cad=rja&ved=0CDIQFjAA&url=http%3A%2F%2Fsdf.lonestar.org%2F&ei=jnUBUdeNKOqg4gT2noGICA&usg=AFQjCNGmbAAnlEEsX6sr0HtNsEyYQtmzsg&bvm=bv.41248874,d.bGE", info);

            info.Query = "";
            KS_TEST_URL("http://www.google.com.br/url?sa=t&rct=j&q=&esrc=s&source=web&cd=1&ved=0CC4QuAIwAA&url=http%3A%2F%2Fwww.youtube.com%2Fwatch%3Fv%3D1lSVOanQJuo&ei=icn-UKm5AoOc8gTqrYHICg&usg=AFQjCNGKbyuQcj9DmnoD1Kj2fK13rMaW6A&bvm=bv.41248874,d.eWU", info);

            // aropan@'s test
            info.Query = "работа.юа";
            KS_TEST_URL("http://www.google.com/url?sa=t&source=web&cd=2&ved=0CDEQjBAwAQ&url=http%3A%2F%2Frabota.ua%2F%25D0%25BA%25D0%25B8%25D0%25B5%25D0%25B2&rct=j&q=%D1%80%D0%B0%D0%B1%D0%BE%D1%82%D0%B0.%D1%8E%D0%B0&ei=zmBFVLSTD4fiywOwlIHIBg&usg=AFQjCNEOy-VD7gqeCkW6izHbcf-vsq4FVA&bvm=bv.77880786,d.bGQ", info);
        }

        // Yahoo redirector urls
        {
            TInfo info(SE_YAHOO, ST_WEB, "", ESearchFlags(SF_SEARCH | SF_REDIRECT));

            KS_TEST_URL("http://ru.search.yahoo.com/r/", info);
            KS_TEST_URL("http://ru.search.yahoo.com/r/_ylt=A7x9QbwjispQF14A0BzLxgt.;_ylu=X3oDMTBybWh0ZnN2BHNlYwNzcgRwb3MDNgRjb2xvA2lyZAR2dGlkAw--/SIG=11mgeedur/EXP=1355479715/**http%3a//www.myspace.com/julee_r%23!", info);

            KS_TEST_URL("http://search.yahoo.com/r/_ylt=A0oG7m9enQZRO2oAN2RXNyoA;_ylu=X3oDMTE1MGV2dmF1BHNlYwNzcgRwb3MDNwRjb2xvA2FjMgR2dGlkA1ZJUDIwN18yMTc-/SIG=128m75lgb/EXP=1359416798/**http%3a//www.4guysfromrolla.com/webtech/011700-1.shtml", info);
        }
    }
}
