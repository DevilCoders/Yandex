#include "regexps_impl.h"

namespace NSe
{
    const TSESimpleRegexp RagelizedRegexpsArray[] = {
        // Yandex
        TSESimpleRegexp(
            TStringBuf("(?:www\\.|(?P<com>dire(?:c|k)t)\\.|(?P<mobile>(?:m|pda)\\.)?(?P<news>news|haber)\\.|(?P<video>video)\\.|(?:xmlsearch)\\.|(?:yandsearch)\\.|(?:(?P<mobile>m\\.)?newssearch)\\.)?(?P<engine>yandex)\\.[\\w\\.]{2,}/(?:yand|family|school|site|(?P<video>video/)|(?P<images>(?:gorsel|images)/)|(?P<news>news/)|(?P<mobile>m|touch)|xml)?(?P<mobile>pad/|touch/)?(?P<search>search)(?P<mobile>/pad|/touch)?"),
            TStringBuf("text"),
            TStringBuf("(?:(?:filter=(?P<people>people)?)|(?:numdoc=(?P<psize>[\\d]*))|(?:p=(?P<pnum>[\\d]*)))")
        ),
        TSESimpleRegexp(
            TStringBuf("(?:www\\.|(?P<mobile>m)\\.)?(?:(?P<images>images)\\.(?P<engine>yandex)\\.[\\w\\.]{2,}|(?P<images>gorsel)\\.(?P<engine>yandex)\\.com\\.tr|(?P<engine>yandex)\\.com\\.tr/(?P<images>gorsel))"),
            TStringBuf("(?P<search>text|(?P<people>person))"),
            TStringBuf("(?P<local>(?:img_url|site|serverurl|surl))=[^#&\\?]*")
        ),
        TSESimpleRegexp(
            TStringBuf("(?:www\\.)?(?P<mobile>(?:m|pda)\\.)?(?P<cat>yaca)\\.(?P<engine>yandex)(?P<search>\\.)[\\w\\.]{2,}"),
            TStringBuf("text")
        ),
        TSESimpleRegexp(
            TStringBuf("(?:www\\.|(?P<mobile>m\\.))?(?:(?P<maps>maps|harita)\\.|(?P<video>video\\.)|(?P<com>market\\.)|(?P<cars>auto\\.)|(?P<int>afis|afisha)\\.|(?P<images>fotki\\.))(?P<engine>yandex)(?P<search>\\.)[\\w\\.]{2,}"),
            TStringBuf("text")
        ),
        TSESimpleRegexp(
            TStringBuf("(?P<engine>yandex)\\.com(?:\\.tr)?/(?P<video>video)/(?:\\#!/video/)?(?P<search>search)"),
            TStringBuf("text")
        ),
        TSESimpleRegexp(
            TStringBuf("(?:www\\.)?(?P<encyc>slovari)\\.(?P<engine>yandex)(?P<search>\\.)[\\w\\.]{2,}/(?P<query>[^/]*)/(?:(?:%D0%B7%D0%BD%D0%B0%D1%87%D0%B5%D0%BD%D0%B8%D0%B5|%D0%BF%D1%80%D0%B0%D0%B2%D0%BE%D0%BF%D0%B8%D1%81%D0%B0%D0%BD%D0%B8%D0%B5|%D0%BF%D1%80%D0%B0%D0%B2%D0%BE%D0%BF%D0%B8%D1%81%D0%B0%D0%BD%D0%B8%D0%B5)/)?")
        ),
        TSESimpleRegexp(
            TStringBuf("(?P<mobile>m)\\.(?P<encyc>slovari)\\.(?P<engine>yandex)(?P<search>\\.[\\w\\.]{2,})/(?:translate|meaning|spelling)\\.xml"),
            TStringBuf("text")
        ),
        // Yandex 2.0 (@see: SERP-11065)
        TSESimpleRegexp(
            TStringBuf("(?:(?:clck\\.(?P<engine>yandex)(?P<search>\\.)[\\w\\.]{2,})|(?:(?:www\\.)?(?P<engine>yandex)(?P<search>\\.)[\\w\\.]{2,}/clck))/jsredir"),
            TStringBuf("text"),
            // @see: https://jira.yandex-team.ru/browse/BUKI-1749?focusedCommentId=3185898&page=com.atlassian.jira.plugin.system.issuetabpanels%3Acomment-tabpanel#comment-3185898
            "(?:"
                "from="
                    "(?:(?:(?P<images>images)|(?P<video>video)|www)\\.)?yandex\\.[\\w\\.]{2,}(?:;|%3B)"             // domain
                    "(?:yand|family|school|(?P<local>site)|(?P<mobile>m|tel|touch|pad)|large|json|xml)?search(?:part)?(?:;|%3B)" // handler
                    "(?:web|browser|(?P<mobile>searchapp))(?:;|%3B)"  // service
                    "(?P<platform>android|iphone|ipad|wp|winrt)?(?:;|%3B)" // platform
                    "[^&]*"  // version
                "|(?:etext=(?P<encrypted_query>[^#&\\?]*)))"sv
            //TStringBuf("(?:etext=(?P<encrypted_query>[^#&\\?]*))")
        ),
    };

