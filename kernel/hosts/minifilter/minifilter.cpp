#include <stdio.h>
#include <stdlib.h>

#include <util/stream/file.h>
#include <util/str_stl.h>
#include <util/string/printf.h>
#include <library/cpp/deprecated/fgood/fgood.h>
#include <library/cpp/deprecated/fgood/ffb.h>
#include <util/memory/segmented_string_pool.h>
#include <library/cpp/deprecated/mbitmap/mbitmap.h>

#include "hostnamehash.h"
#include "minifilter.h"
#include <util/string/split.h>

using namespace std;

static const Tr tr_bkw("\005\004\003\002\001", ".:/?#");
static const Tr tr_fwd(".:/?#", "\005\004\003\002\001");

void TRSPCHARS(char *s) {
   tr_fwd.Do(s);
}
void UNTRSPCHARS(char *s) {
   tr_bkw.Do(s);
}
void TRSPCHARS(char *s, size_t l) {
   tr_fwd.Do(s, l);
}
void UNTRSPCHARS(char *s, size_t l) {
   tr_bkw.Do(s, l);
}
void TRSPCHARS(const char *s, char *d) {
   tr_fwd.Do(s, d);
}
void UNTRSPCHARS(const char *s, char *d) {
   tr_bkw.Do(s, d);
}

MiniFilter::MiniFilter(const char *delims) : p(128 * 1024) {
   // also consider: str_spn::init
   memset(PathDelim, 0, sizeof(PathDelim));
   for(; *delims; delims++)
      PathDelim[(ui8)*delims] = 1;
}
MiniFilter::~MiniFilter() {}

#define UC_ALIGN(p, T) (T*)(((uintptr_t)(p) + (sizeof(T) - 1)) & ~(uintptr_t)(sizeof(T) - 1))
#define UC_ALIGN_OFFS(p, T) (((uintptr_t)(p) + (sizeof(T) - 1)) & ~(uintptr_t)(sizeof(T) - 1))

// return value will become invalid after next Find() call
ui32 *MiniFilter::Find(const char *url)
{
   return Find(url, Cache);
}

