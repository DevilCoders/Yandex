#include <util/system/compat.h>
#include "mirrors.h"
#include "mirrors_status.h"

using namespace NLibMdb;

//for score
#include <ctype.h>

int sf(char **fb, char *buf); //ffb.h

const int path_pen = 3000;                //penalties for path segment
const int inlev_pen = 1000;               //penalties for domain level
const int lev_pen = 2 * inlev_pen;        //penalties for host name prefix
const int offic_sub = 100000 * inlev_pen; //ensures correct 'official mirrors' application
extern                                    // sorry, I use this value in mirrutil.cpp ->
const int rtxt_pen = inlev_pen * 7 / 10;  //penalties for robots.txt contents
const int port_pen = 20;                  //penalties for having port
// unused
//const int enc_pen = 10;                   //penalties for uncommon encoding
const int dom_pen = 50;                   //penalties for wrong top level domain
const int nowww_pen = 90;                 //penalties for not having www in address

/* The real function that calculates score for a mirror */
int score_export_pl(const char *c, int /*cy*/, int status) {
    if (!c || !*c) return 65536 * 32000;
    if (!*c) return 65536 * 32000;
    int s = 0, n;
    char* t;
    char* pa;
    char* p;
    if ((p = (char*)strchr(c, ':')) != nullptr) s += port_pen, *p = 0;
    s += (int)strlen(c);
    if ((pa = (char*)strchr(c, '/')) != nullptr) {
        s += path_pen, *pa = 0;
        for (t = pa + 1; *t; t++) if (*t == '/') s += path_pen;
    }
    //return $score if defined $mhost{$host};
    //$score += 1000;
    for (t = (char*)c, n = 1; *t; t++) if (*t == '.') n++;

    t = (char*)strrchr(c, '.');
    if (t && stricmp(t + 1, "ru")) s += dom_pen;

    t = (char*)strchr(c, '.');
    if (t) *t = 0;

    if (n == 2) {
        s += nowww_pen;
    } else if (n == 3) {
        if (strcmp(c, "www")) s += inlev_pen;
    } else if (n == 4) {
        s += lev_pen;
        if (strcmp(c, "www")) s += inlev_pen;
    } else {
        s += 2*lev_pen + inlev_pen;
    }

    //+$score += 5000 if $s[0] ne 'www' && exists $prefix{$s[0]};
    if (t) *t = '.';
    if (p) *p = ':';
    if (pa) *pa = '/';

    if (status > 0) {
        if (!(status & BITFLAG_WEAK_MIRROR)) status &= ~BITFLAG_REDIR;
        status &= ~BITFLAG_ROBTXT_ON;
    }

    if (status < static_cast<int>(BITFLAG_WEAK_MIRROR) && status > 0)
        s += status * rtxt_pen;
    else
        s += status * inlev_pen;

    //printf("%s\t%i\t%i\n", c, s, n);

    return s;
}

bool mirrors::is_www_mirror(const char* a, const char* b) {
    if (!a || !b) return 0;
//Skip possible https:// prefix
    const char *p;
    p = strchr(a, '@');
    if (p) a = p + 1;
    p = strchr(b, '@');
    if (p) b = p + 1;
    if (!strncmp(a,"http://",7)) a+=7;
    if (!strncmp(b,"http://",7)) b+=7;
    if (!strncmp(a,"https://",8)) a+=8;
    if (!strncmp(b,"https://",8)) b+=8;
    if (strlen(a) > strlen(b)) std::swap(a, b);
    if (!stricmp(a,b)) return true;
    if (strlen(b) < 4) return 0;
    return !strnicmp(b, "www.", 4) && !stricmp(a, b + 4);
}

