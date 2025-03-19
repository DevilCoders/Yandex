#include "hostnamehash.h"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include <library/cpp/on_disk/st_hash/save_stl.h>

#include <library/cpp/charset/ci_string.h>
#include <util/system/compat.h>
#include <util/stream/file.h>
#include <library/cpp/deprecated/fgood/ffb.h>
#include <library/cpp/map_text_file/map_tsv_file.h>
#include <util/string/split.h>

using namespace std;

int hostmap::flushnew(const char* filename, bool quiet) const {
    ui32 n;
    if (!filename) return 1;
    if (maxid < maxnewid) {
        FILE *f=fopen(filename,"a");
        if (!f) return 2;
        for (n = maxid + 1; n <= maxnewid; n++) {
            if (!quiet) fprintf(stderr, "added: %u\t%s\n", n, id_n[n]);
            fprintf(f, "%u\t0\t0\t0\t0\t0\t%s\n", n, id_n[n]);
            fflush(f);
        }
        fclose(f);
    }
    return 0;
}

void hostmap::dump(const char* filename) const {
    if (!filename) return;
    FILE *f = fopen_chk(filename, "w");
    for (ui32 n = 0; n <= maxnewid; n++) if (id_n[n]) {
        fprintf(f, "%u\t%s\n", n, id_n[n] + id_n_offset);
        //fflush(f);
    }
    fclose_chk(f, filename);
}

bool hostmap::ins(const char *c, ui32 id, bool bad_insert_warnings) {
    if (static_buffer) {
        sb_type::const_iterator i(static_buffer->find(c));
        if (i != static_buffer->end()) {
            if (!(flags & INS_SERR_ADD_AGAIN_IN_SB))
                if (bad_insert_warnings) warnx("hostmap: %s was found in static_buffer. too bad.", c);
            flags |= INS_SERR_ADD_AGAIN_IN_SB;
            return false;
        }
    }
    std::pair<THashMap_ci_ui32_pa::iterator, bool> p =
        THashMap_ci_ui32_pa::insert(THashMap_ci_ui32_pa::value_type(sspool.append(c), id));
    if (!p.second) {
        if (p.first->second != id)
            if (bad_insert_warnings) warnx("hostmap: n_id[%s] = %i (wanted to set %i)", c, p.first->second, id);
        else {
            if (!(flags & INS_SERR_ADD_AGAIN_BYNAME))
                if (bad_insert_warnings) warnx("hostmap: tried to add host %s with %i second time\n"
                "hostmap: further warnings suppressed, but you should reconsider your sources", c, p.first->second);
            flags |= INS_SERR_ADD_AGAIN_BYNAME;
        }
        sspool.undo_last_append();
    }
    if (flags & ID_N_NEEDS_IMPORT) import_id_n_from_map();
    if (id >= id_n_size) {
        id_n_resize(id + 500000);
    }
    if (id_n[id]) {
        if (stricmp(id_n[id], c))
            if (bad_insert_warnings) warnx("hostmap: id_n[%i] = %s (wanted %s, will be %s)", id, id_n[id], c, p.first->first);
        else if (!(flags & INS_SERR_ADD_AGAIN_BYID) && (p.second || p.first->second != id)) {
                if (bad_insert_warnings) warnx("hostmap: tried to add hostid %i name %s second time\n"
                "hostmap: further warnings suppressed, but you should reconsider your sources", id, id_n[id]);
            flags |= INS_SERR_ADD_AGAIN_BYID;
        }
    }
    id_n[id] = p.first->first;
    return true;
}

void hostmap::id_n_resize(size_t sz) {
    if (id_n_size >= sz) return; //??
    const char **n = (const char**)realloc(id_n, sz * sizeof(char**));
    //if (!n) free(id_n); <- we do coredump, anyway :)
    id_n = n;
    memset(id_n + id_n_size, 0, (sz - id_n_size) * sizeof(char**));
    id_n_size = sz;
}