// returned value points into cache
ui32 *MiniFilter::Find(const char *url, FindCache& cache) const
{
   char *slash = const_cast<char*>(strchr(url, '/'));
   if (slash) {
      if (slash - url >= HOST_MAX) return nullptr;
      *slash = 0;
   } else {
      if (strlen(url) >= HOST_MAX) return nullptr;
   }
   char *colon = const_cast<char*>(strchr(url, ':'));
   if (colon) *colon = 0;
   if (!cache.cur_host || strcmp(cache.cur_host, url)) {
      Thost_map::const_iterator i = host_map.find(url);
      cache.curr_path_tables.clear();
      if (i == host_map.end()) {
         cache.cur_host = strcpy(cache.cur_host_buf, url);
         cache.curr_host_table = nullptr;
      } else {
         cache.cur_host = i->first;
         const char *hostname_end = cache.cur_host + strlen(cache.cur_host) + 1;
         cache.curr_host_table = UC_ALIGN(hostname_end, size_t);
      }
   }
   if (colon) *colon = ':';
   if (slash) *slash = '/';
   if (!cache.curr_host_table)
      return nullptr;
   const char *path = slash? slash : "/";
   if (!cache.curr_path_tables.empty()) {
      // Check if we are within the same path as previous url
      for(vector<MF_PathSeg>::iterator i = cache.curr_path_tables.begin(); i != cache.curr_path_tables.end(); ++i) {
         // Check if path segment matches
         if (strncmp(i->str, path, i->str_len)) {
            cache.curr_path_tables.erase(i, cache.curr_path_tables.end());
            break;
         }
         // Check if path segment is terminated with correct delimiter symbol.
         // At construction time, it is assured that t may be null only for leafs
         char pt = path[i->str_len], t = i->terminator;
         if (pt && (t && pt != t || !t && !PathDelim[(ui8)pt])) {
            cache.curr_path_tables.erase(i, cache.curr_path_tables.end());
            break;
         }
         path += i->str_len;
         // Skip delimiter symbol (even if there's many of them)
         if (t) while (*path == t) path++;
      }
   }
   size_t num_children;
   const char **children;
   if (cache.curr_path_tables.empty()) {
      // Search will start from host's base
      num_children = *cache.curr_host_table;
      children = (const char**)(cache.curr_host_table + 1);
   } else {
      // We still have some matching segments
      num_children = cache.curr_path_tables.back().num_children;
      children = cache.curr_path_tables.back().children;
   }
   // Actual search is here
   // TODO: negative search results should also be cached
   while(*path) {
      if (!num_children) break;
      // Children are sorted, TODO: use std::lower_bound
      size_t num_children_new = 0;
      const char **children_new = nullptr;
      bool has_nt_neighb = false;
      for(size_t n = 0; n < num_children; n++) {
         Y_ASSERT(*children[n]);
         size_t c_len = strlen(children[n]) - 1;
         if (strncmp(children[n], path, c_len))
            continue;
         // Note: similar logic is 30 lines above
         char pt = path[c_len], t = children[n][c_len];
         t = t == 1? 0 : t;
         if (pt && (t && pt != t || !t && !PathDelim[(ui8)pt]))
            continue;
         if (t) {
            path += c_len;
            while (*path == t) path++;
         }

         // data unpack
         ui8 *u_data = (ui8*)children[n] + c_len + 2;
         size_t *us_data = UC_ALIGN(u_data + 1, size_t);
         ui32 data = (*u_data >> 4) & 7;
         bool is_leaf = (*u_data & 128) != 0;
         num_children_new = *u_data & 15;
         if (data == 7)
            data = (ui32)-1;
         else if (data == 6)
            data = (ui32) *us_data++;
         if (num_children_new == 15)
            num_children_new = *us_data++;
         children_new = (const char**)us_data;
         //Y_ASSERT(t || !num_children_new);

         if (has_nt_neighb) {
            ui32 prev_data = cache.curr_path_tables.back().data;
            cache.curr_path_tables.pop_back();
            if (!is_leaf)
               is_leaf = true, data = prev_data;
            has_nt_neighb = false;
         }
         MF_PathSeg ctmp = { children[n], c_len, t, is_leaf, data, num_children_new, children_new };
         cache.curr_path_tables.push_back(ctmp);
         if (t)
            break;
         Y_ASSERT(!has_nt_neighb);
         Y_ASSERT(is_leaf);
         has_nt_neighb = true;
      }
      if (has_nt_neighb)
            path += cache.curr_path_tables.back().str_len;
      children = children_new;
      num_children = num_children_new;
   }
   if (cache.curr_path_tables.empty()) {
      return nullptr;
   } else {
      for(vector<MF_PathSeg>::iterator i = cache.curr_path_tables.end() - 1; ; --i) {
         if (i->is_leaf)
            return &i->data;
         if (i == cache.curr_path_tables.begin())
            break;
      }
      return nullptr;
   }
}

// note: adding to this list may require revieving code that calls TRSPCHARS
str_spn path_delimeters("/?");

size_t MiniFilter::GetChildCount(MF_PathData *paths_b, MF_PathData *paths_e, size_t child_offs) {
   if (paths_e - paths_b <= 1)
      return paths_e - paths_b;
   char *cur_child = paths_b++->str + child_offs;
   char *cur_child_end = path_delimeters.brk(cur_child);
   size_t num = 1;
   for(; paths_b != paths_e; paths_b++) {
      char *cur = paths_b->str + child_offs;
      char *end = path_delimeters.brk(cur);
      if (end - cur != cur_child_end - cur_child || memcmp(cur_child, cur, end - cur + 1)) {
         cur_child = cur;
         cur_child_end = end;
         if (*end || end - cur)
            num++;
      }
   }
   return num;
}

