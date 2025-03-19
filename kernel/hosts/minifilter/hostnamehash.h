#pragma once

#include <library/cpp/charset/ci_string.h>
#include <library/cpp/deprecated/datafile/datafile.h>
#include <library/cpp/deprecated/mapped_file/mapped_file.h>
#include <library/cpp/on_disk/st_hash/static_hash.h>

#include <util/generic/hash.h>
#include <util/generic/vector.h>
#include <util/memory/segpool_alloc.h>
#include <util/system/filemap.h>

typedef THashMap<const char *, ui32, ci_hash32, ci_equal_to> THashMap_ci_ui32;
typedef THashMap<const char *, ui32, ci_hash32, ci_equal_to, segpool_alloc_vt> THashMap_ci_ui32_pa;

struct hostmap : /*protected*/public THashMap_ci_ui32_pa {
    segmented_string_pool sspool;
    segmented_pool<char> node_pool;
    typedef sthash<const char*, ui32, ci_hash32, ci_equal_to> sb_type;
    sb_type *static_buffer;
    mutable TMappedFile sb_map;
    const char **id_n;
    size_t id_n_size;
    ui32 maxid, maxnewid;
    ui32 flags;
    uintptr_t id_n_offset;
    hostmap() : node_pool(128 * 1024), static_buffer(nullptr), id_n(nullptr), id_n_size(0),
                maxid(0), maxnewid(0), flags(0), id_n_offset(0),
                count4precharge(0)
    {
        GetNodeAllocator().pool = &node_pool;
    }
    ~hostmap();

    ui32 HostByName(const char *name, bool add = false) noexcept;
    const char *NameByHost(ui32) const noexcept;

    void clear();
    int load(const char*, bool merge = false, bool two_fields = false);

    // only one field in file, ids are assigned. unchecked -> not necessarily a host list
    void load_new(const char*, bool merge, bool unchecked);

    /* name and id are read from name_col and id_col positions in tabulated text file,
       if strict is true, then lexception is thrown in case of wrong file format
       and warnx() is called in case some bad events in ins()
    */
    int load_from(const char*, ui16 name_col = 0, ui16 id_col = 1, bool strict = true, bool merge = false);

    int flushnew(const char* filename, bool quiet = false) const;
    void dump(const char* filename) const;
    size_t HostCount() const { return THashMap_ci_ui32_pa::size(); }
    size_t size() const {
        ui32 actualMaxId = std::max(maxid, maxnewid);
        return actualMaxId? actualMaxId + 1 : 0;
    }
    void save_to_hash(const char *fname, bool relink_to_map = false) /*const*/;
    void map_hash(const char *fname, bool imp_id_n = false, bool want_precharge = false);
    int getridofmap();

    // only call before load()
    void Inflate(ui32 maxsz, ui32 maxhash);

    /* TODO: let ins() throw yexeptions instead of just flooding stderr with messages */
    // Note: ins is for dump loading only. Do not use it until you know what you are doing.
    bool ins(const char *name, ui32 id, bool bad_insert_warnings = true);
    void save_sb_to_hash(const char *fname, bool relink_to_map) /*const*/; // second stage for save_to_hash

protected:
    enum {
        INS_SERR_ADD_AGAIN_BYNAME = 1,
        INS_SERR_ADD_AGAIN_BYID = 2,
        INS_SERR_ADD_AGAIN_IN_SB = 4,
        ID_N_NEEDS_IMPORT = 8,
        ID_N_IMPORTED = 16,
    };

    void import_id_n_from_map();
    void id_n_resize(size_t sz);
    void precharge() const;
    mutable int count4precharge;
};

/// Subset of read-only functionality of hostmap
struct TMappedHostHash : public TDataFileBase {
    typedef sthash<const char*, ui32, ci_hash32, ci_equal_to> sb_type;

    ui32 HostByName(const char *name) const noexcept;
    const char *NameByHost(ui32) const noexcept;
    bool has(const char *name) const noexcept {
        return HostByName(name) != (ui32)-1;
    }

    // to be deleted
    TMappedHostHash(const char *fname = nullptr, bool want_precharge = false) :
                    count4precharge(0), NameToId(nullptr), IdToName(nullptr), IdToName_size(0)
    {
        if (fname)
            MapHash(fname, want_precharge);
    }

    TMappedHostHash(const char *fname, EDataLoadMode load_mode) :
                    count4precharge(0), NameToId(nullptr), IdToName(nullptr), IdToName_size(0)
    {
        if (fname)
            LoadHash(fname, load_mode);
    }

    void LoadHash(const char *fname, EDataLoadMode load_mode);
    void Term();

    // compatibility wrapper
    void MapHash(const char *fname, bool want_precharge = false) {
        LoadHash(fname, want_precharge? DLM_MMAP_PRC : DLM_MMAP_AUTO_PRC);
    }

    sb_type::const_iterator names_begin() const noexcept {
        return NameToId->begin();
    }
    sb_type::const_iterator names_end() const noexcept {
        return NameToId->end();
    }
    ui32 ids_size() const noexcept {
        return (ui32)IdToName_size;
    }

protected:
    mutable int count4precharge;

    sb_type     *NameToId;
    uintptr_t   *IdToName;
    size_t       IdToName_size;
};

struct THashMap_ci_ssp : /*protected*/public THashMap_ci_ui32 {
    segmented_string_pool sspool;
    typedef std::pair<THashMap_ci_ui32::iterator, bool> inspair;
    inspair insert(const char *c, ui32 v) {
         inspair p = THashMap_ci_ui32::insert(THashMap_ci_ui32::value_type(sspool.append(c), v));
         if (!p.second)
            sspool.undo_last_append();
         return p;
    }
    inspair insert(const char *c, size_t len, ui32 v) {
         inspair p = THashMap_ci_ui32::insert(THashMap_ci_ui32::value_type(sspool.append(c, len), v));
         if (!p.second)
            sspool.undo_last_append();
         return p;
    }
    void clear() {
        THashMap_ci_ui32::clear();
        sspool.clear();
    }
};
