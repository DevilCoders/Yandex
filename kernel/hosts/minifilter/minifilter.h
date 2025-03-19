#pragma once

#include <library/cpp/on_disk/st_hash/static_hash_map.h>
#include <library/cpp/deprecated/sgi_hash/sgi_hash.h>
#include <library/cpp/map_text_file/map_tsv_file.h>

#include <library/cpp/charset/ci_string.h>
#include <util/generic/string.h>
#include <util/generic/hash.h>
#include <util/generic/hash_set.h>
#include <util/memory/segmented_string_pool.h>
#include <util/system/maxlen.h>
#include <util/system/filemap.h>

// special replacement functions, analogue of respective scripts in yweb/scripts/ :)
void TRSPCHARS(char *s);
void UNTRSPCHARS(char *s);
void TRSPCHARS(char *s, size_t l);
void UNTRSPCHARS(char *s, size_t l);
void TRSPCHARS(const char *s, char *d);
void UNTRSPCHARS(const char *s, char *d);

/// MiniFilter class tells whether some url lies within directory or script
//  listed in some text file. It provides functionality similar to
//  FilterFind/CatFilterFind except that filter data is a text file.
//
//  MiniFilter is optimized for use with sorted input, i.e. Find() tries to
//  use results of previous call to Find()

struct MiniFilter {
 protected:
   struct MF_PathSeg {
      const char *str;
      size_t str_len;
      char terminator;
      bool is_leaf;
      ui32 data;
      size_t num_children;
      const char **children;
   };

   struct MF_PathData {
      char *str;
      ui32 data;
      MF_PathData(char *str_, ui32 data_) : str(str_), data(data_) {}
      MF_PathData() {}
      bool operator <(const MF_PathData &w) const {
          return strcmp(str, w.str) < 0;
      }
   };

 public:
   typedef THashMap<const char *, size_t> Thost_map;
   Thost_map host_map;
   segmented_pool<char> p;
   TString FileName;
   char PathDelim[256];

   struct FindCache {
      FindCache() : cur_host(nullptr) {}

      char cur_host_buf[HOST_MAX];
      const char *cur_host;
      size_t *curr_host_table;
      std::vector<MF_PathSeg> curr_path_tables;
   };

   FindCache Cache;

   MiniFilter(const char *delims = "/?" /* "&/=?_" */);
   ~MiniFilter();

   // Note: urls must be sorted by host. Allowed formats:
   // 1. <url>
   // 2. <host>\t<path>
   // mode: 0 - no flags, 1 - flag is line number, 2 - flag is in +1 field
   void LoadFlat(const char *fname, int mode);

   // return value will become invalid after next Find() call
   ui32 *Find(const char *url);
   ui32 *Find(const char *url, FindCache& cache) const;

   bool Has(const char *url) { return Find(url) != nullptr; }
   bool Has(const char *url) const { FindCache cache; return Find(url, cache) != nullptr; }

 protected:
   void AddHostPaths(const char *host, std::vector<MF_PathData> &paths);
   void AddChildren(const char **chld_ptr, MF_PathData *paths_b, MF_PathData *paths_e, size_t child_offs);
   static size_t GetChildCount(MF_PathData *paths_b, MF_PathData *paths_e, size_t child_offs);
};

// ************* domain level/owner/spam testing functions *************

/// Note: tld optimization
/// has_domain, area_subdomain, is_ban_host_or_domain, etc. by default consider that
/// top-level domains can't be in the container they use.

struct hostmap;
struct TMappedHostHash;
struct bitmap_1;
extern segmented_string_pool sts_pool; // global pool (non-thread-safe, etc.!) for lazy people

// this is map to unused int by intent
struct hostset : public THashMap<const char *, int> {
    bool HasTld = false; // turn off tld optimization (set manually)
    bool has_domain(const char *host) const {
        return find_domain(host) != end();
    }
    const_iterator find_domain(const char *host) const;
    hostset() {}
    hostset(const char *fname1, segmented_string_pool &p = sts_pool, bool strict_fmt = true, bool lower_case = true) {
        load(fname1, p, strict_fmt, lower_case);
    }
    hostset(const char *fname1, int key_field, int value_field, segmented_string_pool &p = sts_pool, bool strict_fmt = true, bool lower_case = true) {
        loadf(fname1, key_field, value_field, p, strict_fmt, lower_case);
    }
    hostset(const char *fname1, const char *fname2, segmented_string_pool &p = sts_pool) {
        load(fname1, p);
        load(fname2, p);
    }

    // loads (1 or more)-field file with host/domain list, second field is always 1
    void load(const char *fname, segmented_string_pool &p = sts_pool, bool strict_fmt = true, bool lower_case = true); // it appends!