void MiniFilter::AddChildren(const char **chld_ptr, MF_PathData *paths_b, MF_PathData *paths_e, size_t child_offs) {
   if (paths_b == paths_e) return; // m.b. Y_ASSERT
   char *cur_child = paths_b->str + child_offs;
   char *cur_child_end = path_delimeters.brk(cur_child);
   MF_PathData *cur_child_seq = paths_b++;
   bool cur_is_leaf = !*cur_child_end || !cur_child_end[1];
   for(; paths_b <= paths_e; paths_b++) {
      char *cur = nullptr, *end = nullptr;
      if (paths_b != paths_e) {
         cur = paths_b->str + child_offs;
         end = path_delimeters.brk(cur);
      }
      if (paths_b == paths_e || end - cur != cur_child_end - cur_child || memcmp(cur_child, cur, end - cur + 1)) {

         size_t next_offs = child_offs + cur_child_end - cur_child + 1;
         // if child is non-terminated, it actually can not have children
         // TODO: this Y_ASSERT seems to be incorrect. Fix it
         //Y_ASSERT(*cur_child_end || paths_b - cur_child_seq == 1);
         size_t numchld = GetChildCount(cur_child_seq + !*cur_child_end, paths_b, next_offs);
         size_t numbytes = cur_child_end - cur_child + 3 + sizeof(size_t) + numchld * sizeof(char*);
         numbytes = UC_ALIGN_OFFS(numbytes, size_t);
         if (numchld >= 15) numbytes += sizeof(size_t);
         // allocate buffer and put string, also put reference to parent's table
         char *dst_ptr = p.append(nullptr, numbytes);
         memcpy(dst_ptr, cur_child, cur_child_end - cur_child + 1);
         *chld_ptr++ = dst_ptr;
         dst_ptr += cur_child_end - cur_child;
         // we append \001 if there was no 'delimiter' at the end of string
         if (!*cur_child_end) *dst_ptr = 1;
         *++dst_ptr = 0;
         dst_ptr++;
         // pack data (currently 0 or 1) and number of child's children
         ui32 data = cur_child_seq->data;
         *dst_ptr = char(((data == (ui32)-1? 7 : data >= 6? 6 : data) << 4) |
                    (cur_is_leaf << 7) | (numchld >= 15? 15 : numchld));
         dst_ptr++;
         size_t *dst_ptr_u = UC_ALIGN(dst_ptr, size_t);
         if (data != (ui32)-1 && data >= 6) *dst_ptr_u++ = data;
         if (numchld >= 15) *dst_ptr_u++ = numchld;
         AddChildren((const char**)dst_ptr_u, cur_child_seq + !*cur_child_end, paths_b, next_offs);

         cur_child = cur;
         cur_child_end = end;
         cur_child_seq = paths_b;
         cur_is_leaf = false;
      }
      if (paths_b != paths_e && (!*end || !end[1]))
         cur_is_leaf = true;
   }
}

void MiniFilter::AddHostPaths(const char *host, vector<MF_PathData> &paths) {
   // sort paths so that they are correctly aligned ;)
   for(size_t n = 0; n < paths.size(); n++)
      TRSPCHARS(paths[n].str);
   sort(paths.begin(), paths.end());
   for(size_t n = 0; n < paths.size(); n++)
      UNTRSPCHARS(paths[n].str);
   size_t numchld = GetChildCount(&*paths.begin(), &*paths.end(), 0);
   Y_ASSERT(sizeof(size_t) == sizeof(char*)); //??
   size_t numbytes = strlen(host) + 1 + sizeof(size_t) + numchld * sizeof(char*);
   numbytes = UC_ALIGN_OFFS(numbytes, size_t);
   char *dst_ptr = p.append(nullptr, numbytes);
   strcpy(dst_ptr, host);
   if (host_map.insert(Thost_map::value_type(dst_ptr, 0)).second != true)
      ythrow yexception() << "File " <<  FileName.data() << ": bad sort order, host " <<  host << " was met again";
   size_t *dst_ptr_u = UC_ALIGN(dst_ptr + strlen(host) + 1, size_t);
   *dst_ptr_u++ = numchld;
   AddChildren((const char**)dst_ptr_u, &*paths.begin(), &*paths.end(), 0);
}

