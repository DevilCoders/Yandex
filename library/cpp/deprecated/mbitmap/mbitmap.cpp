#include "mbitmap.h"

#include <library/cpp/deprecated/fgood/fput.h>
#include <library/cpp/deprecated/fgood/fgood.h>
#include <util/stream/file.h>
#include <util/ysaveload.h>

#include <cstdio>
#include <cerrno>
#include <stdlib.h>

void bitmap_2::LoadDeletedDocuments(FILE* fportion, const ui8 val) {
    char buf[20];
    int i = 0;

    while (fgets(buf, sizeof(buf), fportion)) {
        i++;
        char* end;
        long handle = strtol(buf, &end, 0);
        if (handle < 0 || handle == LONG_MAX || (*end != '\n' && *end != '\r'))
            ythrow yexception() << "bad line from docdel: " << i;
        set(handle, val);
    }
    if (ferror(fportion))
        ythrow yexception() << "Can't read docdel: " << LastSystemErrorText();
}

ui32 bitmap_1::count(ui32 page) const {
    ui32* v = vect[page];
    if (!v)
        return 0;
    ui32 n, k;
    for (n = 0, k = 0; n < (ui32)bitmap_pagesize; n++) {
        ui32 u = v[n];
        u = (u >> 1 & 0x55555555) + (u & 0x55555555);
        u = (u >> 2 & 0x33333333) + (u & 0x33333333);
        u = (u >> 4 & 0x0f0f0f0f) + (u & 0x0f0f0f0f);
        u = (u >> 8 & 0x00ff00ff) + (u & 0x00ff00ff);
        k += (u >> 16) + (u & 0xffff);
    }
    return k;
}

ui32 bitmap_1::count() const {
    ui32 cnt = 0, n;
    for (n = 0; n < numpages(); n++)
        cnt += count(n);
    return cnt;
}

bool bitmap_1::set(ui32 id) {
    ui32 mask;
    mask = 1 << (id & 31);
    ui32 elh = id / (bitmap_pagesize * 32);
    id = id / 32 & (bitmap_pagesize - 1);
    if (elh >= vect.size()) { //resize
        vect.resize(elh + 2, (ui32*)nullptr);
    }
    if (!vect[elh]) {
        vect[elh] = new ui32[bitmap_pagesize];
        memset(vect[elh], 0, bitmap_pagesize * sizeof(ui32));
    }
    ui32* v = vect[elh];
    if (v[id] & mask)
        return false;
    v[id] |= mask;
    return true;
}

bool bitmap_1::reset(ui32 id) {
    ui32 mask;
    mask = 1 << (id & 31);
    ui32 elh = id / (bitmap_pagesize * 32);
    id = id / 32 & (bitmap_pagesize - 1);
    if (elh >= vect.size() || !vect[elh])
        return false;
    ui32* v = vect[elh];
    if (!(v[id] & mask))
        return false;
    v[id] &= ~mask;
    return true;
}

void bitmap_1::clear() {
    for (ui32 n = 0; n < vect.size(); n++)
        delete[] vect[n];
    vect.clear();
}

void bitmap_1::save(const char* f) const {
    TOFStream file(f);
    save(&file);
    file.Finish(); // We want an exception here not in dtor
}

void bitmap_1::save(IOutputStream* file) const noexcept {
    ui32 SIG = 0x42744D70, SubSIG = 1, Ver = 0;
    SaveMany(file, SIG, SubSIG, Ver);
    ui32 n, numfpages = 0, mp = 0, Count = 0;
    TVector<char> v(numpages(), (char)0);
    for (n = 0; n < numpages(); n++) {
        ui32 cnt = count(n);
        if (cnt)
            mp = n + 1, numfpages++, Count += cnt, v[n] = 1;
    }
    n = (mp + 3) & ~3;
    SaveMany(file, n, numfpages, mp, Count);
    for (n = 0; n < mp; n++)
        Save(file, v[n]);
    char p = 0;
    for (; n & 3; n++)
        Save(file, p);
    for (n = 0; n < numpages(); n++)
        if (v[n]) {
            file->Write(vect[n], sizeof(ui32) * bitmap_pagesize);
        }
}

void bitmap_1::load(const char* f) {
    TIFStream file(f);
    load(&file);
}

void bitmap_1::load(IInputStream* file, const char* dbg_fname) {
    ui32 SIG, SubSIG, Ver;
    ui32 bsz, numfpages, mp, Count, numfpages_chk = 0;

    LoadMany(file, SIG, SubSIG, Ver, bsz, numfpages, mp, Count);
    if (SIG != 0x42744D70 || SubSIG != 1 || Ver != 0)
        ythrow yexception() << dbg_fname << ": unexpected data in bitmap file header";
    clear();
    TVector<char> v(bsz);
    if (bsz != 0)
        file->LoadOrFail(&v[0], bsz);
    if (vect.size() < mp)
        vect.resize(mp, (ui32*)nullptr);
    for (ui32 n = 0; n < mp; n++)
        if (v[n]) {
            if (!vect[n])
                vect[n] = new ui32[bitmap_pagesize];
            file->LoadOrFail(vect[n], sizeof(ui32) * bitmap_pagesize);
            numfpages_chk++;
        }
    if (numfpages_chk != numfpages)
        ythrow yexception() << dbg_fname << ": bitmap file damaged";
}

bitmap_1_iterator::bitmap_1_iterator(const bitmap_1& v_)
    : v(v_)
{
    doc = 0;
    cur_page_end = 0;
    cur_page = (size_t)-1;
    cur_page_ptr = nullptr;
    pptr = eptr = nullptr;
    num_pages = v.numpages();
    if (!next_page())
        return;
    if (!(cur_mask & 1))
        operator++();
}

bool bitmap_1_iterator::next_page() {
    cur_page++;
    while (true) {
        while (cur_page < num_pages && !v.vect[cur_page])
            cur_page++;
        if (cur_page == num_pages)
            return false;
        cur_page_ptr = v.vect[cur_page];
        pptr = cur_page_ptr, eptr = pptr + bitmap_pagesize;
        for (; pptr < eptr && !*pptr; pptr++) {
        }
        if (pptr < eptr) {
            doc = cur_page * bitmap_1::elems_per_page + (pptr - cur_page_ptr) * 32;
            cur_mask = *pptr;
            break;
        }
        cur_page++;
    }
    return true;
}