void hostmap::Inflate(ui32 maxsz, ui32 maxhash) {
    THashMap_ci_ui32_pa::reserve(maxhash + 1);
    if (maxsz == (ui32)-1) {
        id_n_resize(maxhash + 1);
        return;
    }
    id_n_resize(maxsz + 1);
    if (maxsz > maxid) maxid = maxsz;
    if (maxid > maxnewid) maxnewid = maxid;
}

int hostmap::load(const char* filename, bool merge, bool two_fields) {
    if (!filename)
        return 1;
    if (!merge)
        clear();
    ui32 mb = THashMap_ci_ui32_pa::size();
    if (!merge)
        id_n_resize(5000000);
    ui32 maxid = this->maxid;
    for (const auto& line : TMapTsvFile(filename, two_fields ? 2 : 7)) {
        TString hostname = TString(line.back());
        ui32 id;
        if (!TryFromString(line.front(), id))
            ythrow yexception() << "Field 0 of " <<  filename << " must be a number";
        ins(to_lower(hostname).c_str(), id);
        if (id > maxid)
            maxid = id;
    }
    maxnewid = (merge && maxnewid > maxid) ? maxnewid : maxid;
    if (!merge)
        this->maxid = maxid;
    if (THashMap_ci_ui32_pa::size() == mb)
        return 3;
    return 0;
}

void hostmap::load_new(const char* filename, bool merge, bool unchecked) {
    if (!merge)
        clear();
    for (const auto& line : TMapTsvFile(filename)) {
        if (line.empty() || line[0].empty())
            continue;
        TString first_field = TString(line[0]);
        if (unchecked) {
            ins(first_field.c_str(), ++maxnewid);
            continue;
        }
        if (HostByName(first_field.c_str(), true) == (ui32)-1)
            ythrow yexception() <<  filename << ": expected hostname, got '" <<  line[0] << "'";
    }
}

namespace {

struct SpecificSplitter {
    static void LineToFieldVec(TStringBuf line, TVector<TStringBuf>* res) {
        StringSplitter(line).SplitBySet(" \t").AddTo(res);
    }
};

}

int hostmap::load_from(const char* filename, ui16 name_col, ui16 id_col, bool strict, bool merge) {
    if (!filename)
        ythrow yexception() << "can't load hostnamehash: file name is empty";
    if (!merge)
        clear();
    ui32 mb = THashMap_ci_ui32_pa::size();
    if (!merge)
        id_n_resize(5000000);
    ui32 maxid = this->maxid;
    ui32 n = 0;
    ui16 max_col = std::max(id_col, name_col);
    for (const auto& line : TMapSvFile<SpecificSplitter>(filename)) {
        if (line.size() <= max_col) {
            if (strict)
                ythrow yexception()
                    << "file '" <<  filename
                    << "': expected " <<  max_col + 1
                    << " fields, read " <<  line.size();
            continue;
        }
        TString name = to_lower(TString(line[name_col]));
        ui32 id;
        if (!TryFromString(line[id_col], id)) {
            if (strict) {
                ythrow yexception()
                    << "file '" <<  filename
                    << "', coloumn " <<  id_col
                    << ": can't convert '" <<  line[id_col] << "' to number";
            }
            continue;
        }
        ins(name.c_str(), id, strict);
        if (id > maxid)
            maxid = id;
        n++;
    }
    maxnewid = (merge && maxnewid > maxid) ? maxnewid : maxid;
    if (!merge)
        this->maxid = maxid;
    if (THashMap_ci_ui32_pa::size() == mb)
        return 3;
    return 0;
}

hostmap::~hostmap() {
    clear();
}

void hostmap::clear() {
    THashMap_ci_ui32_pa::clear();
    node_pool.clear();
    if (!(flags & ID_N_NEEDS_IMPORT))
        free(id_n);
    id_n = nullptr, id_n_size = 0;
    sspool.clear();
    maxid = 0; maxnewid = 0;
    if (static_buffer) sb_map.term();
    id_n_offset = 0;
}

