#include <util/system/defaults.h>
#include <util/system/compat.h>
#include <util/system/file.h>
#include <util/system/maxlen.h>
#include <util/generic/utility.h>
#include <library/cpp/uri/http_url.h>
#include <library/cpp/string_utils/url/url.h>

#include "catfilter.h"

#define NUM_MASK    0x3fffffff
#define FOLLOW_MASK 0x40000000

static TCatFilter defaultCatFilter;

TCatAttrsPtr CatFilterFind(const char *url) {
   return defaultCatFilter.Find(url);
}

TCatAttrsPtr CatFilterFind2(const char *url, const char *Path) {
   return defaultCatFilter.Find2(url, Path);
}

TCatAttrsPtr CatFilterFind0(const char *relurl) {
   return defaultCatFilter.Find0(relurl);
}

int CatFilterLoad(const char *fname) {
   return defaultCatFilter.Load(fname);
}

// stuct TreeNode {
//   int       NumChilds;
//   char     *Entry[NumChilds];
//   TreeNode *ChildNode[NumChilds];
//   int      *CatNums;
// };

TCatFilter::TCatFilter() {
    DefFilter[0] = 0;
    DefFilter[1] = 0;
    RootNode = &DefFilter[0];
    Filter = nullptr;
    NullReturn = new TCatAttrs;
    FirstChar = nullptr;
    IsMapped = false;
}

TCatAttrsPtr TCatFilter::Find0(const char *relurl) const {
    relurl += GetHttpPrefixSize(relurl);
    const char *ptr = strchr(relurl, '/');
    if (ptr == relurl)
        return NullReturn;
    if (ptr == nullptr)
        ptr = strchr(relurl, 0);

    char host[HOST_MAX];
    char normHost[HOST_MAX];
    char normPath[URL_MAX];

    size_t hostLen = Min<size_t>(ptr - relurl, HOST_MAX - 1);
    strlcpy(host, relurl, hostLen + 1);
    NormalizeHostName(normHost, host, HOST_MAX);
    NormalizeUrlName(normPath, ptr, URL_MAX);
    return Find2(normHost, normPath);
}

TCatAttrsPtr TCatFilter::Find(const char *url) const {
    THttpURL Url;
    if (Url.Parse(url, THttpURL::FeatureSchemeFlexible))
        return NullReturn;
    if (!Url.IsValidAbs())
        return NullReturn;

    char normHost[HOST_MAX];
    char normPath[URL_MAX];
    NormalizeHostName(normHost, Url.PrintS(THttpURL::FlagHostPort), HOST_MAX);
    NormalizeUrlName(normPath, Url.GetField(THttpURL::FieldPath), URL_MAX);
    return Find2(normHost, normPath, Url.GetField(THttpURL::FieldScheme));
}

#define RET_NODE(Node)    (Filter+Filter[1]+*(Node+1+((*Node&NUM_MASK)<<1)))
#define RET_UP_NODE(Node) ((*Node&FOLLOW_MASK) ? NULL : RET_NODE(Node))

TCatAttrsPtr TCatFilter::GetReturnedAttrs(const ui32* attrs) const {
    THolder<TCatAttrs> returnedAttrs(new TCatAttrs);
    while (attrs && *attrs) {
        returnedAttrs->push_back(*attrs);
        ++attrs;
    }
    return returnedAttrs.Release();
}