/* rearrange files according to mirrors.txt loaded to om */
int mirrors::use_official_mirrors(mirrors &om, bool strict_mode) {
    cl_map::iterator i = map.begin(), j;
    int mmcnt = 0, tmcnt = 0;
    for(; i != map.end(); i++) {
        if ((i->second.status & (BITFLAG_ROBTXT_BAN | BITFLAG_WEAK_MIRROR) && i->second.status >= 0) || (strict_mode && i->second.status & BITFLAG_FAR_REDIR)) continue;
        char *s = (char*)i->first;
//    if (!strncmp(s,"https://",8)) s+=8;
//    char *c = strchr(s, ':');
//        if (c) *c = 0;
        j = om.map.find(s);
        i->second.status &= ~BITFLAG_WEBMASTER_SIGNAL;
        if (j != om.map.end()) {
        if ( !strict_mode || is_www_mirror(getcheck(s),s) ){
            if (strict_mode)
                i->second.status |= BITFLAG_WEBMASTER_SIGNAL;
            if (j->second.status <= -10)
                    i->second.score -= offic_sub, mmcnt++;
            else
                i->second.score -= lev_pen;
            tmcnt++;
        }
        }
//        if (c) *c = ':';
    }
    if (tmcnt == 0) return 3;
    press();
    creategroups();
    genrl();
//It's ok to have an empty official mirror list, so check is disabled
//    if (mmcnt == 0) return 1;
    return 0;
}

void TrunkLang(const char *&c) {
    const char *p = strchr(c, '@');
    if (p && strncmp(c, "RUS@", 4)) {
        c = nullptr;
    }
    else if (p) {
        c += 4;
    }
}

/* file loading/merging itselt */
int mirrors::merge(FILE *f, bool official_mirrors, bool use_multilang) {
    add("");
    int ingr = 0;
    //mirrors cl;
    char buf[1200], buf_bc[1200 + 5];
    size_t mb = map.size();
    char *fb[32];
    while(fgets(buf, 1200, f)) {
        int status = 0, cy = 0;
//        strlwr(buf);
        if (!official_mirrors) {
            const char *c = "";
            if (sf(fb, buf) >= 3) {
                status = atoi(fb[0]);
                cy = atoi(fb[1]);
                c = fb[2];
            }
            if (c && !use_multilang) {
                TrunkLang(c);
            }
            if (!c) continue;
            add(c, cy, status);
            if (bc && *c != 0) {
                char *d = (char*)bc->getcheck_lite(c);
                if (d == c) {
                    d = buf_bc;
                    strcpy(d, "www."); strcpy(d + 4, c);
                    add(d, cy, status | BITFLAG_FILTER);
                } else if (d != nullptr) {
                    add(d, cy, status);
                }
            }
        } else {
            const char *c = "";
            if (sf(fb, buf) >= 1)
                c = fb[0];
            if (!use_multilang && c) {
                TrunkLang(c);
            }
            if (!c) continue;
            if (!ingr) status = -25;
            else status = 1;
            ingr = *c != 0;
            add(c, -2, status);
        }
    }
    press();
    creategroups();
    genrl();
    if (map.size() == mb) return 3;
    return 0;
}
// ----------------------------------------------------------


mirrors::mirrors(bool srt, const char *filename) :
        node_pool(1024 * 1024),
        grp(-1), groups(nullptr), ngroups(0), bc(nullptr), must_sort(srt)
{
    map.GetNodeAllocator().pool = &node_pool;
    allow_paths = 0;
    load(filename);
}
int mirrors::load(const char *filename, bool official_mirrors, bool use_multilang) {
    clear();
    grp = -1;
    return merge(filename, official_mirrors, use_multilang);
}
int mirrors::load(FILE *f, bool official_mirrors, bool use_multilang) {
    clear();
    grp = -1;
    return merge(f, official_mirrors, use_multilang);
}

int mirrors::merge(const char *filename, bool official_mirrors, bool use_multilang) {
    if (!filename) return 1;
    FILE *f = fopen(filename, "r");
    if (!f) return 2;
    int rv = merge(f, official_mirrors, use_multilang);
    fclose(f);
    return rv;
}

int bomj_cnt::load(const char *filename) {
    if (!filename) return 1;
    FILE *f = fopen(filename, "r");
    if (!f) return 2;
    int rv = load(f);
    fclose(f);
    return rv;
}

