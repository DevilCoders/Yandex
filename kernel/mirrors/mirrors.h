#pragma once

#include <util/system/defaults.h>
#include <library/cpp/charset/doccodes.h>
#include <util/memory/segpool_alloc.h>
#include <util/generic/vector.h>
#include <util/generic/hash.h>
#include <util/system/filemap.h>
#include <library/cpp/on_disk/st_hash/static_hash.h>
#include <library/cpp/deprecated/datafile/datafile.h>


#define HTTP_OFF(c) { if (!strncmp(c, "http://", 7)) c += 7; }

struct sgrp_it {
    const char *name;
    int score;
    int cy;
    int status;
    sgrp_it() {} //use carefully!
    sgrp_it(const char *n, int sc, int c, int st) : name(n), score(sc),cy(c),status(st) {}
    bool operator <(const sgrp_it &with) const { return score<with.score || (score==with.score && strcmp(name,with.name)<0); }
    bool operator ==(const sgrp_it &with) const { return score==with.score && !strcmp(name,with.name); }
};
typedef std::vector<sgrp_it> sgrp;
class sgrp_cnt;
struct bomj_cnt;

//This _must_ be moved to util in future
struct str_equal_to__M {
    bool operator()(const char *x, const char *y) const { return !strcmp(x,y); }
    bool operator()(const char *x, const TStringBuf &y) const {
        // as comparison is likely to fail, perform the strlen scan second
        return !memcmp(x, y.data(), y.length()) && strlen(x) == y.length();
    }
};

struct mirrors {
    struct url_info {
        int group;
        int score;
        int cy;
        int status;
        url_info(int theGrp, int scr, int c, int st) : group(theGrp), score(scr), cy(c), status(st) {}
    };
    typedef THashMap<char*, url_info, THash<char*>, str_equal_to__M, segpool_alloc_vt> cl_map;
    typedef sthash<const char*, url_info, THash<const char*>, str_equal_to__M> sb_cl_map;
    typedef std::vector<char*> cr_map;
    typedef std::vector<int> cl_map_map;
    cl_map map;
    cl_map_map map_map;
    cr_map rmap;
    segmented_string_pool names_pool;
    segmented_pool<char> node_pool;
    int grp;
    sgrp_cnt *groups; int ngroups;
    bomj_cnt *bc;
    bool must_sort, allow_paths, is_ro_ld;

    void apply(int to,int p);
    virtual void add(const char *c, const int cy = 0, int status = 0);
    void add(const char *c, const int cy, int status, int grp);
    bool remove(const char *c);
    void press();
    void genrl();
    void creategroups();
    void killgroups();
    int score(const char *c, int cy, int);

    mirrors(bool sort = false, const char *filename=nullptr);//"mirrors.in"
    mirrors(const mirrors &src);//copy
    virtual ~mirrors();
    /// These commands load/merge mirror list from file.
    /// If boolean paramemer is true, load 'official list' file, i.e. without
    /// first two data fields.
    int load(const char*, bool = false, bool = false);
    int load(FILE*, bool = false, bool = false);
    int merge(const char*, bool = false, bool = false);
    int merge(FILE*, bool = false, bool = false);
    void merge(const mirrors &src);

    /// Select main mirrors listed in om. No new names are added to this!
    int use_official_mirrors(mirrors &om, bool strict_mode=false);

    /// Get main mirror name. Return null if c not found.
    const char *getcheck(const char *c) const;
    /// Also returns 0 if c is main mirror
    const char *check(const char *c) const;
    //returns true if a and b differs only by www. prefix
    static bool is_www_mirror(const char* a, const char* b);

    void clear();
    int isMain(const char*) const; // 0 - no, 1 - yes, -1 - not found
    int isSoft(const char*) const; // 0 - no, 1 - yes, -1 - not found
    int isSafe(const char*) const; // 0 - no, 1 - yes, -1 - not found
    sgrp *getgroup(int n);
    const sgrp *getgroup(int n) const;
    sgrp *getgroup(const char *);
    const sgrp *getgroup(const char *) const;
};

struct psgrp {
    const sgrp_it *start;
    const size_t Size;
    const uintptr_t base_offset;
    const sgrp_it operator[] (size_t i) const { return sgrp_it(start[i].name + base_offset, start[i].score, start[i].cy, start[i].status); }
    size_t size() const { return Size; }
    psgrp(const sgrp_it *st, size_t sz, uintptr_t bo) : start(st), Size(sz), base_offset(bo) {}
};

