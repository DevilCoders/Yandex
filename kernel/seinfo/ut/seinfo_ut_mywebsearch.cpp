#include "seinfo_ut.h"

using namespace NSe;

void TestSeFunc29() {
    // mywebsearch
    {
        TInfo info(SE_MY_WEB_SEARCH, ST_WEB, "cow in space", SF_SEARCH);

        KS_TEST_URL("http://search.mywebsearch.com/mywebsearch/GGmain.jhtml?searchfor=cow+in+space&st=hp&ptb=&id=ZZ&ptnrS=ZZyyyyyyYYUS&n=&tpr=hpsb", info);

        info.Type = ST_IMAGES;
        KS_TEST_URL("http://search.mywebsearch.com/mywebsearch/AJimage.jhtml?searchfor=cow+in+space&tpr=sbt&st=hp&id=ZZyyyyyyYYUS&ptnrS=ZZyyyyyyYYUS&ss=sub&gcht=&ps=cow+in+space", info);

        info.Type = ST_COM;
        KS_TEST_URL("http://search.mywebsearch.com/mywebsearch/PRshop.jhtml?searchfor=cow+in+space&tpr=sbt&st=hp&id=ZZyyyyyyYYUS&ptnrS=ZZyyyyyyYYUS&ss=sub&gcht=&ps=cow+in+space", info);

        info.Type = ST_CATALOG;
        KS_TEST_URL("http://search.mywebsearch.com/mywebsearch/GGdirs.jhtml?searchfor=cow+in+space&tpr=sbt&st=hp&id=ZZyyyyyyYYUS&ptnrS=ZZyyyyyyYYUS&ss=sub&gcht=&ps=cow+in+space", info);
    }
}