int bomj_cnt::load(FILE *f) {
    char buf[1200];
    while(fgets(buf, 1200, f)) {
        int cy;
        char *c = buf, *d;
//        strlwr(buf);
        while (*c && *c < '!') c++;
        if (!*c || *c == '#') continue;
        if (sscanf(c, "%i", &cy) < 1) return 3;
        for(; *c > ' '; c++) { }
        while (*c && *c < '!') c++;
        if (!*c) return 4;
        d = c;
        for(; *c > ' '; c++) {
        }
        *c = 0; // white space is allowed after host name
        exts_map::iterator i = exts.find(d);
        if (i != exts.end()) return 5;
        exts.insert(exts_map::value_type(strdup(d), cy));
    }
    return 0;
}

const char *bomj_cnt::check(const char *url) {
#if 0
    const char *dots[10];
#endif
    const char *c;
    int n = 0;
    for (c = url; *c; c++) if (*c == '.') {
#if 0
        dots[n++] = c;
#endif
        if (n == 10) break; //we don't want to handle this
    }
    if (n < 3) return nullptr;
#if 0
    if (exts.find((char*)dots[n - 2] + 1) == exts.end()) return 0;
    return dots[n - 3] + 1;
#else
    sgrp *g = getgroup(url);
    if (!g) return nullptr;
    return (*g)[0].name;
#endif
}
const char *bomj_cnt::getcheck_lite(const char *url) const {
    const char *dots[10], *c;
    int n = 0;
    for (c = url; *c; c++) if (*c == '.') {
        dots[n++] = c;
        if (n == 10) break; //we don't want to handle this
    }
    if (n < 2) return nullptr;
    if (exts.find((char*)dots[n - 2] + 1) == exts.end()) return nullptr;
    if (n == 2) return url;
    return dots[n - 3] + 1;
}
const char *bomj_cnt::getcheck(const char *url) {
    sgrp *g = getgroup(url);
    if (!g) return nullptr;
    return (*g)[0].name;
}
void bomj_cnt::clear() {
    groups_map::iterator i = map.begin();
    while(i != map.end()) {
        sgrp *g = i->second;
        //map.erase(i);
        i++;
        for (unsigned k = 1; k < g->size(); k++) delete [](char*)(*g)[k].name;
        delete g;
    }
    map.clear();
}
void bomj_cnt::unload() {
    exts_map::iterator i = exts.begin();
    while(i != exts.end()) {
        char *c = i->first;
        //map.erase(i);
        i++;
        delete []c;
    }
    exts.clear();
}
sgrp *bomj_cnt::getgroup(const char *url) {
    const char *dots[10], *c;
    int n = 0;
    for (c = url; *c && *c != '/'; c++) if (*c == '.') {
        dots[n++] = c;
        if (n == 10) break; //we don't want to handle this
    }
    if (n < 2) return nullptr;
    exts_map::iterator j = exts.find((char*)dots[n - 2] + 1);
    if (j == exts.end()) return nullptr;
    int cy = j->second;
    if (n >= 3) c = dots[n - 3] + 1;
    else {
        c = url;
        url = nullptr;
    }
    if (!c) return nullptr;
    groups_map::iterator i = map.find((char*)c);
    if (i == map.end()) {
        sgrp *gp = new sgrp, &g = *gp;
        if (!url || (n == 3 && !strncmp(url, "www.", 4))) {
            g.resize(2);
        } else {
            g.resize(3);
            g[2] = sgrp_it(strdup(url), 0, 0, 1);
            url = nullptr;
        }
        char *d;
        if (url) d = strdup(url);
        else {
            d = new char[strlen(c) + 5];
            strcpy(d, "www."); strcpy(d + 4, c);
        }
        g[0] = sgrp_it(d + 4, 0, cy, -10);
        g[1] = sgrp_it(d, 0, 0, 1);
        map.insert(groups_map::value_type(d + 4, gp));
        return gp;
    } else {
        sgrp &g = *i->second;
        if (!url || (n == 3 && !strncmp(url, "www.", 4))) return &g;
        for (unsigned k = 2; k < g.size(); k++) if (!strcmp(url, g[k].name)) return &g;
        g.push_back(sgrp_it(strdup(url), 0, 0, 1)); //this is a rare case
        return i->second;
    }
}