void MiniFilter::LoadFlat(const char *fname, int mode) {
   TString buf;
   char *fb[32];
   char cur_host[HOST_MAX]; *cur_host = 0;
   segmented_pool<char> p_tmp(32 * 1024);
   vector<MF_PathData> paths;
   TFileInput f(fname);
   FileName = fname;
   int line = 0;
   while(f.ReadLine(buf)) {
      line++;
      int nf = sf(fb, buf.begin());
      if (nf != 1 + (mode == 2) && nf != 2 + (mode == 2))
         ythrow yexception() << "File " <<  fname << " has " <<  nf << " fields, must be " <<  1 + (mode == 2) << " or " <<  2 + (mode == 2) << "";
      char *slash = strchr(fb[0], '/');
      if (nf == 1) {
         if (slash) *slash = 0;
      } else {
         if (slash)
            ythrow yexception() << "File " <<  fname << ": when 2 fields, first one must be host name";
      }
      if (strlen(fb[0]) >= HOST_MAX) // ignore apparent junk
         continue;
      if (strcmp(cur_host, fb[0])) {
         if (*cur_host)
            AddHostPaths(cur_host, paths);
         p_tmp.restart();
         paths.clear();
         strcpy(cur_host, fb[0]);
      }
      if (slash) *slash = '/';
      const char *path = nf == 1? slash : fb[1];
      if (!path || !*path) path = "/";
      ui32 data = mode == 0? 0 : mode == 1? line : atoi(fb[nf - 1]);
      char *path_a = strcpy(p_tmp.append(nullptr, strlen(path) + 2), path);
      paths.push_back(MF_PathData(path_a, data));
   }
   if (*cur_host)
      AddHostPaths(cur_host, paths);
}

int testminifilter() {
   MiniFilter mf;
   mf.LoadFlat("minifilter.test", 1);
   char buf[2048];
   while(fgets(buf, sizeof(buf), stdin)) {
      chomp(buf);
      ui32 *rv = mf.Find(buf);
      if (!rv) printf("%s -> [Not Found]\n", buf);
      else printf("%s -> %i\n", buf, (i32)*rv);
   }
   return 0;
}

// ************* domain level/owner/spam testing functions *************

ui32 host_or_domain(hostmap &h, const char *host) {
   ui32 id;
   if ((id = h.HostByName(host)) != (ui32)-1) return id;
   int l = 1;
   while((host = strchr(host, '.')) != nullptr) {
      host++, l++;
      if (*host == '.' || !*host || !strchr(host, '.')) break;
      if ((id = h.HostByName(host)) != (ui32)-1) return id;
   }
   return (ui32)-1;
}

ui32 host_or_domain(const TMappedHostHash &h, const char *host) {
   ui32 id;
   if ((id = h.HostByName(host)) != (ui32)-1) return id;
   int l = 1;
   while((host = strchr(host, '.')) != nullptr) {
      host++, l++;
      if (*host == '.' || !*host || !strchr(host, '.')) break;
      if ((id = h.HostByName(host)) != (ui32)-1) return id;
   }
   return (ui32)-1;
}

// in h, subdomain of any domain in h
int is_ban_host_or_domain(hostmap &h, const char *&host) {
   if (h.HostByName(host) != (ui32)-1) return 1;
   int l = 1;
   while((host = strchr(host, '.')) != nullptr) {
      host++, l++;
      if (*host == '.' || !*host || !strchr(host, '.')) break;
      if (h.HostByName(host) != (ui32)-1) return l;
   }
   host = nullptr;
   return 0;
}

// in h, subdomain of any domain in h
int is_ban_host_or_domain(const TMappedHostHash &h, const char *&host) {
   if (h.has(host)) return 1;
   int l = 1;
   while((host = strchr(host, '.')) != nullptr) {
      host++, l++;
      if (*host == '.' || !*host || !strchr(host, '.')) break;
      if (h.has(host)) return l;
   }
   host = nullptr;
   return 0;
}