ui32 hostmap::HostByName(const char *name, bool add) noexcept {
    if (!strncmp(name,"http://",7)) name+=7;
    if (static_buffer) {
        if (Y_UNLIKELY(count4precharge != 0))
            if (!--count4precharge)
                precharge();
        sb_type::const_iterator i(static_buffer->find(name));
        if (i != static_buffer->end())
            return i.Value();
    }
    THashMap_ci_ui32_pa::const_iterator i = THashMap_ci_ui32_pa::find(name);
    if (i == THashMap_ci_ui32_pa::end()) {
        if (!add) return (ui32)-1;
        const char *sl = strchr(name, '/');
        if (sl && sl[1] != '/') return (ui32)-1;
        ui32 id = ++maxnewid;
        ins(name, id);
        return id;
    }
    return (ui32)(*i).second;
}
const char *hostmap::NameByHost(ui32 id) const noexcept {
    if (id >= id_n_size) return nullptr;
    if (Y_UNLIKELY(count4precharge != 0))
        if (!--count4precharge)
            precharge();
    const char *n = id_n[id];
    return n? n + id_n_offset : nullptr;
}

int hostmap::getridofmap() {
    if (!static_buffer)
        return 1;
    if (flags & ID_N_NEEDS_IMPORT) import_id_n_from_map();
    size_t rsz = 0;
    const char *map_start = (char*)sb_map.getData(), *map_end = map_start + sb_map.getSize();
    for(size_t n = 0; n < id_n_size; n++) {
        if (id_n[n] >= map_start && id_n[n] < map_end) rsz++;
    }
    THashMap_ci_ui32_pa::reserve(rsz);
    for(size_t n = 0; n < id_n_size; n++) {
        if (id_n[n] >= map_start && id_n[n] < map_end) {
            id_n[n] = THashMap_ci_ui32_pa::insert(THashMap_ci_ui32_pa::value_type(sspool.append(id_n[n]), n)).first->first;
        }
    }
    sb_map.term();
    static_buffer = nullptr;
    return 0;
}

// Note: you wan't most likely be able to use save_to_hash() w/o relink_to_map
// if your hash takes up more than 1/3 of available RAM
void hostmap::save_to_hash(const char *fname, bool relink_to_map) /*const*/ {
    getridofmap();

    {
        TFixedBufferFileOutput stream(fname);
        TSthashWriter<const char *, ui32> ks;
        save_for_st(&stream, ks);
        /* We make sure file is closed before save_sb_to_hash call. */
    }

    save_sb_to_hash(fname, relink_to_map);
}

void hostmap::save_sb_to_hash(const char *fname, bool relink_to_map) /*const*/ {
    sb_map.init(fname);
    static_buffer = (sb_type*)sb_map.getData();
    TFile f(fname, OpenAlways | WrOnly | ForAppend);
    if (relink_to_map) {
        // hard cleanup ->
        ((THashMap_ci_ui32_pa*)this)->~THashMap_ci_ui32_pa();
        new((THashMap_ci_ui32_pa*)this) THashMap_ci_ui32_pa;
        GetNodeAllocator().pool = &node_pool;
        node_pool.clear();
        // <- end hard cleanup
        sb_type::const_iterator i(static_buffer->begin()), e(static_buffer->end());
        for(; i != e; ++i)
            id_n[i.Value()] = i.Key() - (uintptr_t)static_buffer;
        uintptr_t zoff = 0;
        if (id_n)
            f.Write(id_n, (maxnewid + 1) * sizeof(char*));
        else
            f.Write(&zoff, sizeof(zoff));
        for(size_t n = 0; n <= maxnewid; n++)
            if (id_n && id_n[n])
                id_n[n] += (uintptr_t)static_buffer;
        return;
    }
    for(size_t n = 0; n <= maxnewid; n++) {
        uintptr_t off = 0;
        if (id_n && id_n[n]) {
            sb_type::const_iterator i(static_buffer->find(id_n[n]));
            if (i != static_buffer->end())
                off = i.Key() - (char*)static_buffer;
        }
        f.Write(&off, sizeof(off));
    }
}