void mirrors::clear() {
    killgroups();
    groups = nullptr;
    //cl_map::iterator i = map.begin();
    //while(i != map.end()) {
    //    char *c = i->first;
    //    //map.erase(i);
    //    i++;
    //    free(c);
    //}
    map.clear();
    rmap.clear();
    map_map.clear();
    names_pool.clear();
    node_pool.clear();
    //if (!map.empty()) printf("!!!!!!!!!!!!!!\n");
}

mirrors::~mirrors() {
    clear();
}
void mirrors::apply(int to, int p) {
    int pp = p;
    while(map_map[p] != p) p = map_map[p];
    if (pp != p) map_map[pp] = p;
    while(map_map[to] != p) {
        int t = map_map[to];
        map_map[to] = p;
        to = t;
    }
}
void mirrors::add(const char *c, const int cy, int status, int grp) {
    char *sl = (char*)c;
    const char *p = strchr(c, '@');
    const char *pp = strchr(c, '/');
    if (p && pp && p < pp) sl = (char*)p + 1;

    if(!strncmp(sl,"https://",8))
        sl+=8;
    if(!strncmp(sl,"http://",7))
        sl+=7;
    sl=(char*)strchr(sl, '/');
    if (sl && sl[1]) {
        if (!allow_paths) return;
    } else {
        if (sl) *sl = 0;
    }
    cl_map::iterator i = map.find((char*)c);
    if (i == map.end()) {
        map.insert(cl_map::value_type(names_pool.append(c), url_info(grp, must_sort? score(c, cy, status) : int(map.size()), cy, status)));
    } else {
        if (cy >= 0 && ((int)i->second.cy < 0 || (int)i->second.cy == 5)) i->second.cy = cy;
        if (i->second.status > status) i->second.status = status
            | (status < 0? 0 : (i->second.status & BITFLAG_WEAK_MIRROR));
        if (must_sort)
            i->second.score = score(c, i->second.cy, i->second.status);
        else if (status <= -25) // same effect as 'use official'
            i->second.score -= offic_sub;
        if (!(status & BITFLAG_FAR_REDIR)) {
            apply(grp, i->second.group);
        }
    }
}
void mirrors::add(const char *c, const int cy, int status) {
    if (*c == 0) { grp++; map_map.push_back(grp); return; }
    add(c, cy, status, grp);
}
bool mirrors::remove(const char *c) {
    if (*c == 0) return false;
    cl_map::iterator i = map.find((char*)c);
    if (i == map.end()) return false;
    //char *d = i->first;
    map.erase(i);
    //delete []d;
    return true;
}
void mirrors::press() {
    typedef cl_map_map::size_type size_t;
    size_t n, size = map_map.size();
    for(n = 0; n < size; n++) {
        int p = map_map[n];
        while(map_map[p] != p) p = map_map[p];
        map_map[n] = p;
    }
}
// What we have is that p1<p2 -> map_map[p1]<=map_map[p2] !?
void mirrors::genrl() {
    int n;
    TVector<int> scores;
    cl_map::iterator i = map.begin();
    rmap.resize(grp + 1); scores.resize(grp + 1);
    for(n = 0; n <= grp; n++) rmap[n] = nullptr, scores[n] = 65536 * 32000 + 1;
    for(; i != map.end(); i++) {
        //char *c = (char*)i->first;
        int n = map_map[i->second.group];
        int srn = scores[n], sri = i->second.score;
        if (sri < srn || rmap[n] == nullptr || (sri == srn && strcmp(i->first, rmap[n]) < 0))
            rmap[n] = i->first, scores[n] = sri;
    }
    for(n = 0; n <= grp; n++) if (scores[n] > 8500/*?*/) {
        //printf("bad \[%i\] = %s\n", scores[n], rmap[n]);
        //-rmap[n] = 0; //cut off too bad groups
    }
    for(n = 0; n <= grp; n++) rmap[n] = rmap[map_map[n]];
}