// in h, subdomain of any domain in h
int is_ban_host_or_domain_c(const hostset &h, const char *host) {
   if (h.contains(host)) return 1;
   int l = 1;
   while((host = strchr(host, '.')) != nullptr) {
      host++, l++;
      if (*host == '.' || !*host || !strchr(host, '.')) break;
      if (h.contains(host)) return l;
   }
   return 0;
}

int is_ban_host_or_domain_c(const TMappedHostHash &h, const char *host) {
   if (h.has(host)) return 1;
   int l = 1;
   while((host = strchr(host, '.')) != nullptr) {
      host++, l++;
      if (*host == '.' || !*host || !strchr(host, '.')) break;
      if (h.has(host)) return l;
   }
   return 0;
}

// subdomain of any domain in h (can not be in h itself)
const char *area_subdomain(const hostset &h, const char *host) {
   if (h.contains(host)) return nullptr;
   const char *host_sub = host;
   while((host = strchr(host, '.')) != nullptr) {
      host++;
      if (*host == '.' || !*host || !strchr(host, '.')) break;
      if (h.contains(host)) return host_sub;
      host_sub = host;
   }
   return nullptr;
}

// subdomain of any domain in h (can be in h, if h contains domain and its subdomains)
const char *area_subdomain_r(const hostset &h, const char *host) {
   const char *host_sub = host;
   while((host = strchr(host, '.')) != nullptr) {
      host++;
      if (*host == '.' || !*host || !strchr(host, '.')) break;
      if (h.contains(host)) return host_sub;
      host_sub = host;
   }
   return nullptr;
}

// ********************* THost2Str *********************  //
segmented_string_pool sts_pool;

template<class C>
inline typename C::const_iterator find_domain(const C &c,const char *host, bool HasTld) {
   typename C::const_iterator i = c.find(host);
   if (i != c.end()) return i;
   while((host = strchr(host, '.')) != nullptr) {
      host++;
      if (*host == '.' || !*host || !HasTld && !strchr(host, '.')) break;
      i = c.find(host);
      if (i != c.end()) return i;
   }
   return c.end();
}

THost2Str::const_iterator THost2Str::find_domain(const char *host) const {
   return ::find_domain(*this, host, HasTld);
}

hostset::const_iterator hostset::find_domain(const char *host) const {
   return ::find_domain(*this, host, HasTld);
}

void load_txt_file(THashMap<int, const char *> &h, const char *fname, segmented_string_pool &p) {
   for (const auto& line : TMapTsvFile(fname, 2)) {
      int num;
      if (!TryFromString(line[0], num))
          ythrow yexception() << "" << fname << ": field " << 0 << " is not a number: '" << line[0] << "'";
      if (!h.insert(typename THashMap<int, const char *>::value_type(num, p.Append(line[1]))).second)
         p.undo_last_append();
   }
}

void load_txt_file(THashMap<int, int> &h, const char *fname) {
   for (const auto& line : TMapTsvFile(fname, 2)) {
      int num0, num1;
      if (!TryFromString(line[0], num0))
          ythrow yexception() << "" << fname << ": field " << 0 << " is not a number: '" << line.front() << "'";
      if (!TryFromString(line[1], num1))
          ythrow yexception() << "" << fname << ": field " << 1 << " is not a number: '" << line.back() << "'";
      h.insert(std::make_pair(num0, num1));
   }
}

inline bool SplitLineWasEmpty(const TVector<TStringBuf>& fields) {
    return fields.empty() || (fields.size() == 1 && fields.front().empty());
}

void hostset::load(const char *fname, segmented_string_pool &p, bool strict_fmt, bool lower_case) {
   for (const auto& line : TMapTsvFile(fname, strict_fmt ? 1 : -1)) {
       if (!strict_fmt && SplitLineWasEmpty(line))
           continue;
       char *key = p.Append(line.front());
       if (lower_case)
           strlwr(key);
       if (!insert(hostset::value_type(key, 1)).second)
         p.undo_last_append();
   }
}