void hostmap::import_id_n_from_map() {
    const char **id_n_mapped = id_n;
    id_n = (const char**)malloc((id_n_size = maxid + 1) * sizeof(char*));
    // out of memory -> crash
    memset(id_n, 0, id_n_size * sizeof(char*));
    for(ui32 n = 0; n <= maxid; n++) {
        id_n[n] = id_n_mapped[n]? id_n_mapped[n] + id_n_offset : nullptr;
    }
    id_n_offset = 0;
    flags &= ~ID_N_NEEDS_IMPORT;
    flags |= ID_N_IMPORTED;
    // may be, should unmap it back
    // ...
}

void hostmap::precharge() const {
    sb_map.precharge();
}

void hostmap::map_hash(const char *fname, bool imp_id_n, bool want_precharge) {
    sb_map.init(fname);
    const char *sb_map_end = (char*)sb_map.getData() + sb_map.getSize();
    count4precharge = want_precharge? 0 : Max<int>(20, sb_map.getSize() >> 23);
    if (want_precharge)
        precharge();
    static_buffer = (sb_type*)sb_map.getData();
    flags |= ID_N_NEEDS_IMPORT;
    id_n_offset = (uintptr_t)static_buffer;
    Y_ASSERT(sizeof(char*) == sizeof(uintptr_t));
    id_n = (const char**)static_buffer->end().Data;
    if ((char*)(id_n + 1) > sb_map_end)
        ythrow yexception() <<  fname << " is not a valid host hash file";
    const char **id_n_end = (const char**)((char*)static_buffer + sb_map.getSize());
    maxnewid = maxid = (id_n_size = id_n_end - id_n) - 1;
    if (imp_id_n)
        import_id_n_from_map();
}

// simplified version //

ui32 TMappedHostHash::HostByName(const char *name) const noexcept {
    if (!strncmp(name, "http://", 7)) name += 7;
    if (Y_UNLIKELY(count4precharge != 0))
        if (!--count4precharge)
            Precharge();
    sb_type::const_iterator i(NameToId->find(name));
    if (i != NameToId->end())
        return i.Value();
    return (ui32)-1;
}

const char *TMappedHostHash::NameByHost(ui32 id) const noexcept {
    if (id >= IdToName_size) return nullptr;
    if (Y_UNLIKELY(count4precharge != 0))
        if (!--count4precharge)
            Precharge();
    uintptr_t n = IdToName[id];
    return n? n + (const char*)NameToId : nullptr;
}

void TMappedHostHash::LoadHash(const char *fname, EDataLoadMode load_mode) {
    Y_ASSERT(sizeof(char*) == sizeof(uintptr_t));
    DoLoad(fname, load_mode);
    if (TDataFileBase::Length < sizeof(sb_type))
        ythrow yexception() <<  fname << " is not a valid host hash file (it is too small)";
    count4precharge = (load_mode & DLM_LD_TYPE_MASK) == DLM_MMAP_AUTO_PRC? Max<int>(20, TDataFileBase::Length >> 23) : 0;

    NameToId = (sb_type*)TDataFileBase::Start;
    IdToName = (uintptr_t*)NameToId->end().Data;
    const char *sb_map_end = (char*)TDataFileBase::Start + TDataFileBase::Length;
    IdToName_size = (uintptr_t*)sb_map_end - IdToName;

    if (*(size_t*)TDataFileBase::Start != 0)
        ythrow yexception() <<  fname << " is not a valid host hash file (header is bad)";
    if ((char*)(IdToName + 1) > sb_map_end)
        ythrow yexception() <<  fname << " is not a valid host hash file";
}

void TMappedHostHash::Term() {
    NameToId = nullptr;
    IdToName = nullptr;
    Destroy();
}