const char *mirrors::check(const char *c) const {
    cl_map::const_iterator i = map.find((char*)c);
    if (i == map.end()) {
        return bc? bc->check(c) : nullptr;
    }
    const char *d = rmap[i->second.group];
    if (i->first != d) return d;
    return nullptr;
}
const char *mirrors::getcheck(const char *c) const {
    cl_map::const_iterator i = map.find((char*)c);
    if (i == map.end()) {
        return bc? bc->getcheck(c) : nullptr;
    }
    const char *d = rmap[i->second.group];
    return d;
}
int mirrors::isMain(const char *c) const {
    cl_map::const_iterator i = map.find((char*)c);
    if (i == map.end()) return -1;
    const char *d = rmap[i->second.group];
    if (i->first == d) return 1;
    return 0;
}
int mirrors::isSoft(const char *c) const {
    cl_map::const_iterator i = map.find((char*)c);
    if (i == map.end()) return -1;
    return (i->second.status & BITFLAG_SOFT_MIRROR) == BITFLAG_SOFT_MIRROR ;
}
int mirrors::isSafe(const char *c) const {
    cl_map::const_iterator i = map.find((char*)c);
    if (i == map.end()) return -1;
    return (i->second.status & BITFLAG_UNSAFE_MIRROR) == 0 ;
}

void mirrors::killgroups() {
    if (groups) {
        for(int n = 0; n < ngroups; n++) groups[n].kill();
        delete[] groups;
    }
    groups = nullptr;
}
struct s_ksgc_less {
    sgrp_cnt *groups;
    s_ksgc_less(sgrp_cnt *g) : groups(g) {}
    bool operator()(ui32 a, ui32 b) { return groups[a] < groups[b]; }
};
void mirrors::creategroups() {
    int n;
    killgroups();
    groups = new sgrp_cnt[ngroups = grp + 1];

    cl_map::iterator i = map.begin();
    for(; i != map.end(); i++) {
        int &grp = i->second.group; grp = map_map[grp];
        groups[grp].add(i->first, i->second.score, i->second.cy, i->second.status);
    }
    /*if (must_sort)*/ for(n = 0; n <= grp; n++) groups[n].sort();
    if (must_sort) {
        ui32 *map0 = new ui32[ngroups], *map1 = new ui32[ngroups];
        for(n = 0; n < ngroups; n++) map0[n] = n;
        std::sort(map0, map0 + ngroups, s_ksgc_less(groups));
        for(n = 0; n < ngroups; n++) map1[map0[n]] = n;
        for(cl_map::iterator i = map.begin(); i != map.end(); i++) i->second.group = map1[i->second.group];
        sgrp_cnt *g1 = groups;
        groups = new sgrp_cnt[ngroups];
        for(n = 0; n < ngroups; n++) groups[map1[n]] = g1[n], map_map[n] = n;
        delete[] map0; delete[] map1; delete[] g1;
    }
}

int mirrors::score(const char *c, const int cy, int status) {
    return score_export_pl(c, cy, status);
}


sgrp *mirrors::getgroup(int n) { return groups[n].group; }
const sgrp *mirrors::getgroup(int n) const { return groups[n].group; }

sgrp *mirrors::getgroup(const char *c) {
    cl_map::iterator i = map.find((char*)c);
    if (i == map.end()) {
        if (!bc) return nullptr;
        return bc->getgroup(c);
    }
    return groups[i->second.group].group;
}

const sgrp *mirrors::getgroup(const char *c) const {
    cl_map::const_iterator i = map.find((char*)c);
    if (i == map.end()) {
        if (!bc)
            return nullptr;
        return bc->getgroup(c);
    }
    return groups[i->second.group].group;
}

