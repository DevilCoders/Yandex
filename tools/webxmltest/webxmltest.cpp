#include <fstream>

#include <util/string/util.h>
#include <util/generic/string.h>
#include <library/cpp/http/fetch/http_digest.h>
#include <library/cpp/uri/http_url.h>
#include <library/cpp/digest/md5/md5.h>
#include <util/network/socket.h>
#include <util/network/hostip.h>

#include <yweb/webxml/webxml.h>
#include <yweb/webxml/rssparse.h>
#include <yweb/webxml/friends_parse.h>

#include "trans_str.h"

/********************************************************/
/********************************************************/
template <>
inline void Out<wxXMLLoader::wxStatus>(IOutputStream& o, wxXMLLoader::wxStatus t) {
    o.Write(int(t));
}

/******************************************************/
class TestBlogRSSHandler: public blogRSSHandler
{
  protected:
        void   putItem();

  public:
        TestBlogRSSHandler(wxXMLLoader*        owner,
                           const char*         url,
                           int                 line_limit):
            blogRSSHandler(owner, url, false, line_limit,
                           2048, 4096, 65536)
        {}

        ~TestBlogRSSHandler() override
        {}

        bool closeItem (bool finished) override
        {
            if(!finished)
                Cerr << "Unfinished item was dropped" << Endl;
            else
                putItem();
            return blogRSSHandler::closeItem(finished);
        }

        void reportChannel();

        static int doTest(const char* recogn_dict,
                          const char* tmp_file,
                          int         line_limit = 60);
};

/******************************************************/
static void logLoad(const char* msg)
{
    Cerr << msg;
}

/******************************************************/
int TestBlogRSSHandler::doTest
    (const char* recogn_dict,
     const char* tmp_file,
     int         line_limit)
{
    wxXMLLoader loader;

    wxXMLLoader::wxStatus ret = loader.init
        (recogn_dict,
         2, nullptr, nullptr, logLoad);

    if(ret)
    {
        Cerr << "Bad initialization: " << ret << Endl;
        //return 1;
    }

    wxXMLLoadInstructions instr;

    TestBlogRSSHandler blog(&loader, "Test RSS parse", line_limit);
    blog.setRefURL(tmp_file);

    Cout << "<blog >\n";

    ret = blog.parse (tmp_file, instr);

    if(ret)
    {
        Cerr << "\nBad parse: " << ret << Endl;
        return 1;
    }

    blog.reportChannel();

    Cout << "</blog>" << Endl;

    return 0;
}

/******************************************************/
void   TestBlogRSSHandler::putItem()
{
    sanitizeItem(true);

    Cout << "  <item>\n"
         << "   <link>"  << EncodeXML   (mItem.mLink.c_str()).c_str()
         << "</link>\n"
         << "   <guid>"  << EncodeXML   (mItem.mGuid.c_str()).c_str()
         << "</guid>\n"
         << "   <title>"
         << EncodeXML   (mItem.mTitle.c_str()).c_str()
         << "</title>\n"
         << "   <descr base='" << mItem.mDescriptionBase.c_str() << "'>"
         << EncodeXML   (mItem.mDescription.c_str()).c_str()
         << "</descr>\n"
         << "   <pubDate>" << EncodeXMLString (mItem.mPubDate.date8601()).c_str()
         << "</pubDate>\n"
         << "   <author>" << EncodeXML  (mItem.mAuthor.c_str()).c_str()
         << "</author>\n"
         << "   <comments>" << EncodeXML  (mItem.mComments.c_str()).c_str()
         << "</comments>\n"
         << "   <commentrss>" << EncodeXML  (mItem.mCommentRss.c_str()).c_str()
         << "</commentrss>\n";

    if(!mItem.mCategories.empty())
    {
        Cout << "   <categories>\n";
        for(TVector<TString>::iterator it = mItem.mCategories.begin();
            it!=mItem.mCategories.end(); ++it)
        {
            Cout << "    <category>" << EncodeXML (it->c_str())
                 << "</category>\n";
        }
        Cout << "   </categories>\n";
    }
    if(!!mItem.mMood)
        Cout << "   <mood>" << EncodeXML(mItem.mMood.c_str())
                << "</mood>\n";
    if(!!mItem.mMusic)
        Cout << "   <music>" << EncodeXML(mItem.mMusic.c_str())
                << "</music>\n";

    if(!mItem.mEnclosures.empty())
    {
        for(TVector<TString>::iterator it = mItem.mEnclosures.begin();
            it!=mItem.mEnclosures.end(); ++it)
        {
            if(it->c_str()[0])
                Cout << "   " << it->c_str() << "\n";
        }
    }

    if(mItem.mPubDate.timestamp())
    {
        Cout << "   <it-time>" << mItem.mPubDate.timestamp() << "</it-time>\n";
    }

    Cout << "  </item>\n";
}