TCatAttrsPtr TCatFilter::Find2(const char *Host
    , const char *Path, TStringBuf Scheme) const
{
    char *colon;
    const char *domain;
    char *point;
    char *query;
    const char *path;
    char *slash;
    ui32 *NextNode, *Node;
    Node = RootNode;
    char hostcopy[HOST_MAX + 1];
    strncpy(hostcopy, Host, HOST_MAX);
    hostcopy[HOST_MAX] = '\0';

    if (!Filter)
        return NullReturn;

    if (!Scheme.empty() && Scheme != "http") {
        char scheme[16];
        memcpy(scheme, Scheme.data(), Scheme.length());
        memcpy(scheme + Scheme.length(), "://", 4); // includes \0

        if ((*Node & NUM_MASK) == 0 || (NextNode = FindChild(Node, scheme)) == nullptr) {
            return GetReturnedAttrs(RET_NODE(Node));
        }

        Node = NextNode;
    }

    // Scan domain
    colon = (char*)strchr(hostcopy, ':');
    if (colon)
        *colon = 0;
    for (domain = point = nullptr; domain != hostcopy; Node = NextNode) {

        if (domain) {
            point = (char*)domain-1;
            *point = 0;
        }
        domain = strrchr(hostcopy, '.');
        domain = domain ? domain+1 : hostcopy;
        if ((*Node & NUM_MASK) == 0 ||
            (NextNode = FindChild(Node, domain)) == nullptr && (NextNode = FindChild(Node, "?")) == nullptr)
        {
            if (point)
                *point = '.';
            if (colon)
                *colon = ':';

            return GetReturnedAttrs(RET_UP_NODE(Node));

        }
        if (point)
            *point = '.';
    }

    // Scan Port
    if (colon)
        *colon = ':';
#if 0 // do not analize port
    if (!(*Node&NUM_MASK) ||
        !((NextNode = FindChild(Node, colon ? colon : ":80")) || (NextNode = FindChild(Node, ":?")))
    )
        return GetReturnedAttrs(RET_NODE(Node));
    Node = NextNode;
#endif

    // Scan Path
    query = (char*)strchr(Path, '?');
    if (query)
        *query = '/';
    for (path = (char*)Path; path; Node = NextNode, path = slash) {

        while (path[1] == '/')
            path++;
        slash = (char*)strchr(path+1, '/');
        if (slash)
            *slash = 0;
        if ((*Node & NUM_MASK) == 0 ||
            (NextNode = FindChild(Node, path)) == nullptr && (NextNode = FindChild(Node, "/?")) == nullptr)
        {
            if (slash)
                *slash = '/';
            if (query)
                *query = '?';

            return GetReturnedAttrs(RET_UP_NODE(Node));
        }
        if (slash)
            *slash = '/';


    }
    if (query)
        *query = '?';

    return GetReturnedAttrs(RET_NODE(Node));
}

#define PTR2NODE(ptr) (Filter+Node[(*Node&NUM_MASK)+1+(ptr-&Node[1])])
#define NODE2STR(p) (&FirstChar[*p])

ui32 *TCatFilter::FindChild(ui32 *Node, const char *key) const {
    ui32 *lo, *hi, *mid;
    int num, half, result;

    num = *Node&NUM_MASK;
    lo = &Node[1];
    hi = lo + (num - 1);

    while (lo <= hi)
        if ((half = num / 2) != 0) {
            mid = lo + (num & 1 ? half : (half - 1));
            if ((result = strcmp(key, NODE2STR(mid))) == 0)
                return PTR2NODE(mid);
            else if (result < 0) {
                hi = mid - 1;
                num = num & 1 ? half : half-1;
            } else {
                lo = mid + 1;
                num = half;
            }
        } else if (num)
            return strcmp(key, NODE2STR(lo)) ? nullptr : PTR2NODE(lo);
        else
            break;

    return(nullptr);
}

TCatFilter::~TCatFilter() {
    if (Filter)
        FreeFilter();
}

// 0 - filter size bytes
// 1 - node array size plus this header(4)
// 2 - cat array size
// 3 - first node

void TCatFilter::FreeFilter()
{
    if (IsMapped)
        FilterMap.term();
    else
        free(Filter);
    Filter = nullptr;
    RootNode = &DefFilter[0];
}

int TCatFilter::Load(const char *fname) {
    ui32 size;
    if (Filter) {
        FreeFilter();
    }
    IsMapped = false;
    TFileHandle file(fname, RdOnly);
    if (!file.IsOpen()) {
        warn("Can't read filter \"%s\"", fname);
        return 1;
    }
    if (4 != file.Read(&size, 4)) {
        warn("Can't read filter size");
        return 1;
    }
    if ((Filter = (ui32*) malloc((size + 3) & ~3)) == nullptr) {
        warn("Can't allocate filter");
        return 1;
    }
    Filter[0] = size;
    if (((ssize_t)size - 4) != file.Read(Filter + 1, size - 4)) {
        warn("Can't read filter");
        free(Filter);
        Filter = nullptr;
        return 1;
    }
    size_t nodenum = Filter[1];
    size_t catnum = Filter[2];
    RootNode = &Filter[Filter[3]];
    FirstChar = (char*)&Filter[nodenum+catnum];
    return 0;
}

int TCatFilter::Map(const char *fname) {
    if (Filter) {
        FreeFilter();
    }
    IsMapped = true;
    FilterMap.init(fname);
    Filter = (ui32*)FilterMap.getData();
    size_t nodenum = Filter[1];
    size_t catnum = Filter[2];
    RootNode = &Filter[Filter[3]];
    FirstChar = (char*)&Filter[nodenum + catnum];
    return 0;
}