struct mirrors_mapped : public TDataFileBase {
    typedef mirrors::sb_cl_map sb_cl_map;
    typedef sb_cl_map::const_iterator TdCiter;
    struct rmap_and_grp_type {
        const char       *name;
        size_t      grp_start;
        size_t size() const { return this[1].grp_start - grp_start; }
    };
    uintptr_t base_offset;
    const sb_cl_map *map;
    const rmap_and_grp_type *rmap;
    const sgrp_it *allgroups;
    int numgroups;

    mutable EDataLoadMode LoadMode; // 3 = auto precharge
    mutable int TotalRequests; // auto precharge after 30 requests

    void Map(const char *fname, int wantPrecharge = 2);
    void Load(const char *fname, EDataLoadMode loadMode);
    void Precharge() const;

    // Constructor will throw yexception if fname!=0 and file can't be mapped
    mirrors_mapped(const char *fname = nullptr, int wantPrecharge = 2);
    ~mirrors_mapped();

    const char *getcheck(const char *c) const {
        return GetCheckImpl(FindReq(c));
    }
    const char *getcheck(const TStringBuf &str) const {
        return GetCheckImpl(FindReq(str));
    }

    const char *check(const char *c) const {
        return CheckImpl(FindReq(c));
    }
    const char *check(const TStringBuf &str) const {
        return CheckImpl(FindReq(str));
    }

    const char *getmain(const char *c) const {
        const char *m = getcheck(c);
        return m? m : c;
    }

    // 0 - no, 1 - yes, -1 - not found
    int isMain(const char *c) const {
        return IsMainImpl(FindReq(c));
    }
    int isMain(const TStringBuf &str) const {
        return IsMainImpl(FindReq(str));
    }

    // 0 - no, 1 - yes, -1 - not found
    int isSoft(const char *c) const {
        return IsSoftImpl(Find(c));
    }
    int isSoft(const TStringBuf &str) const {
        return IsSoftImpl(Find(str));
    }

    // 0 - no, 1 - yes, -1 - not found
    int isSafe(const char *c) const {
        return IsSafeImpl(Find(c));
    }
    int isSafe(const TStringBuf &str) const {
        return IsSafeImpl(Find(str));
    }

    const psgrp getgroup(int n) const;

    const psgrp getgroup(const char *str) const {
        return GetGroupImpl(FindReq(str));
    }
    const psgrp getgroup(const TStringBuf &str) const {
        return GetGroupImpl(FindReq(str));
    }

private:
    template <typename T> TdCiter Find(const T &str) const {
        return map->find(str);
    }

    template <typename T> TdCiter FindReq(T str) const {
        ++TotalRequests;
        if (Y_UNLIKELY(LoadMode == DLM_MMAP_AUTO_PRC && TotalRequests > 30))
            Precharge();
        return Find(str);
    }

private:
    const char *GetVal(const TdCiter &it) const
    {
        return rmap[it.Value().group].name + base_offset;
    }
    const char *CheckImpl(const TdCiter &it) const;
    const char *GetCheckImpl(const TdCiter &it) const;
    int IsMainImpl(const TdCiter &it) const;
    int IsSoftImpl(const TdCiter &it) const;
    int IsSafeImpl(const TdCiter &it) const;
    const psgrp GetGroupImpl(const TdCiter &it) const;
};

struct bomj_cnt {
protected:
    typedef THashMap<char*, int, ::hash<const char*>, str_equal_to__M> exts_map;
    typedef THashMap<char*, sgrp*, ::hash<const char*>, str_equal_to__M> groups_map;
    bomj_cnt(bomj_cnt&);

public:
    exts_map exts;
    groups_map map;
    bomj_cnt() = default;
    ~bomj_cnt() { clear(); unload(); }
    int load(const char *fname);
    int load(FILE *f);

    const char *check(const char *url);
    const char *getcheck(const char *url);
    sgrp *getgroup(const char *url);
    void clear();
    void unload();
    // do not perform any allocations
    const char *getcheck_lite(const char *url) const;
};

class sgrp_cnt {
    friend struct mirrors;
    friend struct mirrutil;
    friend struct s_ksgc_less;
    sgrp *group;
    sgrp_cnt() : group(nullptr) {}
    sgrp *operator ->() { return group; }
    void add(const char *name, int sc, int cy, int status) {
        if (!group) group=new sgrp;
        sgrp_it sss(name,sc,cy,status);
        //sgrp::iterator i = group->begin(), e = group->end();
        //for(; i != e; i++) if (*i == sss) break;
        //if (i != e) return;  // This should not happen!
        group->push_back(sss);
    }
    void sort() {
        if (group && group->size()>1) std::sort(group->begin(),group->end());
    }
    bool operator <(const sgrp_cnt &with) const {
        if (!group) return false;
        else
        if (!with.group) return true;
        return *group<*with.group;
    }
    void kill() { if (group) delete group; group=nullptr; }
};