/******************************************************/
void   TestBlogRSSHandler::reportChannel()
{
    sanitizeChannel();
    Cout << " <noindex>" << mChannel.mNoIndex
         << "</noindex>\n"
         << " <title>"
         << EncodeXML(mChannel.mTitle.c_str()).c_str()
         << "</title>\n"
         << " <descr base='" << mChannel.mDescriptionBase.c_str() << "'>"
         << EncodeXML(mChannel.mDescription.c_str()).c_str()
         << "</descr>\n"
         << " <linkHTML>" << EncodeXML(mChannel.mLinkHTML.c_str()).c_str()
         << "</linkHTML>\n"
         << " <pubDate>" << EncodeXMLString (mChannel.mPubDate.date8601()).c_str()
         << "</pubDate>\n"
         << " <imgLink>" << EncodeXML(mChannel.mImageLink.c_str()).c_str()
         << "</imgLink>\n"
         << " <imgHeight>" << EncodeXML(mChannel.mImageHeight.c_str()).c_str()
         << "</imgHeight>\n"
         << " <imgWidth>" << EncodeXML(mChannel.mImageWidth.c_str()).c_str()
         << "</imgWidth>\n";
}

/******************************************************/
/******************************************************/
class TestLinkTransformer: public tsStringTransformer
{
  public:
    void Log(const char* msg) override
    {
        Cerr<< "LOG TestLinkTransformer: " << msg << Endl;
    }

};
/********************************************************/
class TestFriendsCollectionHandler: public friendsCollectionHandler
{
    protected:
        TestLinkTransformer mTransformer;

    public:
        TestFriendsCollectionHandler(wxXMLLoader& owner,
                                     const char*  linkRSSConfig):
            friendsCollectionHandler(&owner, 2048),
            mTransformer()
        {
            mTransformer.prepare(linkRSSConfig, false);

        }

        ~TestFriendsCollectionHandler() override{}

        void setGroup        (TString& name) override;
        void addFriendByRSS  (TString& name, TString& linkRSS) override;
        void addFriendByHTML (TString& name, TString& linkHTML) override;

        static int doTest(const char* recogn_dict,
                          const char* linkRSSConfig,
                          const char* the_file);
};

/******************************************************/
void TestFriendsCollectionHandler::setGroup
    (TString& name)
{
    Cout << "GROUP: " << name.c_str() << Endl;
}

/********************************************************/
void TestFriendsCollectionHandler::addFriendByRSS
    (TString& name, TString& linkRSS)
{
    TString linkRSSCopy = linkRSS;

    while(mTransformer.process(linkRSS))
    {
        if(strstr(linkRSS.c_str(), "/%/") != nullptr)
            continue;
        break;
    }

    THttpURL  pURL;
    if(!pURL.Parse(linkRSS,
                   THttpURL::FeatureAuthSupported|
                   THttpURL::FeaturesNormalizeSet) &&
       pURL.IsValidAbs())
    {
        TString u = pURL.PrintS();
        Cout << "ADD_RSS: " << name << " URL=" << u.c_str() << Endl;
    }
    else
    {
        Cout << "BAD_RSS: URL=" << linkRSSCopy.c_str() << Endl;
    }
}

/********************************************************/
void TestFriendsCollectionHandler::addFriendByHTML
    (TString& name, TString& linkHTML)
{
    THttpURL  pURL;
    if(!pURL.Parse(linkHTML,
                   THttpURL::FeatureAuthSupported|
                   THttpURL::FeaturesNormalizeSet) &&
       pURL.IsValidAbs())
    {
        TString u = pURL.PrintS();
        Cout << "ADD_HTML: " << name << " URL=" << u.c_str() << Endl;
    }
    else
    {
        Cout << "BAD_HTML: URL=" << linkHTML.c_str() << Endl;
    }
}

/******************************************************/
int TestFriendsCollectionHandler::doTest
    (const char* recogn_dict,
     const char* linkRSSConfig,
     const char* the_file)
{
    wxXMLLoader loader;

    wxXMLLoader::wxStatus ret = loader.init
        (recogn_dict,
         2, nullptr, nullptr, logLoad);

    if(ret)
    {
        Cerr << "Bad initialization: " << ret << Endl;
        //return 1;
    }

    TestFriendsCollectionHandler FR(loader, linkRSSConfig);

    wxXMLLoadInstructions instr;

    ret = FR.parse(the_file, instr);

    if(FR.getSeeAlso())
    {
        Cout << "SEE_ALSO: " << FR.getSeeAlso() << Endl;
    }

    if(ret != wxXMLLoader::WX_OK)
    {
        Cerr << "\nBad parse: " << ret << Endl;
        return 1;
    }

    Cout << "DONE: OK" << Endl;
    return 0;
}

/******************************************************/
/******************************************************/
static char sCorrectAuthorization[]="Authorization: Digest username=\"Mufasa\", realm=\"testrealm@host.com\", nonce=\"dcd98b7102dd2f0e8b11d0f600bfb0c093\", uri=\"/dir/index.html\", algorithm=MD5, qop=auth, nc=00000001, cnonce=\"0a4f113b\", response=\"6629fae49393a05397450978507c4ef1\", opaque=\"5ccc069c403ebaf9f0171e9517f40e41\"\r\n";