void hostset::loadf(const char *fname, int kfld, int vfld, segmented_string_pool &p, bool strict_fmt, bool lower_case, int vfdefv) {
   for (const auto& line : TMapTsvFile(fname)) {
      const int fields_number = line.size();
      if (kfld >= fields_number)
          ythrow yexception() << "" <<  fname << ": must be at least " <<  kfld + 1 << " fields, got " <<  fields_number << ": '" <<  line.front() << "'";
      if (vfld >= fields_number && strict_fmt)
          ythrow yexception() << "" <<  fname << ": must be at least " <<  vfld + 1 << " fields, got " <<  fields_number << ": '" <<  line.front() << "'";
      char *key = p.Append(line[kfld]);
      if (lower_case)
          strlwr(key);
      const int val = vfld >= fields_number ? vfdefv : FromString<int>(line[vfld]);
      if (!insert(hostset::value_type(key, val)).second)
         p.undo_last_append();
   }
}

namespace {

struct SpaceSplitter {
    static void LineToFieldVec(TStringBuf line, TVector<TStringBuf> *resultFields) {
        StringSplitter(line).Split(' ').AddTo(resultFields);
    }
};

}

void hostset::load2(const char *fname, segmented_string_pool &p) {
   for (const auto& line : TMapSvFile<SpaceSplitter>(fname)) {
      if (!line.empty() && !line.front().empty() && line.front()[0] != ';' && line.front()[0] != '#')
          insert(hostset::value_type(strlwr(p.Append(line.front())), 1));
   }
}

void read_ids_to_bitmap(bitmap_1 &dst, const char *fname, const char *fname2) {
   const char *fnames[2] = { fname, fname2 };
   for (int n = 0; n < 2; n++) if (fnames[n]) {
      for (const auto& line : TMapSvFile<SpaceSplitter>(fnames[n])) {
         if (SplitLineWasEmpty(line))
             continue;
         int val;
         if (TryFromString(line[0], val))
            dst.set(val);
      }
   }
}

TString PrintNSGroup(ui32 nsGroupId, sthash<int, const char *, inthash> *id2nsgroup, sthash<const char *, const char *, sgi_hash> *ip2nsname) {
   TString name_servers;
   sthash<int, const char *, inthash>::const_iterator nsi = id2nsgroup->find(nsGroupId);
   if (nsi != id2nsgroup->end()) {
      TString tmp(nsi.Value());
      char *nservers[32];
      int num_nservers = sf(' ', nservers, tmp.begin());
      if (num_nservers)
         name_servers = ": ";
      for (int n = 0; n < num_nservers; n++) {
         name_servers += nservers[n];
         sthash<const char *, const char *, sgi_hash>::const_iterator ipi = ip2nsname->find(nservers[n]);
         if (ipi != ip2nsname->end())
            fcat(name_servers, " (%s)", ipi.Value());
         name_servers += ", ";
      }
      if (name_servers.size() > 2) name_servers.resize(name_servers.size() - 2);
   }
   return name_servers;
}

char *invertdomain(char *url) {
   // no scheme cut-off, no 80th port normalization
   char *sl = strchr(url, '/');
   if (sl) *sl = 0;
   char *cl = strchr(url, ':');
   if (cl) *cl = 0;
   char *end = cl? cl : sl? sl : url + strlen(url);
   // invert string
   for (char *s = url, *e = end - 1; s < e; s++, e--)
      std::swap(*s, *e);
   // invert back each host name segment
   for (char *s = url, *e1 = strchr(s, '.'); ; e1 = strchr(s = e1 + 1, '.')) {
      for (char *e = (e1? e1 : end) - 1; s < e; s++, e--)
         std::swap(*s, *e);
      if (!e1) break;
   }
   if (cl) *cl = ':';
   if (sl) *sl = '/';
   return url;
}