    // key field must be present, value field might not (will be 0)
    void loadf(const char *fname, int key_field = 0, int value_field = 1, segmented_string_pool &p = sts_pool, bool strict = false, bool lower_case = true, int vfdefv = 0); // it appends!

    // empty lines & comments (; or #) allowed
    void load2(const char *fname, segmented_string_pool &p = sts_pool); // it appends!

    void AddKey(const char *host, int value = 1, segmented_string_pool &p = sts_pool) {
        insert_ctx ins;
        if (!contains(host, ins))
            insert_direct(std::make_pair(p.append(host), value), ins);
    }
};

// in h, subdomain of any domain in h; returns 'depth' (in h -> 1)
int is_ban_host_or_domain_c(const hostset &h, const char *host);

int is_ban_host_or_domain(hostmap &h, const char *&host);
int is_ban_host_or_domain(const TMappedHostHash &h, const char *&host);

inline int is_ban_host_or_domain_c(hostmap &h, const char *host) {
   return is_ban_host_or_domain(h, host);
}
int is_ban_host_or_domain_c(const TMappedHostHash &h, const char *host);

template<class T>
inline bool InDomain(const T &h, const char *host) {
   if (h.Has(host)) return true;
   while((host = strchr(host, '.')) != nullptr) {
      host++;
      if (*host == '.' || !*host || !strchr(host, '.')) break;
      if (h.Has(host)) return true;
   }
   return false;
}

// in h, subdomain of any domain in h; returns hostid
ui32 host_or_domain(hostmap &h, const char *host);
ui32 host_or_domain(const TMappedHostHash &h, const char *host);

// subdomain of any domain in h (can not be in h itself)
const char *area_subdomain(const hostset &h, const char *host);

// subdomain of any domain in h (can be in h, if h contains domain and its subdomains)
const char *area_subdomain_r(const hostset &h, const char *host);

template <typename hasher_type>
void load_txt_file(THashMap<const char *, const char *, hasher_type> &h, const char *fname, segmented_string_pool &p) {
   THashSet<const char *> nsnames;
   for (const auto& line : TMapTsvFile(fname)) {
      int fields_number = line.size();
      if (fields_number != 2 && fields_number != 3)
          ythrow yexception() << "" << fname << ": " << fields_number << " fields, expected 2 or 3";
      auto it = nsnames.find(line.back());
      if (it == nsnames.end()) {
         it = nsnames.insert(p.Append(line.back())).first;
      }
      h.insert(typename THashMap<const char *, const char *, hasher_type>::value_type(p.Append(line.front()), *it));
   }
}

template <typename hasher_type>
void load_txt_file(THashMap<const char *, int, hasher_type> &h, const char *fname, segmented_string_pool &p) {
   for (const auto& line : TMapTsvFile(fname, 2)) {
      int num;
      if (!TryFromString(line[1], num))
          ythrow yexception() << "" << fname << ": field " << 1 << " is not a number: '" << line[1] << "'";
      if (!h.insert(typename THashMap<const char *, int, hasher_type>::value_type(p.Append(line.front()), num)).second)
         p.undo_last_append();
   }
}

void load_txt_file(THashMap<int, const char *> &h, const char *fname, segmented_string_pool &p = sts_pool);
void load_txt_file(THashMap<int, int> &h, const char *fname);

void read_ids_to_bitmap(bitmap_1 &dst, const char *fname, const char *fname2 = nullptr);

struct THost2Str : public THashMap<const char *, const char *> {
   segmented_string_pool pool;
   bool HasTld = false; // turn off tld optimization (set manually)
   const_iterator find_domain(const char *host) const;
   void load_txt_file(const char *fname) {
       ::load_txt_file(*this, fname, pool);
   }
};

// for simplifying code that gets url's host or owner in-place
struct TQUrlSpl {
   char *slash, *colon;
   TQUrlSpl() {}
   TQUrlSpl(char *url) { outline_host(url); }
   inline void outline_host(char *url) {
      slash = strchr(url, '/');
      if (slash) *slash = 0;
      colon = strchr(url, ':');
      if (colon) *colon = 0;
      strlwr(url);
   }
   inline void recover_url() const {
      if (colon) *colon = ':';
      if (slash) *slash = '/';
   }
   inline void recover_host() const {
      if (colon) *colon = ':';
   }
   void trim_TCiString(TString &s) { // same string, i.e. s.begin() was used in outline_host()
      if (colon || slash)
         s.resize((colon? colon : slash) - s.data());
   }
};

typedef ::hash<int> inthash;

// utility function for viewer and debug output
TString PrintNSGroup(ui32 nsGroupId, sthash<int, const char *, inthash> *id2nsgroup, sthash<const char *, const char *, sgi_hash> *ip2nsname);

// url must not have scheme prefix (i.e. http://)
char *invertdomain(char *url);