    const TSESimpleRegexp DefaultSearchRegexpsArray[] = {
        // Google
        TSESimpleRegexp(
            TStringBuf(
                "(?:"
                    "www\\."
                    "|(?P<images>images)\\."
                    "|(?P<answer>otvety)\\."
                    "|(?P<maps>maps)\\."
                    "|(?P<science>scholar)\\."
                    "|(?P<cat>sites)\\."
                ")?"
                "(?P<engine>google)"
                "(?P<search>\\.)"
                "(?P<science>scholar\\.)?"
                "(?:[\\w]{2}|co\\.[\\w]{2}|com(?:\\.[\\w]{2})?)"
                "/"
                "(?P<mobile>mobile/)?"
                "(?:"
                    "(?P<science>scholar)"
                    "|(?P<com>finance)"
                    "|search"
                    "|(?P<maps>maps)(?:/(place|search)/(?P<query>[^/]+)/)?"
                    "|otvety"
                    "|site(?P<cat>search)?"
                    "|ie"
                    "|(?P<redirect>url)"
                    "|places"
                    "|images"
                    "|webhp"
                    "|(?=\\#)"
                    "|(?P<mobile>m)"
                    "|custom"
                    "|xhtml"
                    "|(?P<images>imgres)"
                    "|cse"
                    "|uds/afs"
                    "|\\?"
                ")"
            ),
            TStringBuf("fake"),
            TStringBuf(
                "(?:"
                    "(?:tbm="
                        "(?:"
                            "(?P<images>isch)"
                            "|(?P<video>vid)"
                            "|(?P<news>nws)"
                            "|(?P<blog>blg)"
                            "|(?P<com>shop)"
                        ")"
                    ")"
                    "|(?:(?P<answer>btnG)=Search\\+Answers)"
                    "|(?:as_(?P<local>sitesearch)=[^#&\\?]*)"
                    "|(?:tbs=(?P<byimage>sbi|simg)(?P<query>)[^#&\\?]*)"
                    "|(?:gs_l=(?P<mobile>mobile)[^#&\\?]*)"
                    "|(?:source="
                        "(?:"
                            "(?P<images>images)"
                            "|(?P<blog>blogsearch)"
                            "|(?P<news>newssearch)"
                            "|(?P<com>productsearch|finance)"
                            "|suggest"
                            "|(?P<video>video)"
                            "|w|web|webhp"
                            "|(?P<books>books)"
                        ")"
                    ")"
                    "|as_q=(?P<query>[^#&\\?]*)"
                    "|(?:q(?:uery)?=(?P<query>[^#&\\?]*))"
                    "|(?:num=(?P<psize>[^#&\\?]*))"
                    "|(?:start=(?P<pstart>[^#&\\?]*))"
                    "|(?:(?P<byimage>imgrefurl)=(?P<low_query>[^#&\\?]*))"
                ")"
            )
        ),
        TSESimpleRegexp(
            TStringBuf("(?:www\\.)?(?P<engine>google)(?P<search>\\.)[\\w\\.]{2,}/(?P<mobile>mobile/)?(?P<cat>sitesearch)"),
            TStringBuf("q"),
            TStringBuf("(?:(?:num=(?P<psize>[^#&\\?]*))|(?:start=(?P<pstart>(?P<query>[^#&\\?]*))))")
        ),
        // mail.ru
        TSESimpleRegexp(
            TStringBuf("(?:www\\.)?go\\.(?P<engine>mail\\.ru)/(?P<search>(?:search(?:(?P<images>_images)|(?P<video>_video)|[^_?])?)|(?P<blog>realtime)|(?:(?P<mobile>m)(?:search|(?P<images>images)|(?P<answer>answer))))"),
            TStringBuf("q")
        ),
        TSESimpleRegexp(
            TStringBuf("(?:www\\.)?(?:(?:(?P<video>video)(?:\\.))|(?P<com>torg)\\.|(?P<encyc>search\\.enc)\\.)(?P<engine>mail(?P<search>\\.)ru)"),
            TStringBuf("q")
        ),
        // baidu
        TSESimpleRegexp(
            TStringBuf("(?:www\\.)?(?P<video>v\\.|(?P<images>image\\.)|(?P<news>news\\.))?(?P<engine>baidu)(?P<search>\\.)com/(?:s|i|ns|v)"),
            TStringBuf("(?:wd|word)")
        ),
        // rambler
        TSESimpleRegexp(
            TStringBuf(
                "(?:www\\.)?"
                "(?:"
                    "(?P<news>news\\.)"
                    "|(?P<maps>maps\\.)"
                    "|(?P<encyc>dict\\.)"
                    "|(?P<com>supermarket\\.)"
                    "|(?P<cat>top100\\.)"
                    "|(?P<images>(?:images|foto)\\.)"
                    "|(?P<mobile>m\\.)"
                    "|(?P<search>search\\.)"
                    // TODO: replace this with "|(?!mail|ad)[a-z0-9_]+\\."
                    "|nova\\."
                    "|horoscopes\\."
                ")*"
                "(?P<engine>rambler)(?P<search>\\.)ru"
                "(?:"
                    "/(?P<video>video)"
                    "|/(?P<images>pictures)"
                    "|/(?P<news>news)"
                    "|/(?P<music>premium-music)"
                    "|/(?P<search>search)"
                ")*"),
            TStringBuf("(?:query|words|search|t)"),
            TStringBuf("filter=(?:(?P<sport>sport)|(?P<tv>tv))\\.rambler\\.ru")
        ),
        // conduit
        TSESimpleRegexp(
            TStringBuf("(?P<search>(?P<apps>apps)|search)\\.(?P<engine>conduit)\\.com"),
            TStringBuf("q")
        ),
        // Bing
        TSESimpleRegexp(
            TStringBuf("(?:www\\.)?(?P<engine>bing)\\.com/(?P<search>search|(?P<images>images)|(?P<video>(?:videos|movies))|(?P<com>shopping)|(?P<news>news)|(?P<maps>maps)|(?P<encyc>Dictionary)|(?P<events>events)|(?P<rec>recipe)|(?P<cars>autos)|(?P<social>social))"),
            TStringBuf("(?:q|Q)"),
            TStringBuf("nrv=(?P<music>music)")
        ),
        TSESimpleRegexp(
            TStringBuf("(?P<mobile>m)\\.(?P<engine>bing)\\.com/(?P<search>search)"),
            TStringBuf("(?:q|Q)"),
            TStringBuf("D=(?P<images>Image)")
        ),
        // Moikrug
        TSESimpleRegexp(
            TStringBuf("(?:(?:www\\.)?(?P<engine>(?P<fsocial>moikrug))\\.ru/(?P<search>(?P<local>(?P<people>persons|resumes)|(?P<job>vacancies)|(?P<com>companies|services)))?|(?P<mobile>m\\.)(?P<engine>(?P<fsocial>moikrug))\\.yandex\\.ru/(?P<search>(?P<local>(?P<social>search)))?)"),
            TStringBuf("keywords")
        ),
        // Yahoo!
        TSESimpleRegexp(
            TStringBuf(
                "(?:"
                    "(?P<images>images?\\.)"
                    "|(?P<news>news\\.)"
                    "|(?P<video>video\\.)"
                    "|(?P<apps>apps\\.)"
                    "|(?P<com>(?:finance|shopping)\\.)"
                    "|(?P<blog>blogs?\\.)"
                    "|(?P<forum>forum\\.)"
                    "|(?P<sport>sports\\.)"
                    "|(?P<rec>recipes\\.)"
                    "|(?P<encyc>knowledge\\.)"
                    "|(?P<cat>dir\\.)"
                    "|(?P<local>local\\.)"
                    "|(?P<maps>maps\\.)"
                    "|(?P<rec>recipes\\.)"
                    "|(?P<sport>sports\\.)"
                    "|(?P<tv>tv\\.)"
                    "|(?P<com>shopping\\.)"
                    "|(?P<mobile>m\\.)"
                    "|[a-z0-9_]+\\."
                ")*"
                "(?P<engine>yahoo)(?P<search>\\.)[\\w\\.]{2,}"
                "/(?:(?:w|yhs)/)?(?:search|(?P<redirect>r))"),
            TStringBuf("p")
        ),
        // nigma.ru
        TSESimpleRegexp(
            TStringBuf("(?:full\\.|www\\.|(?P<mobile>pda)\\.)?(?P<engine>nigma)(?P<search>\\.)ru"),
            TStringBuf("(?:q|s)"),
            TStringBuf("t=(?:(?P<images>img)|(?P<books>lib)|(?P<music>music))")
        ),
        // aport.ru
        TSESimpleRegexp(
            TStringBuf("(?:sm\\.|www\\.|(?P<mobile>m|pda|wap)\\.|(?P<images>pics)\\.|(?P<tv>tv)\\.|(?P<video>video)\\.)?(?P<engine>aport)(?P<search>\\.)ru"),
            TStringBuf("(?:r|ss)")
        ),
        // blogs.yandex.*
        TSESimpleRegexp(
            TStringBuf("(?:www\\.|(?P<mobile>m)\\.)?(?P<blog>blogs)\\.(?P<engine>yandex)(?P<search>\\.)[\\w\\.]{2,}"),
            TStringBuf("text")
        ),
        // all.by
        TSESimpleRegexp(
            TStringBuf("(?:www\\.)?(?P<engine>all(?P<search>\\.)by)"),
            TStringBuf("(?:query|findtext)")
        ),
        // tut.by
        TSESimpleRegexp(
            TStringBuf("(?:www\\.)?(?:(?P<search>search)\\.|(?P<blog>blog)\\.|(?P<forum>forums)\\.|(?P<music>mp3)\\.|(?P<news>news)\\.)?(?P<engine>tut(?P<search>\\.)by)(?:/search)?"),
            TStringBuf("(?:query|str)"),
            TStringBuf("vs=(?P<video>[^&])|is=(?P<images>[^&])")
        ),
        // search.bigmir.net
        TSESimpleRegexp(
            TStringBuf("(?P<search>search)\\.(?P<engine>bigmir)\\.net"),
            TStringBuf("(?:q|z)")
        ),
        // meta.ua
        TSESimpleRegexp(
            TStringBuf("(?:www\\.)?(?P<engine>meta(?P<search>\\.)ua)"),
            TStringBuf("q")
        ),
        // search.com
        TSESimpleRegexp(
            TStringBuf("(?:www\\.)?(?P<engine>search\\.com)/(?P<search>(?P<com>shop/)?search|(?P<images>images)|(?P<video>video)|(?P<news>news))"),
            TStringBuf("q")
        ),
        // search.aol.com
        TSESimpleRegexp(
            TStringBuf("(?:www\\.)?search\\.(?P<engine>aol)\\.com/aol/(?P<search>search|(?P<images>image)|(?P<video>video)|(?P<news>news))"),
            TStringBuf("(?:q|query)")
        ),
        TSESimpleRegexp(
            TStringBuf("(?P<engine>aol)search\\.(?P<com>pgpartner)\\.com/(?P<search>search)\\.php"),
            TStringBuf("form_keyword")
        ),
        // search.lycos.com
        TSESimpleRegexp(
            TStringBuf("(?:www\\.|(?P<mobile>m)\\.)?(?P<search>search)\\.(?P<engine>lycos)\\.com(?:(?P<images>/image)|(?P<video>/video))?"),
            TStringBuf("query")
        ),
        TSESimpleRegexp(
            TStringBuf("(?:www\\.)?(?P<com>shopping)\\.(?P<engine>lycos)\\.com/(?P<search>search)"),
            TStringBuf("q")
        ),
        TSESimpleRegexp(
            TStringBuf("(?P<news>news)\\.(?P<engine>lycos)\\.com/(?P<search>search)"),
            TStringBuf("query")
        ),
        // qip.ru
        TSESimpleRegexp(
            TStringBuf("(?P<mobile>pda\\.)?(?P<search>search)\\.(?:(?P<images>photo)\\.|(?P<video>video)\\.)?(?P<engine>qip)\\.ru/"),
            TStringBuf("query")
        ),
        TSESimpleRegexp(
            TStringBuf("(?:magna|(?P<maps>maps)|(?P<abs>5ballov)|(?P<events>afisha)|(?P<job>job))\\.(?P<engine>qip)(?P<search>\\.)ru"),
            TStringBuf("(?:q(?:uery)?|search|words)")
        ),
        // blekko
        TSESimpleRegexp(
            TStringBuf("(?:www\\.)?(?P<engine>blekko)(?P<search>\\.)com/ws/(?P<query>.*)")
        ),
        // topsy
        TSESimpleRegexp(
            TStringBuf("(?:www\\.)?(?P<engine>topsy)\\.com/(?P<search>s)(?:\\?q=|/)(?P<query>[^/]*)(?:/(?:(?P<social>tweet)|(?P<images>image)|(?P<video>video)|(?P<people>expert)))?")
        ),
        // icq.com
        TSESimpleRegexp(
            TStringBuf("(?:www\\.)?search\\.(?P<engine>icq\\.com)/(?P<search>search)/(?P<images>img_)?results\\.php"),
            TStringBuf("q")
        ),
        TSESimpleRegexp(
            TStringBuf("(?:www\\.)?(?P<video>video)\\.(?P<engine>icq\\.com)/video/(?P<search>video_results)\\.php"),
            TStringBuf("q")
        ),
        // ask.com
        TSESimpleRegexp(
            TStringBuf("(?:www\\.)?(?:(?P<search>search\\.))?(?P<engine>ask\\.com)/(?P<search>web|(?P<images>pictures)|(?P<news>(?:news|ref))|(?P<video>videos)|(?P<com>shopping))"),
            TStringBuf("q")
        ),
        TSESimpleRegexp(
            TStringBuf("(?:www\\.)?(?P<engine>ask(?P<search>\\.)com)/(?P<maps>local)"),
            TStringBuf("what")
        ),
        TSESimpleRegexp(
            TStringBuf("(?:www\\.)?(?:[\\w\\.]{2,})(?P<engine>ask(?P<search>\\.)com)/(?P<web>web|(?P<images>pictures)|(?P<video>youtube)|(?P<web>fr))"),
            TStringBuf("q")
        ),
        TSESimpleRegexp(
            TStringBuf("(?P<mobile>m)\\.(?P<engine>ask(?P<search>\\.)com)"),
            TStringBuf("(?:QUERY|SEARCHFOR)"),
            TStringBuf("submit-(?:(?P<images>IMAGES-BUTTONIMAGE2\\.x)|(?P<news>NEWS-BUTTONIMAGENEWS\\.x)|(?P<maps>RESULTS-GO))=[^#&\\?]*")
        ),
        // exalead.com
        TSESimpleRegexp(
            TStringBuf("(?:www\\.)?(?P<engine>exalead)\\.(?:com|fr)/(?P<search>search)/(?:web|(?P<images>image)|(?P<video>video)|(?P<encyc>wikipedia))/results/?"),
            TStringBuf("q")
        ),
        // alexa.com
        TSESimpleRegexp(
            TStringBuf("(?:www\\.)?(?P<engine>alexa)\\.com/(?P<search>search)"),
            TStringBuf("q")
        ),
        // quintura
        TSESimpleRegexp(
            TStringBuf("(?:www\\.)?(?:affiliates\\.)?(?P<engine>quintura)(?P<search>\\.)(?:com|ru)"),
            TStringBuf("request")
        ),
        // webalta
        TSESimpleRegexp(
            TStringBuf("(?:www\\.)?(?P<engine>webalta)(?P<search>\\.)ru"),
            TStringBuf("q")
        ),
        // metabot.ru
        TSESimpleRegexp(
            TStringBuf("(?P<search>results)\\.(?P<engine>metabot\\.ru)"),
            TStringBuf("st")
        ),
        // i.ua
        TSESimpleRegexp(
            TStringBuf("(?P<search>search|(?P<cat>catalog)|(?P<com>shop|board))\\.(?P<engine>i\\.ua)"),
            TStringBuf("q")
        ),
        TSESimpleRegexp(
            TStringBuf("(?P<fsocial>narod)\\.(?P<engine>i\\.ua)/(?P<search>(?P<cat>catalog)|(?P<int>interests)|(?P<books>books)|(?P<music>musics)|(?P<video>films)|(?P<games>games)|(?P<people>search))"),
            TStringBuf("words")
        ),
        // daemon-search
        TSESimpleRegexp(
            TStringBuf("(?:www\\.)?(?P<engine>daemon-search)\\.com/[^/]+/explore/(?P<search>web|(?P<images>images)|(?P<video>videos))"),
            TStringBuf("q")
        ),
        // incredimail
        TSESimpleRegexp(
            TStringBuf("(?P<search>search)\\.(?P<engine>incredimail)\\.com"),
            TStringBuf("q")
        ),
        // gde.ru
        TSESimpleRegexp(
            TStringBuf("(?:(?P<com>)|cat\\.|(?P<forum>forums)\\.|(?P<adr>adresa)\\.|(?P<cat>top)\\.)(?P<engine>gde\\.ru)"),
            TStringBuf("(?P<search>keywords|search|t)")
        ),
        // mywebsearch
        TSESimpleRegexp(
            TStringBuf("(?P<search>search)\\.(?P<engine>mywebsearch)\\.com/mywebsearch/(?:GGmain|(?P<images>AJimage)|(?P<com>PRshop)|(?P<cat>GGdirs))\\.jhtml"),
            TStringBuf("searchfor")
        ),
        // search.babylon
        TSESimpleRegexp(
            TStringBuf("i?(?P<engine>search\\.babylon)(?P<search>\\.)com"),
            TStringBuf("q"),
            TStringBuf("s=(?:(?P<images>images)|(?P<video>video)|(?P<news>news))")
        ),
        // seznam.cz
        TSESimpleRegexp(
            TStringBuf("(?P<search>search|(?P<encyc>encyklopedie))\\.(?P<engine>seznam\\.cz)"),
            TStringBuf("q")
        ),
        TSESimpleRegexp(
            TStringBuf("(?P<search>www)(?P<maps>\\.)(?P<engine>mapy\\.cz)"),
            TStringBuf("q")
        ),
        TSESimpleRegexp(
            TStringBuf("(?P<search>www)(?P<com>\\.)(?P<engine>zbozi\\.cz)"),
            TStringBuf("q")
        ),
        TSESimpleRegexp(
            TStringBuf("(?P<search>www)(?P<images>\\.)(?P<engine>obrazky\\.cz)"),
            TStringBuf("q")
        ),
        TSESimpleRegexp(
            TStringBuf("www(?P<com>\\.)(?P<engine>firmy\\.cz)/(?P<search>phr)/(?P<query>[^&\\#\\?]*)")
        ),
        // gbg.bg
        TSESimpleRegexp(
            TStringBuf("(?P<search>find|search)\\.(?P<engine>gbg\\.bg)"),
            TStringBuf("fake"),
            TStringBuf("(?:(?:c=(?:google|(?P<cat>gbg)|(?P<encyc>znam)))|(?:q=(?P<query>[^#&\\?]*)))")
        ),
        // nur.kz
            //# TODO thing on charsets
        TSESimpleRegexp(
            TStringBuf("(?P<search>search|(?P<news>news)|(?P<video>video)|(?P<music>music)|(?P<blog>blog))\\.(?P<engine>nur\\.kz)"),
            TStringBuf("(?:query|str|q|s)")
        ),
        // kazakh.ru
        TSESimpleRegexp(
            TStringBuf("(?:www\\.)?(?P<engine>kazakh\\.ru)/(?:(?P<news>news)/|(?P<forum>talk)/)?(?P<search>search)"),
            TStringBuf("query"),
            TStringBuf("wheresearch=(?P<local>(?:1|2))")
        ),
        TSESimpleRegexp(
            TStringBuf("(?:(?P<blog>blogs)|(?P<cat>catalog))(?P<search>\\.)(?P<engine>kazakh\\.ru)"),
            TStringBuf("q")
        ),
        // duckduckgo
        TSESimpleRegexp(
            TStringBuf("(?:www\\.)?(?P<engine>duckduckgo)(?P<search>\\.)com"),
            TStringBuf("q")
        ),
        // go.km.ru
        TSESimpleRegexp(
            TStringBuf("go\\.(?P<engine>km(?P<search>\\.)ru)"),
            TStringBuf("sq"),
            TStringBuf("idr=(?P<local>1)")
        ),
        // gigabase
        TSESimpleRegexp(
            TStringBuf("(?:www\\.)?(?P<engine>gigabase(?P<search>\\.)ru)"),
            TStringBuf("q")
        ),
        // search.kaz.kz
        TSESimpleRegexp(
            TStringBuf("(?P<engine>search\\.kaz(?P<search>\\.)kz)"),
            TStringBuf("q"),
            TStringBuf("i=(?:(?P<video>1)|(?P<news>2))")
        ),
        // poisk.ru
        TSESimpleRegexp(
            TStringBuf("(?:www\\.)?(?P<engine>poisk(?P<search>\\.)ru)"),
            TStringBuf("text")
        ),
        // gogo
        TSESimpleRegexp(
            TStringBuf("(?:www\\.)?(?P<engine>gogo)\\.ru/(?P<search>go|(?P<images>images)|(?P<video>video))"),
            TStringBuf("q")
        ),
        // youtube
        TSESimpleRegexp(
            TStringBuf("(?:www\\.)?(?P<mobile>m\\.)?(?P<engine>youtube)(?P<video>\\.)(?P<local>com)/(?P<search>results)"),
            TStringBuf("search_query")
        ),
        // tineye
        TSESimpleRegexp(
            TStringBuf("(?:www\\.)?(?P<engine>tineye)(?P<byimage>\\.)com/(?P<search>search)/(?P<query>[^/]*)")
        ),
        // lemoteur
        TSESimpleRegexp(
            TStringBuf("(?:www\\.)?(?P<engine>lemoteur)(?P<search>\\.)fr/"),
            TStringBuf("kw"),
            TStringBuf("bhv=(?:(?P<images>images)|(?P<video>videos)|(?P<news>actu))")
        ),
        // naver.com
        TSESimpleRegexp(
            TStringBuf("(?:(?:cafeblog|kin|news|image|video|dic|magazine|time|realtime|web|(?P<mobile>m))\\.)?(?P<search>search)\\.(?P<engine>naver)\\.com/search\\.naver"),
            TStringBuf("query"),
            TStringBuf("where=(?:m_)?(?:nexearch|site|webkr|realtime|timentopic|(?P<blog>post)|(?P<local>cafe)?(?P<blog>blog)|(?P<encyc>(?:kin|kdic|ldic))|(?P<news>news)|(?P<images>image)|(?P<video>video)|(?P<magazines>magazine))")
        ),
            // TODO: What is where=site, where=webkr, where=timentopic and where=realtime ?
        TSESimpleRegexp(
            TStringBuf("(?P<com>)shopping\\.(?P<engine>naver)\\.com/(?P<search>search)/all_search\\.nhn"),
            TStringBuf("query")
        ),
        TSESimpleRegexp(
            TStringBuf("(?P<maps>map)\\.(?P<engine>naver)(?P<search>\\.)com/index\\.nhn"),
            TStringBuf("query")
        ),
        TSESimpleRegexp(
            TStringBuf("(?P<music>music)\\.(?P<engine>naver)\\.com/search/(?P<search>search)\\.nhn"),
            TStringBuf("query")
        ),
        TSESimpleRegexp(
            TStringBuf("(?P<books>book)\\.(?P<engine>naver)\\.com/(?P<search>search)/search\\.nhn"),
            TStringBuf("query")
        ),
        TSESimpleRegexp(
            TStringBuf("(?P<science>academic)\\.(?P<engine>naver)\\.com/(?P<search>search|noResult)\\.nhn"),
            TStringBuf("query")
        ),
        // liveinternet
        TSESimpleRegexp(
            TStringBuf("(?:www\\.)?(?P<engine>liveinternet)\\.ru/(?P<search>q)/"),
            TStringBuf("q"),
            TStringBuf("t=(?:(?P<images>img)|(?P<video>video)|(?P<web>web))")
        ),
        TSESimpleRegexp(
            TStringBuf("(?:www\\.)?(?P<engine>liveinternet)\\.ru/(?P<com>market)(?P<search>/)"),
            TStringBuf("findtext")
        ),
        TSESimpleRegexp(
            TStringBuf("(?P<news>news)\\.(?P<engine>liveinternet)(?P<search>\\.)ru/"),
            TStringBuf("search")
        ),
        TSESimpleRegexp(
            TStringBuf("www\\.(?P<engine>liveinternet)\\.ru/m?(?P<fsocial>(?P<search>search))/"),
            TStringBuf("q"),
            TStringBuf("i=(?:(?P<music>1)|(?P<images>3)|(?P<video>2))")
        ),
        TSESimpleRegexp(
            TStringBuf("(?:www\\.)?(?P<engine>liveinternet)\\.ru/(?P<people>j)(?P<fsocial>(?P<search>search))/"),
            TStringBuf("s")
        ),
        //handycafe
        TSESimpleRegexp(
            TStringBuf("(?P<search>search)\\.(?P<engine>handycafe)\\.com/(?P<mobile>m/)?(?:results|search.php)"),
            TStringBuf("q"),
            TStringBuf("c=(?:(?P<images>images)|(?P<video>video)|(?<web>web)|(?P<news>news))")
        ),
        //searchya
        TSESimpleRegexp(
            TStringBuf("(?:www\\.)?(?P<engine>searchya)(?P<search>\\.)com/[^\\?]*"),
            TStringBuf("q"),
            TStringBuf("category=(?:(?P<images>images)|(?P<video>video)|(?<web>web)|(?P<news>news))")
        ),
        // sogou
        TSESimpleRegexp(
            TStringBuf("(?:www\\.)?(?P<images>pic\\.|(?P<video>v\\.)|(?P<news>news\\.))?(?P<engine>sogou)(?P<search>\\.)com/(?:web|pics|v|news)"),
            TStringBuf("query")
        ),

        // NEWS

        // ria.ru
        TSESimpleRegexp(
            TStringBuf("(?:www\\.)?(?P<engine>ria)(?P<local>\\.)(?P<news>ru)/(?P<search>search)/"),
            TStringBuf("query")
        ),
        // lenta.ru
        TSESimpleRegexp(
            TStringBuf("(?:www\\.)?(?P<engine>lenta)(?P<local>\\.)(?P<news>ru)/(?P<search>search)/"),
            TStringBuf("query")
        ),
        // gazeta.ru
        TSESimpleRegexp(
            TStringBuf("(?:www\\.)?(?P<engine>gazeta)(?P<local>\\.)(?P<news>ru)/(?P<search>search)\\.shtml"),
            TStringBuf("text")
        ),
        // newsru.com
        TSESimpleRegexp(
            TStringBuf("(?:www\\.)?(?P<engine>newsru)(?P<local>\\.)(?P<news>com)/(?P<search>search)\\.html"),
            TStringBuf("qry")
        ),
        // vesti.ru
        TSESimpleRegexp(
            TStringBuf("(?:www\\.)?(?P<engine>vesti)(?P<local>\\.)(?P<news>ru)/(?P<search>search_adv)\\.html"),
            TStringBuf("q")
        ),
        // rbc.ru
        TSESimpleRegexp(
            TStringBuf("(?:www\\.)?(?P<search>search)\\.(?P<engine>rbc)(?P<local>\\.)(?P<news>ru)/"),
            TStringBuf("query")
        ),
        // vz.ru
        TSESimpleRegexp(
            TStringBuf("(?:www\\.)?(?P<engine>vz)(?P<local>\\.)(?P<news>ru)/(?P<search>search)/"),
            TStringBuf("s_string")
        ),
        // kp.ru
        TSESimpleRegexp(
            TStringBuf("(?:www\\.)?(?P<engine>kp)(?P<local>\\.)(?P<news>ru)/(?P<search>search)/"),
            TStringBuf("q")
        ),
        // regnum.ru
        TSESimpleRegexp(
            TStringBuf("(?:www\\.)?(?P<engine>regnum)(?P<local>\\.)(?P<news>ru)/(?P<search>search)/"),
            TStringBuf("txt")
        ),
        // dni.ru
        TSESimpleRegexp(
            TStringBuf("(?:www\\.)?(?P<engine>dni)(?P<local>\\.)(?P<news>ru)/(?P<search>search)/"),
            TStringBuf("s_string")
        ),
        // utro.ru
        TSESimpleRegexp(
            TStringBuf("(?:www\\.)?(?P<search>search)\\.(?P<engine>utro)(?P<local>\\.)(?P<news>ru)/"),
            TStringBuf("query")
        ),
        // mk.ru
        TSESimpleRegexp(
            TStringBuf("(?:www\\.)?(?P<engine>mk)(?P<local>\\.)(?P<news>ru)/(?P<search>search2)/"),
            TStringBuf("search")
        ),
        // aif.ru
        TSESimpleRegexp(
            TStringBuf("(?:www\\.)?(?P<engine>aif)(?P<local>\\.)(?P<news>ru)/(?P<search>search)"),
            TStringBuf("fake"),
            TStringBuf("q=(?P<query>.*?)\\+site%3Awww\\.aif\\.ru")
        ),
        // rg.ru
        TSESimpleRegexp(
            TStringBuf("(?:www\\.)?search\\.(?P<engine>rg)(?P<local>\\.)(?P<news>ru)/(?P<search>wwwsearch)/"),
            TStringBuf("text")
        ),
        // vedomosti.ru
        TSESimpleRegexp(
            TStringBuf("(?:www\\.)?(?P<engine>vedomosti)(?P<local>\\.)(?P<news>ru)/(?P<search>search)/"),
            TStringBuf("s")
        ),
        // rb.ru
        TSESimpleRegexp(
            TStringBuf("(?:www\\.)?(?P<engine>rb)(?P<local>\\.)(?P<news>ru)/(?P<search>search)/"),
            TStringBuf("search")
        ),
        // trud.ru
        TSESimpleRegexp(
            TStringBuf("(?:www\\.)?(?P<engine>trud)(?P<local>\\.)(?P<news>ru)/(?P<search>search)/"),
            TStringBuf("query")
        ),
        // rin.ru
        TSESimpleRegexp(
            TStringBuf("(?:www\\.)?news\\.(?P<engine>rin)(?P<local>\\.)(?P<news>ru)/(?P<search>search)[/]+(?P<query>[^/]*)/")
        ),
        // izvestia.ru
        TSESimpleRegexp(
            TStringBuf("(?:www\\.)?(?P<engine>izvestia)(?P<local>\\.)(?P<news>ru)/(?P<search>search)"),
            TStringBuf("search")
        ),
        // kommersant.ru
        TSESimpleRegexp(
            TStringBuf("(?:www\\.)?(?P<engine>kommersant)(?P<local>\\.)(?P<news>ru)/(?P<search>Search)/Results"),
            TStringBuf("search_query")
        ),
        // expert.ru
        TSESimpleRegexp(
            TStringBuf("(?:www\\.)?(?P<engine>expert)(?P<local>\\.)(?P<news>ru)/(?P<search>search)/"),
            TStringBuf("search_text")
        ),
        // interfax.ru
        TSESimpleRegexp(
            TStringBuf("(?:www\\.)?(?P<engine>interfax)(?P<local>\\.)(?P<news>ru)/(?P<search>archive).asp"),
            TStringBuf("sw")
        ),
        // echo.msk.ru
        TSESimpleRegexp(
            TStringBuf("(?:www\\.)?(?P<engine>echo\\.msk)(?P<local>\\.)(?P<news>ru)/(?P<search>search)/"),
            TStringBuf("search_cond%5Bquery%5D")
        ),
        // newizv.ru
        TSESimpleRegexp(
            TStringBuf("(?:www\\.)?(?P<engine>newizv)(?P<local>\\.)(?P<news>ru)/(?P<search>search)/"),
            TStringBuf("text")
        ),
        // rbcdaily.ru
        TSESimpleRegexp(
            TStringBuf("(?:www\\.)?(?P<engine>rbcdaily)(?P<local>\\.)(?P<news>ru)/(?P<search>search)/"),
            TStringBuf("query")
        ),
        // argumenti.ru
        TSESimpleRegexp(
            TStringBuf("(?:www\\.)?(?P<engine>argumenti)(?P<local>\\.)(?P<news>ru)/(?P<search>search)"),
            TStringBuf("text")
        ),

        // SOCIAL

        // vk.com
        TSESimpleRegexp(
            TStringBuf("(?:(?:www\\.)?(?P<mobile>m\\.)?(?:[0-9a-z]*\\.)?(?P<fsocial>(?P<engine>vk)\\.(?:com|me))/(?P<search>(?P<local>(?P<music>audio)|(?P<video>video)|(?P<social>search|feed.*q=(?P<query>[^#&]*))))?)"),
            TStringBuf("(?:q|c%5Bq%5D)"),
            TStringBuf("(?P<fake_search>z=[^#&\\?]*)")
        ),
        // facebook.com
        TSESimpleRegexp(
            TStringBuf("(?:www\\.)?(?P<mobile>m\\.)?(?P<engine>facebook)\\.(?P<fsocial>com)/(?P<search>(?P<social>(?P<local>search/results\\.php|findfriends/search|\\#!/search)))?"),
            TStringBuf("(?:q|query)"),
            TStringBuf("type=(?P<people>users)")
        ),
        // twitter.com
        TSESimpleRegexp(
            TStringBuf("(?P<engine>twitter)\\.(?P<fsocial>com)/(?P<search>(?P<social>(?P<local>search)))?"),
            TStringBuf("q")
        ),
        // odnoklassniki.ru
        TSESimpleRegexp(
            TStringBuf("(?:http://)(?:www\\.)?(?P<mobile>m\\.)?(?P<fsocial>(?P<engine>odnoklassniki\\.ru))"),
            TStringBuf("(?P<search>(?P<social>(?<local>st\\.query|st\\.search)))")
        ),
        // my.mail.ru
        TSESimpleRegexp(
            TStringBuf("(?:(?P<mobile>m\\.)?my\\.(?P<engine>mail\\.ru)(?P<fsocial>/)(?P<search>(?P<local>(?P<music>[^/]*/[^/]*/audio\\?search=(?P<query>[^&#?]*))|(?P<video>cgi-bin/video/search\\?q=(?P<query>[^&#?]*))|(?P<social>my/search_people\\?q=(?P<query>[^&#?]*))|(?P<social>my/communities-search\\?q=(?P<query>[^&#?]*))))?)|(?:foto\\.(?P<engine>mail\\.ru)(?P<fsocial>/)(?P<search>(?P<local>(?P<images>cgi-bin/photo/search\\?q=(?P<query>[^&#?]*)))))")
        ),
        // Google+
        TSESimpleRegexp(
            TStringBuf("plus\\.(?P<fsocial>(?P<engine>google))\\.com/u/[0-9]*/(?P<search>(?P<local>(?P<social>s/(?P<query>[^?&#/]*))?))")
        ),
        // LiveJournal.com
        TSESimpleRegexp(
            TStringBuf("(?:www\\.)?(?P<mobile>m\\.)?(?:[-a-z0-9_]+\\.)?(?P<fsocial>(?P<engine>livejournal))\\.com/(?P<search>(?P<local>(?P<social>search)))?"),
            TStringBuf("q")
        ),
        // Я.ру
        TSESimpleRegexp(
            TStringBuf("(?:my|[-0-9a-z_]*)\\.(?P<engine>(?P<fsocial>ya))\\.ru/(?P<search>(?P<local>(?P<social>search_posts\\.xml|search_clubs_by_name\\.xml)))?"),
            TStringBuf("text")
        ),
        // LinkedIn
        TSESimpleRegexp(
            TStringBuf("(?:http://)(?P<mobile>touch)?www\\.(?P<engine>(?P<fsocial>linkedin))\\.com/(?P<search>(?P<local>(?P<social>search/fpsearch|jsearch|csearch|signalinbox/messages/search|search-fe)))?"),
            TStringBuf("keywords")
        ),
        // Mamba
        TSESimpleRegexp(
            TStringBuf("www\\.(?P<engine>(?P<fsocial>mamba))\\.ru")
        ),
        // Sprashivai.ru
        TSESimpleRegexp(
            TStringBuf("(?P<mobile>m\\.)?(?P<engine>(?P<fsocial>sprashivai\\.ru))/(?P<search>(?P<local>(?P<social>search)))?"),
            TStringBuf("q")
        ),
        // Tumblr
        TSESimpleRegexp(
            TStringBuf("www\\.(?P<engine>(?P<fsocial>tumblr))\\.com/(?P<search>(?P<local>(?P<social>tagged/(?P<query>[^&?#/]*))))?")
        ),
        // Myspace
        TSESimpleRegexp(
            TStringBuf("(?P<engine>(?P<fsocial>myspace))\\.com")
        ),

        // MUSIC

        // music.yandex.ru
        TSESimpleRegexp(
            TStringBuf("(?P<music>music)(?P<local>\\.)(?P<engine>yandex)\\.[\\w\\.]{2,}/(?:\\#!/)?(?P<search>search)"),
            TStringBuf("text")
        ),
        // itunes
        TSESimpleRegexp(
            TStringBuf("ax\\.search(?P<local>\\.)(?P<engine>itunes)(?P<music>\\.)apple\\.com/WebObjects/MZSearch\\.woa/wa/(?P<search>search)"),
            TStringBuf("term")
        ),
        // zvooq.ru
        TSESimpleRegexp(
            TStringBuf("(?P<engine>zvooq)(?P<local>\\.)(?P<music>ru)/\\#/(?P<search>search)/"),
            TStringBuf("query")
        ),
        // zvooq.ru
        TSESimpleRegexp(
            TStringBuf("(?P<engine>grooveshark)(?P<local>\\.)(?P<music>com)/\\#!/(?P<search>search)"),
            TStringBuf("q")
        ),
        // lastfm.ru
        TSESimpleRegexp(
            TStringBuf("www\\.(?P<engine>lastfm)(?P<local>\\.)(?P<music>ru)/(?P<search>search)"),
            TStringBuf("q")
        ),
        // prostopleer.com
        TSESimpleRegexp(
            TStringBuf("(?P<engine>prostopleer)(?P<local>\\.)(?P<music>com)/(?P<search>search)"),
            TStringBuf("q")
        ),
        // soundcloud.com
        TSESimpleRegexp(
            TStringBuf("(?P<engine>soundcloud)(?P<local>\\.)(?P<music>com)/(?P<search>search)"),
            TStringBuf("q%5Bfulltext%5D")
        ),
        // muzebra.com
        TSESimpleRegexp(
            TStringBuf("(?P<engine>muzebra)(?P<local>\\.)(?P<music>com)/(?P<search>search)/"),
            TStringBuf("q")
        ),
        // weborama.fm
        TSESimpleRegexp(
            TStringBuf("www\\.(?P<engine>weborama)(?P<local>\\.)(?P<music>fm)/\\#/music/(?P<search>found)/(?P<query>[^/]*)/")
        ),
        // 101.ru
        TSESimpleRegexp(
            TStringBuf("(?P<engine>101)(?P<local>\\.ru)(?P<music>/)\\?an=search_full\\&(?P<search>search)=(?P<query>[^&]*)")
        ),
        // www.pandora.com
        TSESimpleRegexp(
            TStringBuf("www\\.(?P<engine>pandora)(?P<local>\\.)(?P<music>com)/(?P<search>search)/(?P<query>[^/]*)")
        ),
        // www.deezer.com
        TSESimpleRegexp(
            TStringBuf("www\\.(?P<engine>deezer)(?P<local>\\.)(?P<music>com)/[^/]*/(?P<search>search)/(?P<query>[^/]*)")
        ),

        // WORK

        // hh.com
        TSESimpleRegexp(
            TStringBuf("(?P<engine>hh)(?P<local>\\.)(?P<job>ru)/applicant/(?P<search>searchvacancyresult)\\.xml"),
            TStringBuf("text")
        ),
        // www.rabota.ru
        TSESimpleRegexp(
            TStringBuf("www\\.(?P<engine>rabota)(?P<local>\\.)(?P<job>ru)/v3_(?P<search>search)VacancyByParamsResults\\.html"),
            TStringBuf("w")
        ),
        // www.rabota.ru
        TSESimpleRegexp(
            TStringBuf("www\\.(?P<engine>rabota)(?P<local>\\.)(?P<job>ru)/v3_(?P<search>search)VacancyByParamsResults\\.html"),
            TStringBuf("w")
        ),
        // rabota.yandex.ru
        TSESimpleRegexp(
            TStringBuf("(?P<job>rabota)(?P<local>\\.)(?P<engine>yandex)\\.[\\w\\.]{2,}(?:/(?P<search>search))?"),
            TStringBuf("text")
        ),
        // superjob.ru
        TSESimpleRegexp(
            TStringBuf("www\\.(?P<engine>superjob)\\.ru/(?P<job>vacancy)/(?P<local>(?P<search>search))/"),
            TStringBuf("keywords%5B0%5D%5Bkeys%5D")
        ),
        // superjob.ru
        TSESimpleRegexp(
            TStringBuf("www\\.(?P<engine>job\\.ru)/(?P<job>seeker)/(?P<local>job)(?P<search>/)"),
            TStringBuf("q")
        ),

        // TORRENTS
        // rutracker.org
        TSESimpleRegexp(
            TStringBuf("(?P<engine>rutracker\\.org)/(?P<local>forum)/(?P<torrent>tracker)(?P<search>\\.)php"),
            TStringBuf("nm")
        ),
        // rutor.org
        TSESimpleRegexp(
            TStringBuf("(?:www\\.)?(?P<engine>rutor\\.org)(?P<local>/)(?P<torrent>(?P<search>search))/(?:[^/]+/[^/]+/[^/]+/[^/]+/)?(?P<query>[^?#&]*)")
        ),
        // torrentino.com
        TSESimpleRegexp(
            TStringBuf("www\\.(?P<engine>torrentino\\.com)(?P<local>/)(?P<torrent>(?P<search>search))"),
            TStringBuf("search")
        ),
        // fast-torrent.ru
        TSESimpleRegexp(
            TStringBuf("www\\.(?P<engine>fast-torrent\\.ru)(?P<local>/)(?P<torrent>(?P<search>search))/(?P<query>[^/]*)/1\\.html")
        ),
        // tfile.me
        TSESimpleRegexp(
            TStringBuf("(?P<engine>tfile\\.me)/(?P<local>forum)/(?P<torrent>(?P<search>search))"),
            TStringBuf("q")
        ),
        // kinozal.tv
        TSESimpleRegexp(
            TStringBuf("(?P<engine>kinozal\\.tv)/(?P<local>browse)(?P<torrent>\\.)(?P<search>php)"),
            TStringBuf("s")
        ),
        // nnm-club.ru
        TSESimpleRegexp(
            TStringBuf("(?P<engine>nnm-club\\.ru)/(?P<local>forum)/(?P<torrent>(?P<search>search))\\.php"),
            TStringBuf("q")
        ),
        // ex.ua
        TSESimpleRegexp(
            TStringBuf("www\\.(?P<engine>ex\\.ua)(?P<local>/)(?P<torrent>(?P<search>search))"),
            TStringBuf("s")
        ),
        // torrent-games.net
        TSESimpleRegexp(
            TStringBuf("(?P<engine>torrent-games\\.net)(?P<local>/)(?P<torrent>(?P<search>search))/"),
            TStringBuf("q")
        ),
        // my-hit.ru
        TSESimpleRegexp(
            TStringBuf("(?P<engine>my-hit\\.ru)(?P<local>/)(?P<torrent>index)(?P<search>\\.)php"),
            TStringBuf("search_string")
        ),
        // torrentino.ru
        TSESimpleRegexp(
            TStringBuf("www\\.(?P<engine>torrentino\\.ru)(?P<local>/)(?P<torrent>(?P<search>search))"),
            TStringBuf("search")
        ),
        // x-torrents.ru
        TSESimpleRegexp(
            TStringBuf("(?P<engine>x-torrents\\.org)(?P<local>/)(?P<torrent>browse)(?P<search>\\.)php"),
            TStringBuf("search")
        ),
        // torrentfilms.net
        TSESimpleRegexp(
            TStringBuf("(?P<engine>torrentfilms\\.net)(?P<local>/)(?P<torrent>(?P<search>search))\\.html"),
            TStringBuf("s")
        ),
        // seedoff.net
        TSESimpleRegexp(
            TStringBuf("(?P<local>www\\.)(?P<engine>seedoff\\.net)(?P<torrent>(?P<search>/))"),
            TStringBuf("search")
        ),
        // pornoshara.tv
        TSESimpleRegexp(
            TStringBuf("(?P<engine>pornoshara\\.tv)(?P<local>/)(?P<torrent>(?P<search>search))\\.php"),
            TStringBuf("q")
        ),
        // tfile.org
        TSESimpleRegexp(
            TStringBuf("(?P<engine>tfile\\.org)/(?P<local>mdb)/(?P<torrent>(?P<search>search-ext))/go"),
            TStringBuf("text")
        ),
        // torrentszona.com
        TSESimpleRegexp(
            TStringBuf("(?P<engine>torrentszona\\.com)(?P<local>/)(?P<torrent>category)(?P<search>/)"),
            TStringBuf("search")
        ),
        // xxx-tracker.com
        TSESimpleRegexp(
            TStringBuf("(?P<engine>xxx-tracker\\.com)(P<local>/)(?P<torrent>b)(?P<search>\\.)php"),
            TStringBuf("search")
        ),
        // kubalibre.com
        TSESimpleRegexp(
            TStringBuf("(?P<engine>kubalibre\\.com)(?P<local>/)(?P<torrent>browse)(?P<search>\\.)php"),
            TStringBuf("search")
        ),
        // imhonet.ru
        TSESimpleRegexp(
            TStringBuf("(?P<engine>imhonet\\.ru)(?P<local>/)(?P<torrent>(?P<search>search))/"),
            TStringBuf("search")
        ),
        // torrentsmd.com
        TSESimpleRegexp(
            TStringBuf("(?:www\\.)?(?P<engine>torrentsmd\\.com)(?P<local>/)(?P<torrent>(?P<search>search))\\.php"),
            TStringBuf("search_str")
        ),
        // torrnado.ru
        TSESimpleRegexp(
            TStringBuf("www\\.(?P<engine>torrnado\\.ru)(?P<local>/)(?P<torrent>(?P<search>search_new))\\.php"),
            TStringBuf("q")
        ),
        // riper.am
        TSESimpleRegexp(
            TStringBuf("(?P<engine>riper\\.am)(?P<local>/)(?P<torrent>(?P<search>search))\\.php"),
            TStringBuf("keywords")
        ),
        // kinovit.ru
        TSESimpleRegexp(
            TStringBuf("www\\.(?P<engine>kinovit\\.ru)(?P<local>/)(?P<torrent>(?P<search>search))\\.html"),
            TStringBuf("s")
        ),
        // katushka.net
        TSESimpleRegexp(
            TStringBuf("(?P<engine>katushka\\.net)(?P<local>/)(?P<torrent>torrents)(?P<search>/)"),
            TStringBuf("search")
        ),
        // kat.ph
        TSESimpleRegexp(
            TStringBuf("(?P<engine>kat\\.ph)(?P<local>/)(?P<torrent>(?P<search>usearch))/(?P<query>[^/]*)/")
        ),
        // weburg.net
        TSESimpleRegexp(
            TStringBuf("(?P<engine>weburg\\.net)(?P<local>/)(?P<torrent>(?P<search>search))/"),
            TStringBuf("q")
        ),
        // kinokopilka.tv
        TSESimpleRegexp(
            TStringBuf("www\\.(?P<engine>kinokopilka\\.tv)(?P<local>/)(?P<torrent>(?P<search>search))"),
            TStringBuf("q")
        ),
        // filebase.ws
        TSESimpleRegexp(
            TStringBuf("www\\.(?P<engine>filebase\\.ws)(?P<local>/)(?P<torrent>torrents)/(?P<search>search)/"),
            TStringBuf("search")
        ),
        // evrl.to
        TSESimpleRegexp(
            TStringBuf("(?P<engine>evrl\\.to)(?P<local>/)[^/]*/(?P<torrent>(?P<search>search))/"),
            TStringBuf("search")
        ),
        // arenabg.com
        TSESimpleRegexp(
            TStringBuf("0\\.(?P<search>(?P<local>(?P<torrent>(?P<engine>arenabg\\.com))))/torrents/search:(?P<query>[^/]*)")
        ),
        // 4fun.mksat.net
        TSESimpleRegexp(
            TStringBuf("(?P<search>(?P<local>(?P<torrent>(?P<engine>4fun\\.mksat\\.net))))/browse\\.php"),
            TStringBuf("search")
        ),
        // beetor.org
        TSESimpleRegexp(
            TStringBuf("(?P<search>(?P<local>(?P<torrent>(?P<engine>beetor\\.org))))/index\\.php"),
            TStringBuf("s")
        ),
        // extratorrent.com
        TSESimpleRegexp(
            TStringBuf("(?P<search>(?P<local>(?P<torrent>(?P<engine>extratorrent\\.com))))/search/"),
            TStringBuf("search")
        ),
        // isohunt.com
        TSESimpleRegexp(
            TStringBuf("(?P<search>(?P<local>(?P<torrent>(?P<engine>isohunt\\.com))))/torrents/"),
            TStringBuf("ihq")
        ),
        // newtorr.org
        TSESimpleRegexp(
            TStringBuf("(?P<search>(?P<local>(?P<torrent>(?P<engine>newtorr\\.org))))/index\\.php"),
            TStringBuf("search")
        ),
        // rarbg.com
        TSESimpleRegexp(
            TStringBuf("(?P<search>(?P<local>(?P<torrent>(?P<engine>rarbg\\.com))))/torrents\\.php"),
            TStringBuf("search")
        ),
        // tracker.name
        TSESimpleRegexp(
            TStringBuf("(?P<search>(?P<local>(?P<torrent>(?P<engine>tracker\\.name))))/index\\.php"),
            TStringBuf("search")
        ),
        // bigfangroup.org
        TSESimpleRegexp(
            TStringBuf("www\\.(?P<search>(?P<local>(?P<torrent>(?P<engine>bigfangroup\\.org))))/browse.php"),
            TStringBuf("search")
        ),
        // file.lu
        TSESimpleRegexp(
            TStringBuf("www\\.(?P<search>(?P<local>(?P<torrent>(?P<engine>file\\.lu))))/browse/pub/added:desc/(?:\\([^)]*\\))*\\(str:(?P<query>[^.]*)\\)\\.html")
        ),
        // linkomanija.net
        TSESimpleRegexp(
            TStringBuf("www\\.(?P<search>(?P<local>(?P<torrent>(?P<engine>linkomanija\\.net))))/browse\\.php"),
            TStringBuf("search")
        ),
        // omgtorrent.com
        TSESimpleRegexp(
            TStringBuf("www\\.(?P<search>(?P<local>(?P<torrent>(?P<engine>omgtorrent\\.com))))/recherche/"),
            TStringBuf("query")
        ),
       // smartorrent.com
        TSESimpleRegexp(
            TStringBuf("www\\.(?P<search>(?P<local>(?P<torrent>(?P<engine>smartorrent\\.com))))/"),
            TStringBuf("term")
        ),
        // t411.me
        TSESimpleRegexp(
            TStringBuf("www\\.(?P<search>(?P<local>(?P<torrent>(?P<engine>t411\\.me))))/torrents/search/"),
            TStringBuf("search")
        ),
        // torrent.ai
        TSESimpleRegexp(
            TStringBuf("www\\.(?P<search>(?P<local>(?P<torrent>(?P<engine>torrent\\.ai))))/v2/torrents-search\\.php"),
            TStringBuf("search")
        ),
        // torrentdownloads.me
        TSESimpleRegexp(
            TStringBuf("www\\.(?P<search>(?P<local>(?P<torrent>(?P<engine>torrentdownloads\\.me))))/search/"),
            TStringBuf("search")
        ),
        // torrentreactor.net
        TSESimpleRegexp(
            TStringBuf("www\\.(?P<search>(?P<local>(?P<torrent>(?P<engine>torrentreactor\\.net))))/torrent-search/(?P<query>[^/]*)")
        ),
        // zamunda.net
        TSESimpleRegexp(
            TStringBuf("(?P<search>(?P<local>(?P<torrent>(?P<engine>zamunda\\.net))))/browse\\.php"),
            TStringBuf("search")
        ),
        // retre.org
        TSESimpleRegexp(
            TStringBuf("(?P<search>(?P<local>(?P<torrent>(?P<engine>retre\\.org))))"),
            TStringBuf("search")
        ),
        // mix.sibnet.ru
        TSESimpleRegexp(
            TStringBuf("(?P<search>(?P<local>(?P<torrent>(?P<engine>mix\\.sibnet\\.ru))))/[^/]*/search/"),
            TStringBuf("keyword")
        ),

        // VIDEO HOSTINGS
        // myvi
        TSESimpleRegexp(
            TStringBuf("www\\.(?P<engine>myvi)(?P<video>\\.)(?P<local>ru)/(?P<search>search)"),
            TStringBuf("text")
        ),
        // rutube
        TSESimpleRegexp(
            TStringBuf("(?P<engine>rutube)(?P<video>\\.)(?P<local>ru)/(?P<search>search)/"),
            TStringBuf("query")
        ),
        // kinostok
        TSESimpleRegexp(
            TStringBuf("(?P<engine>kinostok)(?P<video>\\.)(?P<local>tv)/(?P<search>search)/basic/(?P<query>[^/]*)/")
        ),
        // namba
        TSESimpleRegexp(
            TStringBuf("(?P<engine>namba)(?P<video>\\.)(?P<local>net)/\\#!/(?P<search>search)/video/(?P<query>.*)")
        ),
        // bigcinema
        TSESimpleRegexp(
            TStringBuf("(?P<engine>bigcinema)(?P<video>\\.)(?P<local>tv)/(?P<search>search)/topics"),
            TStringBuf("q")
        ),
        // kaztube
        TSESimpleRegexp(
            TStringBuf("(?P<engine>kaztube)(?P<search>\\.)(?P<local>kz)/ru/(?P<video>video)"),
            TStringBuf("s")
        ),
        // kiwi.kz
        TSESimpleRegexp(
            TStringBuf("(?:test\\.)?(?P<engine>kiwi\\.kz)(?P<video>/)(?P<local>(?P<search>search))/"),
            TStringBuf("keyword")
        ),
        // now.ru
        TSESimpleRegexp(
            TStringBuf("www\\.(?P<engine>now\\.ru)(?P<video>/)(?P<local>(?P<search>search))"),
            TStringBuf("q")
        ),
        // vimeo
        TSESimpleRegexp(
            TStringBuf("(?P<engine>vimeo)(?P<video>\\.)(?P<local>com)/(?P<search>search)"),
            TStringBuf("q")
        ),
        // akilli
        TSESimpleRegexp(
            TStringBuf("www\\.(?P<engine>akilli)(?P<video>\\.)(?P<local>tv)/(?P<search>search)\\.aspx"),
            TStringBuf("q")
        ),
        // dailymotion
        TSESimpleRegexp(
            TStringBuf("www\\.(?P<engine>dailymotion)(?P<video>\\.)(?P<local>com)/[^/]+/relevance/(?P<search>search)/(?P<query>[^/]*)/")
        ),
        // izlemex
        TSESimpleRegexp(
            TStringBuf("(?:src|www)(?P<search>\\.)(?P<engine>izlemex)(?P<video>\\.)(?P<local>org)/"),
            TStringBuf("q")
        ),
        // kuzu.tv
        TSESimpleRegexp(
            TStringBuf("www\\.(?P<engine>kuzu)(?P<video>\\.)(?P<local>tv)/(?P<search>arama)"),
            TStringBuf("q")
        ),
        // tekniktv
        TSESimpleRegexp(
            TStringBuf("www\\.(?P<engine>tekniktv)(?P<video>\\.)(?P<local>com)/(?P<search>ara)\\.php"),
            TStringBuf("q")
        ),
        // izlesene
        TSESimpleRegexp(
            TStringBuf("(?P<search>search)\\.(?P<engine>izlesene)(?P<video>\\.)(?P<local>com)/"),
            TStringBuf("kelime")
        ),
        // ukrhome
        TSESimpleRegexp(
            TStringBuf("(?P<video>video)\\.(?P<engine>ukrhome)(?P<local>\\.net)/(?P<search>search)/"),
            TStringBuf("q")
        ),

        // MAIL
        // yandex
        TSESimpleRegexp(
            TStringBuf("(?P<mail>mail)\\.(?P<engine>yandex)\\.[\\w\\.]{2,}/(?:(?:neo2|lite)/(?P<search>(?P<local>\\#search/request=(?P<query>[^&#?]*)))?)?")
        ),
        // gmail.com
        TSESimpleRegexp(
            TStringBuf("(?P<mail>mail)\\.(?P<engine>google)\\.com/mail(?P<search>(?P<local>/[^#]*\\#search/(?P<query>[^/]*)))?")
        ),
        // mail.ru
        TSESimpleRegexp(
            TStringBuf("(?:(?P<mail>e)|(?P<mobile>(?P<mail>touch)))\\.(?P<engine>mail\\.ru)/cgi-bin/msglist\\?[^#]*\\#/(?P<search>search(?P<local>/\\?search=[^&]*&q_query=(?P<query>[^&]*)))?")
        ),
        // rambler
        TSESimpleRegexp(
            TStringBuf("(?P<mail>mail)(?P<mobile>-pda)?\\.(?P<engine>rambler)\\.ru/(?P<search>(?P<local>\\#/folder/[^/]*/search=(?P<query>[^/]*)))?")
        ),
        // yahoo
        TSESimpleRegexp(
            TStringBuf("(?:(?:http://)(?:[^.]+\\.(?P<mail>mail)\\.(?P<engine>yahoo)\\.com|(?P<mobile>m\\.)(?P<engine>yahoo)\\.com/w/(?P<mail>ygo-mail)))")
        ),
        // bigmir.net
        TSESimpleRegexp(
            TStringBuf("(?:http://)(?P<mail>mbox)\\.(?P<engine>bigmir)\\.net"),
            TStringBuf("(?P<search>(?P<local>text))")
        ),
        // qip
        TSESimpleRegexp(
            TStringBuf("(?P<mobile>m\\.)?(?P<mail>mail)\\.(?P<engine>qip)\\.ru/(?P<search>(?P<local>search))?"),
            TStringBuf("search")
        ),
        // i.ua
        TSESimpleRegexp(
            TStringBuf("(?:http://)(?P<mail>mbox2)\\.(?P<engine>i\\.ua)"),
            TStringBuf("(?<search>(?<local>text))")
        ),
        // km.ru
        TSESimpleRegexp(
            TStringBuf("(?:http://)(?P<mail>mail)\\.(?P<engine>km\\.ru)/(?P<search>(?P<local>folder/messagesearch.htm))?"),
            TStringBuf("_subject")
        ),
        // ngs.ru
        TSESimpleRegexp(
            TStringBuf("(?P<mail>mail)\\.(?P<engine>ngs\\.ru)")
        ),
        // live.com
        TSESimpleRegexp(
            TStringBuf("snt[0-9]*\\.(?P<mail>mail)\\.(?P<engine>live\\.com)")
        ),

        // ADVERT_SERP

        // yandex
        TSESimpleRegexp(
            TStringBuf("(?:(?P<adv_serp>yabs)\\.(?P<engine>yandex)\\.[\\w\\.]{2,}/count|(?P<adv_serp>market-click2)\\.(?P<engine>yandex)\\.[\\w\\.]{2,}/redir)"),
            TStringBuf("q")
        ),
        // google
        TSESimpleRegexp(
            TStringBuf("www\\.(?P<engine>google)\\.(?:[\\w]{2}|co\\.[\\w]{2}|com(?:\\.[\\w]{2})?)/(?P<adv_serp>aclk)(?:\\?.*q=(?P<query>[^&]*))?")
        ),

        // ADVERT_WEB

        // yandex
        TSESimpleRegexp(
            TStringBuf("(?P<adv_web>(?:an|awaps))\\.(?P<engine>yandex)\\.[\\w\\.]{2,}/")
        ),
        // google
        TSESimpleRegexp(
            TStringBuf("(?:[^.]+\\.)+(?P<engine>doubleclick)\\.(?P<adv_web>net)/")
        ),
        TSESimpleRegexp(
            TStringBuf("(?P<adv_web>tpc)\\.(?P<engine>google)syndication\\.com/")
        ),
        TSESimpleRegexp(
            TStringBuf("(?:www\\.)?(?P<engine>google)(?P<adv_web>adservices)\\.com/")
        ),
        // bannersbroker.com
        TSESimpleRegexp(
            TStringBuf("(?P<engine>bannersbroker\\.com)/incentives/(?P<adv_web>click_adspot)")
        ),
        // adcash.com
        TSESimpleRegexp(
            TStringBuf("www\\.(?P<engine>adcash\\.com)/script/(?P<adv_web>pop_packcpm\\.php)")
        ),
        // luxup.ru
        TSESimpleRegexp(
            TStringBuf("(?P<engine>luxup\\.ru)/(?P<adv_web>gg|gol)")
        ),
        // adonweb.ru
        TSESimpleRegexp(
            TStringBuf("wu\\.(?P<engine>adonweb\\.ru)/(?P<adv_web>adv_clk_redirect\\.php|best\\.php)")
        ),
        // advertlink.ru
        TSESimpleRegexp(
            TStringBuf("(?P<engine>advertlink\\.ru)/(?P<adv_web>vmclick\\.php)")
        ),
        // exoclick.com
        TSESimpleRegexp(
            TStringBuf("syndication\\.(?P<engine>exoclick\\.com)/(?P<adv_web>splash\\.php)")
        ),
        // recreativ.ru
        TSESimpleRegexp(
            TStringBuf("clk\\.(?P<engine>recreativ\\.ru)/(?P<adv_web>go\\.php)")
        ),
        // ruclicks.com
        TSESimpleRegexp(
            TStringBuf("www\\.(?P<engine>ruclicks\\.com)/(?P<adv_web>out|in)")
        ),
        // ladycenter.ru
        TSESimpleRegexp(
            TStringBuf("(?:www\\.)?(?P<engine>ladycenter\\.ru)/(?P<adv_web>pnews|rnews|tnews)")
        ),
        // medialand.ru
        TSESimpleRegexp(
            TStringBuf("engine\\.[^.]+\\.(?P<engine>medialand\\.ru)/(?P<adv_web>reference)")
        ),
        // admitad.com
        TSESimpleRegexp(
            TStringBuf("ad\\.(?P<engine>admitad\\.com)/(?P<adv_web>goto)")
        ),
        // rmbn.net
        TSESimpleRegexp(
            TStringBuf("(?P<adv_web>post)\\.(?P<engine>rmbn\\.net)/")
        ),
        // tbn.ru
        TSESimpleRegexp(
            TStringBuf("(?P<adv_web>cbn2?)\\.(?P<engine>tbn\\.ru)/")
        ),
        // adnetwork.pro
        TSESimpleRegexp(
            TStringBuf("news\\.(?P<engine>adnetwork\\.pro)/(?P<adv_web>click\\.cgi)")
        ),
        // yadro.ru
        TSESimpleRegexp(
            TStringBuf("(?P<adv_web>sticker)\\.(?P<engine>yadro\\.ru)/")
        ),
        // adwolf.ru
        TSESimpleRegexp(
            TStringBuf("[-a-z0-9]+\\.(?P<engine>adwolf\\.ru)/[0-9]+/(?P<adv_web>mediaplus)/")
        ),
        // magna.ru
        TSESimpleRegexp(
            TStringBuf("(?P<adv_web>feed)\\.(?P<engine>magna\\.ru)/")
        ),
        // goodadvert.ru
        TSESimpleRegexp(
            TStringBuf("(?P<adv_web>engine)\\.(?P<engine>goodadvert\\.ru)/r")
        ),
        // adriver.ru
        TSESimpleRegexp(
            TStringBuf("(?P<adv_web>ad)\\.(?P<engine>adriver\\.ru)/cgi-bin/click\\.cgi")
        ),
        // rambler
        TSESimpleRegexp(
            TStringBuf("(?P<adv_web>ad)\\.(?P<engine>rambler)\\.ru/roff/ban\\.clk")
        ),
        // adfox.ru
        TSESimpleRegexp(
            TStringBuf("(?P<adv_web>ads)\\.(?P<engine>adfox\\.ru)/[0-9]+/goLink")
        ),
        // adblender.ru
        TSESimpleRegexp(
            TStringBuf("(?P<adv_web>bn)\\.(?P<engine>adblender\\.ru)/url")
        ),
        // directadvert.ru
        TSESimpleRegexp(
            TStringBuf("code\\.(?P<engine>directadvert\\.ru)/(?P<adv_web>click)")
        ),
        // gameleads.ru
        TSESimpleRegexp(
            TStringBuf("(?P<engine>gameleads\\.ru)/(?P<adv_web>redirect\\.php)")
        ),
        // cityads.ru
        TSESimpleRegexp(
            TStringBuf("www\\.(?P<engine>cityads\\.ru)/(?P<adv_web>click-)")
        ),
        // marketgid.com
        TSESimpleRegexp(
            TStringBuf("www\\.(?P<engine>marketgid\\.com)/(?P<adv_web>pnews)")
        ),
        // buysellads.com
        TSESimpleRegexp(
            TStringBuf("stats\\.(?P<engine>buysellads\\.com)/(?P<adv_web>click\\.go)")
        ),
        // begun.ru
        TSESimpleRegexp(
            TStringBuf("(?P<adv_web>click02)\\.(?P<engine>begun\\.ru)/click\\.jsp")
        ),
        // kavanga.ru
        TSESimpleRegexp(
            TStringBuf("(?P<adv_web>b)\\.(?P<engine>kavanga\\.ru)/")
        ),
        // yieldmanager.com
        TSESimpleRegexp(
            TStringBuf("(?P<adv_web>ad)\\.(?P<engine>yieldmanager\\.com)/")
        ),
        // adnxs.com
        TSESimpleRegexp(
            TStringBuf("ams1\\.ib\\.(?P<engine>adnxs\\.com)/(?P<adv_web>click|pop)")
        ),
        // dt00.net
        TSESimpleRegexp(
            TStringBuf("(?P<adv_web>mg)\\.(?P<engine>dt00\\.net)")
        ),
        // Criteo.
        TSESimpleRegexp(
            TStringBuf("(?:[^.]*\\.)*(?P<engine>criteo)(?P<adv_web>\\.com)/delivery/ck.php")
        ),

        // Popular porno hostings
        TSESimpleRegexp(
            TStringBuf("www\\.(?P<engine>sex\\.com)/(?P<search>search)(?P<local>/)(?P<video>pin)"),
            TStringBuf("query")
        ),
        TSESimpleRegexp(
            TStringBuf("(?P<search>www\\.)(?P<engine>redtube)(?P<video>\\.)(?P<local>com)/"),
            TStringBuf("search")
        ),
        TSESimpleRegexp(
            TStringBuf("www\\.(?P<engine>pornhub)(?P<local>\\.com)/(?P<video>video)/(?P<search>search)"),
            TStringBuf("search")
        ),
        TSESimpleRegexp(
            TStringBuf("www\\.(?P<engine>porn\\.com)/(?P<search>search)(?P<local>\\.)(?P<video>html)"),
            TStringBuf("q")
        ),
        TSESimpleRegexp(
            TStringBuf("www\\.(?P<engine>porntube)(?P<local>\\.)(?P<video>com)/(?P<search>search)"),
            TStringBuf("q")
        ),
        TSESimpleRegexp(
            TStringBuf("(?P<engine>xhamster)(?P<local>\\.)(?P<video>com)/(?P<search>search)\\.php"),
            TStringBuf("q")
        ),
        TSESimpleRegexp(
            TStringBuf("www\\.(?P<engine>xvideos)(?P<local>\\.)(?P<search>com)(?P<video>/)"),
            TStringBuf("k")
        ),
        TSESimpleRegexp(
            TStringBuf("www\\.(?P<engine>youporn)(?P<local>\\.)(?P<video>com)/(?P<search>search)/"),
            TStringBuf("query")
        ),
        TSESimpleRegexp(
            TStringBuf("www\\.(?P<engine>tube8)(?P<local>\\.)(?P<video>com)/(?P<search>searches)\\.html"),
            TStringBuf("q")
        ),
        TSESimpleRegexp(
            TStringBuf("www\\.(?P<engine>xnxx)(?P<local>\\.)(?P<video>com)(?P<search>/)"),
            TStringBuf("k")
        ),
        TSESimpleRegexp(
            TStringBuf("www\\.(?P<engine>xnxx)(?P<local>\\.)(?P<video>com)(?P<search>/)"),
            TStringBuf("k")
        ),
        TSESimpleRegexp(
            TStringBuf("www\\.(?P<engine>pornmd)(?P<search>\\.)com/(?P<video>straight|gay|shemale)/(?P<query>[^/#?&]*)"),
            TStringBuf("fake")
        ),
        // sputnik.ru
        TSESimpleRegexp(
            TStringBuf("(?:www|(?P<news>news)|(?P<images>pics)|(?P<video>video))\\.(?P<engine>sputnik\\.ru)/(?P<search>search)"),
            TStringBuf("q")
        ),
        TSESimpleRegexp(
            TStringBuf("(?P<maps>maps)\\.(?P<engine>sputnik\\.ru)(?P<search>/)"),
            TStringBuf("q")
        ),
        // istella.it
        TSESimpleRegexp(
            TStringBuf("(?:(?:(?P<images>image)|(?P<video>video)|(?P<news>news)|(?P<maps>mappe))\\.)?(?P<engine>istella).it/(?P<search>search)/"),
            TStringBuf("key")
        ),
        // Avito.ru
        TSESimpleRegexp(
            TStringBuf("(?:www\\.)?(?P<mobile>m\\.)?(?P<engine>avito)\\.(?P<search>ru)(?P<com>/)[^?]*"),
            TStringBuf("q")
        ),
        // Aliexpress.com
        TSESimpleRegexp(
            TStringBuf("[\\w]+\\.(?P<engine>aliexpress)(?P<search>.)(?P<com>com)/"),
            TStringBuf("SearchText")
        ),
        // ozon.ru
        TSESimpleRegexp(
            TStringBuf("www\\.(?P<engine>ozon)(?P<search>\\.ru)(?P<com>/)"),
            TStringBuf("text")
        ),
        // wildberries
        TSESimpleRegexp(
            TStringBuf("www\\.(?P<engine>wildberries)(?P<search>\\.)(?P<com>ru)/([^?&#/]*/)+search\\.aspx"),
            TStringBuf("search")
        ),
        // lamoda.ru
        TSESimpleRegexp(
            TStringBuf("www\\.(?P<engine>lamoda)\\.(?P<search>ru)(?P<com>/)(?:catalog)?search/result/"),
            TStringBuf("q")
        ),
        // ulmart.ru
        TSESimpleRegexp(
            TStringBuf("www\\.(?P<engine>ulmart)(?P<com>\\.ru)/(?P<search>search)"),
            TStringBuf("string")
        ),
        // asos.com
        TSESimpleRegexp(
            TStringBuf("www\\.(?P<engine>asos)(?P<com>\\.com)/[^/]*/(?P<search>search)/[^?&#]*"),
            TStringBuf("q")
        ),
        // 003.ru
        TSESimpleRegexp(
            TStringBuf("www\\.(?P<engine>003)(?P<search>\\.ru)(?P<com>/)"),
            TStringBuf("find")
        ),
        // booking.com
        TSESimpleRegexp(
            TStringBuf("www\\.(?P<engine>booking)(?P<com>\\.com)/(?P<search>searchresults)\\.[^\\.]+\\.html"),
            TStringBuf("ss")
        ),
        // afisha.ru
        // TODO: make it CULTURE, not COMmerce!
        TSESimpleRegexp(
            TStringBuf("www\\.(?P<engine>afisha)(?P<com>\\.ru)/(?P<search>Search)/"),
            TStringBuf("Search_str")
        ),
    };

    static TSquareRegexRegion::value_type AllRegexpsSquareArray[] = {
        RagelizedRegexpsArray,
        DefaultSearchRegexpsArray,
    };
    static const TSquareRegexRegion AllRegexps(AllRegexpsSquareArray);

    static TSquareRegexRegion::value_type NotRagelizedSquareArray[] = {
        DefaultSearchRegexpsArray,
    };
    static const TSquareRegexRegion NotRagelizedRegexps(NotRagelizedSquareArray);


    const TSquareRegexRegion& GetDefaultSearchRegexps() {
        return AllRegexps;
    }

    const TSquareRegexRegion& GetNotRagelizedRegexps() {
        return NotRagelizedRegexps;
    }
};