mirrors::mirrors(const mirrors &src) :
        node_pool(1024 * 1024),
        grp(-1), groups(nullptr), ngroups(0), bc(src.bc), must_sort(src.must_sort)
{
    allow_paths = 0;
    if (this == &src) return;
    map.GetNodeAllocator().pool = &node_pool;
    clear();
    merge(src);
}

void mirrors::merge(const mirrors &src) {
    int n, l;
    unsigned k;
    l = src.ngroups;
    for(n = 0; n < l; n++) {
        if (src.groups[n].group) {
            map_map.push_back(++grp);
            sgrp &g = *src.groups[n].group;
            for (k = 0; k < g.size(); k++) {
                add(g[k].name, g[k].cy, g[k].status, grp);
            }
        }
    }
    press();
    creategroups();
    genrl();
}

mirrors_mapped::mirrors_mapped(const char *fname, int wantPrecharge) {
    base_offset = 0, map = nullptr, rmap = nullptr, allgroups = nullptr, numgroups = 0;
    if (fname)
        Map(fname, wantPrecharge);
}
mirrors_mapped::~mirrors_mapped() {}

void mirrors_mapped::Map(const char *fname, int wantPrecharge) {
    Load(fname, wantPrecharge? wantPrecharge == 2? DLM_MMAP_AUTO_PRC : DLM_MMAP_PRC : DLM_MMAP);
}

void mirrors_mapped::Load(const char *fname, EDataLoadMode loadMode) {
    DoLoad(fname, loadMode);
    map = (sb_cl_map*)TDataFileBase::Start;
    const char *map_end = (char*)TDataFileBase::Start + TDataFileBase::Length;
    TotalRequests = 0;
    LoadMode = loadMode;
    base_offset = (uintptr_t)map;
    Y_ASSERT(sizeof(char*) == sizeof(uintptr_t));
    const char *p = (const char*)map->end().Data;
    if (p >= map_end)
        throw yexception() << "bad mirrors hash format [1] of " << fname;
    ui64 rmap_size = *(ui64*)p; p += sizeof(ui64);
    numgroups = int(rmap_size - 1);
    rmap = (rmap_and_grp_type*)p; p += rmap_size * sizeof(rmap_and_grp_type);
    if (p >= map_end)
        throw yexception() << "bad mirrors hash format [2] of " << fname;
    allgroups = (sgrp_it*)p;
}
void mirrors_mapped::Precharge() const {
    TDataFileBase::Precharge();
    LoadMode = DLM_MMAP_PRC;
}

const char *mirrors_mapped::CheckImpl(const TdCiter &i) const {
    if (i == map->end())
        return nullptr;
    const char *d = GetVal(i);
    if (i.Key() != d) return d;
    return nullptr;
}

const char *mirrors_mapped::GetCheckImpl(const TdCiter &i) const {
    if (i == map->end())
        return nullptr;
    const char *d = GetVal(i);
    return d;
}

int mirrors_mapped::IsMainImpl(const TdCiter &i) const {
    if (i == map->end()) return -1;
    const char *d = GetVal(i);
    if (i.Key() == d) return 1;
    return 0;
}

int mirrors_mapped::IsSoftImpl(const TdCiter &i) const {
    if (i == map->end()) return -1;
    return (i.Value().status & BITFLAG_SOFT_MIRROR) == BITFLAG_SOFT_MIRROR ;
}

int mirrors_mapped::IsSafeImpl(const TdCiter &i) const {
    if (i == map->end()) return -1;
    return (i.Value().status & BITFLAG_UNSAFE_MIRROR) == 0 ;
}


const psgrp mirrors_mapped::getgroup(int n) const {
    const rmap_and_grp_type &r = rmap[n];
    return psgrp(allgroups + r.grp_start, r.size(), base_offset);
}

const psgrp mirrors_mapped::GetGroupImpl(const TdCiter &i) const {
    if (i == map->end())
        return psgrp(nullptr, 0, 0);
    const rmap_and_grp_type &r = rmap[i.Value().group];
    return psgrp(allgroups + r.grp_start, r.size(), base_offset);
}