/******************************************************/
static int tstURL(const char* url, const char* the_file, bool check_ip)
{
    wxXMLLoader loader;

    wxXMLLoader::wxStatus ret = loader.init
        (nullptr, 2, nullptr, nullptr);

    if(ret)
    {
        Cerr << "Bad initialization: " << ret << Endl;
        //return 1;
    }

    TString the_url = url;
    wxXMLLoadInstructions instr;
    instr.mOutURL                 = &the_url;
    ui32 ip = 0;

    if(check_ip)
    {
        Cerr << "Trying IP for " << url << "... " ;
        THttpURL tmpURL;
        if(!tmpURL.Parse(url,
                         THttpURL::FeatureAuthSupported |
                         THttpURL::FeatureSchemeKnown   |
                         THttpURL::FeaturesNormalizeSet) )
        {
            TString h =  tmpURL.Get(THttpURL::FieldHost);
            NResolver::GetHostIP(h.c_str(), &ip);
        }
        if(ip)
        {
            Cerr << "Got: " << ip << Endl;
        }
        else
        {
            Cerr << "Failure!" << Endl;
        }
    }

    ret = loader.download(url, the_file, instr, ip);


    if(ret)
    {
        Cerr << "Bad download: " << ret << Endl;
    }

    if(the_url!=url)
    {
        Cerr << "Redirected to: " << the_url.c_str() << Endl;
    }

    return ret;
}

/******************************************************/
static int tstDigest()
{
    THttpAuthHeader H;
    H.Init();
    H.base = "http://www.nowhere.org/dir/index.html";
    H.use_auth     = true;
    H.realm  = strdup("testrealm@host.com");
    H.opaque = strdup("5ccc069c403ebaf9f0171e9517f40e41");
    H.nonce  = strdup("dcd98b7102dd2f0e8b11d0f600bfb0c093");
    H.qop_auth = true;

    httpDigestHandler D;
    D.setAuthorization("Mufasa", "Circle Of Life");

    if(!D.processHeader(&H, "/dir/index.html", "GET", "0a4f113b"))
    {
        Cout << "Bad process" << Endl;
        return 1;
    }

    if(strcmp(sCorrectAuthorization, D.getHeaderInstruction()))
    {
        Cout << "Bad authorization:" << Endl
             << "**Produced:" << Endl
             << D.getHeaderInstruction() << Endl
             << "**Should be:" << Endl
             << sCorrectAuthorization << Endl;
        return 1;
    }
    Cout << "Digest: OK" << Endl;
    return 0;
}

/******************************************************/
/******************************************************/
static int tstMD5(const char* code)
{
    MD5 ctx;
    char key[33];
    ctx.Init();

    //ctx.Update( (const unsigned char*)(code), strlen(code) );

    const unsigned char* cd = (const unsigned char*)(code);
    int d = 5;

    for(int l = strlen(code);l>0; l-=d, cd+=d)
    {
        if(l<d)
            d=l;
        ctx.Update(cd, d);
    }

    ctx.End(key);

    Cout << "MD5(\"" << code << "\")=\"" << key << "\"" << Endl;
    return 0;
}

/******************************************************/
/******************************************************/
static int usage(char* prg)
{
    Cerr << "Usage: \n"
         << "> " << prg << " modes [args]\n"
         << " current modes:\n"
         << " url <url> <the_file>\n"
         << " url_ip <url> <the_file>\n"
         << " rss <path to recogn.dict> <the_file> [line_limit]\n"
         << " friends <path to recogn.dict> <path to linkRSS> <the_file>\n"
         << " digest\n"
         << " md5 <sequence>\n"
         << Endl;
    return 1;
}

/******************************************************/
/******************************************************/
int main(int argc, char* argv[])
{
    if(argc<2)
        return usage(argv[0]);

    if(!strcmp("digest", argv[1]) && argc==2)
    {
        return tstDigest();
    }

    if(!strcmp("md5", argv[1]) && argc==3)
    {
        return tstMD5(argv[2]);
    }

    if(!strcmp("url", argv[1]) && argc==4)
    {
        return tstURL(argv[2], argv[3], false);
    }

    if(!strcmp("url_ip", argv[1]) && argc==4)
    {
        return tstURL(argv[2], argv[3], true);
    }

    if(!strcmp("rss", argv[1]) && argc==4)
    {
        return TestBlogRSSHandler::doTest(argv[2], argv[3]);
    }

    if(!strcmp("rss", argv[1]) && argc==5)
    {
        return TestBlogRSSHandler::doTest(argv[2], argv[3], atoi(argv[4]));
    }

    if(!strcmp("friends", argv[1]) && argc==5)
    {
        return TestFriendsCollectionHandler::doTest
            (argv[2], argv[3], argv[4]);
    }

    return usage(argv[0]);
}
