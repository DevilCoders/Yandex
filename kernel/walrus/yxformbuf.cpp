#include <cstdio>

#include <util/generic/algorithm.h>

#include <library/cpp/containers/mh_heap/mh_heap.h>
#include <library/cpp/deprecated/fgood/fput.h>

#include "yxformbuf.h"

using namespace NIndexerCore;

void PRHI_FileOrMem::Draw() {
    Y_ASSERT(f);
#   ifdef _unix_
    fseeko(f, f_start * sizeof(SUPERLONG), SEEK_SET);
#   else
    Y_ASSERT(f_start * sizeof(SUPERLONG) < 0x80000000);
    fseek(f, f_start * sizeof(SUPERLONG), SEEK_SET);
#   endif
    size_t num_to_read = Min(buf_len, f_end - f_start);
    size_t readSize = num_to_read * sizeof(SUPERLONG);
    size_t rd;
    if (readSize != (rd = fread(buf_start, 1, readSize, f)))
        ythrow yexception() << "read failed. handle: " << fileno(f) << " errno: " << errno << " result: " << rd << " to read: " << num_to_read * sizeof(SUPERLONG);
    //printf("Seek + read: %u + %u (%u + %u in bytes)\n", f_start, num_to_read, f_start * sizeof(SUPERLONG), num_to_read * sizeof(SUPERLONG));
    cur = buf_start, upper = buf_start + num_to_read;
    f_start += num_to_read;
    upper_is_end = f_start >= f_end;
}

//PRHI_FileOrMem::PRHI_FileOrMem() {}

template<class OutputIndexFile, class InvKeyWriter>
void YxFileWBLTmpl<OutputIndexFile, InvKeyWriter>::DumpPrev(ui32& prevOff, ui32 offset) {
    if (offset > prevOff) {
         if (TmpMutex)
             TmpMutex->Acquire();
         if (1 != fwrite(&pos_buffer[prevOff], (offset - prevOff) * sizeof(pos_buffer[0]), 1, f_buf_tmp))
             ythrow yexception() << "write failed. errno: " << errno;
         if (TmpMutex)
             TmpMutex->Release();
         prevOff = offset;
    }
}

template<class OutputIndexFile, class InvKeyWriter>
void YxFileWBLTmpl<OutputIndexFile, InvKeyWriter>::DumpSorted(TVector<ui32>::iterator& offset, ui32 prevOff, ui32 offset_count) {
     iters4heap.resize(offset_count);
     iters.resize(offset_count);
     ui32 iheap_size = 0;
     for (ui32 i = 0; i < offset_count; i++, offset++) {
         PRHI_FileOrMem& ph = iters4heap[iheap_size];
         iters[iheap_size++] = &ph;
         ui32 start = prevOff % pos_buffer_size;
         ph.RestartMem(&pos_buffer[0], start, start + (*offset - prevOff));
         prevOff = *offset;
     }
     MultHitHeap<PacketReadHitIterator> src_mhh(&iters[0], iheap_size);
     src_mhh.Restart();
     for (; src_mhh.Valid(); ++src_mhh)
         fput(f_buf_tmp, src_mhh.Current());
}

template<class OutputIndexFile, class InvKeyWriter>
void YxFileWBLTmpl<OutputIndexFile, InvKeyWriter>::DumpBuf() {
   if (!buf_offs)
       return;
   ui32 prevOff = 0;
   cur_offset += buf_offs;
   TVector<ui32>::iterator offset = offsets.begin() + cur_off;
   if (need_sort) {
      Y_ASSERT(!keys.empty());
      const char *key = keys.back().key;
      keys.push_back(KeyAndWBLIter(key, cur_offset));
//      memcpy(keys.back().ph.forms_map, keys[keys.size() - 2].ph.forms_map, sizeof(TNumFormArray));
      for (size_t n = buf_start_key; n < keys.size() - 1; n++) {
         size_t offset1 = keys[n].offset % pos_buffer_size;
         size_t offset2 = offset1 + (keys[n + 1].offset - keys[n].offset);
         Y_ASSERT(offset1 < offset2 && offset2 <= pos_buffer_size);
         struct timeval st, fn;
         gettimeofday(&st, nullptr);
         ui32 sorted_count = keys[n].sorted_count;
         if (sorted_count == 0)
              SortMt(&pos_buffer[0] + offset1, &pos_buffer[0] + offset2);
         else {
              DumpPrev(prevOff, offset1);
              DumpSorted(offset, keys[n].offset, sorted_count);
              prevOff = offset2;
         }
         gettimeofday(&fn, nullptr);
         ExternalSortHits += offset2 - offset1;
         ExternalSorts++;
         ExternalSortTime += fn.tv_sec * 1000000UL + fn.tv_usec - (st.tv_sec * 1000000UL + st.tv_usec);
         //printf("%s: sort [%x .. %x) (at + %x)\n", ~tmp_file_name, offset1, offset2, keys[n].offset);
      }
   }
   //printf("%s: write %x (%x bytes) (at %lx)\n", ~tmp_file_name, buf_offs, buf_offs * sizeof(pos_buffer[0]), ftell(f_buf_tmp));
   cur_off = offset - offsets.begin();
   DumpPrev(prevOff, buf_offs);
   fflush(f_buf_tmp);
   if (pos_buffer == &pos_buffer_m[0]) {
       pos_buffer_m.Destroy();
       pos_buffer_m.Create(pos_buffer_size);
       pos_buffer = &pos_buffer_m[0];
   }
   buf_start_key = keys.size() - 1;
   buf_offs = 0;
}

template<class OutputIndexFile, class InvKeyWriter>
void YxFileWBLTmpl<OutputIndexFile, InvKeyWriter>::SaveFormPosCollision(SUPERLONG prev_hit, TVector<int> &dforms) {
   TWordPosition w(prev_hit);
   dforms.push_back(w.Form());
   TVector<int>::iterator i = dforms.begin(), e = dforms.end();
   Sort(i, e);
   int prev_form = -1;
   for (; i != e; ++i) {
      if (form_pos_uniq_eff && *i == prev_form)
         continue;
      prev_form = *i;
      w.SetWordForm(*i);
      WriteHitThr(w.SuperLong());
   }
   dforms.clear();
}

template<class OutputIndexFile, class InvKeyWriter>
void YxFileWBLTmpl<OutputIndexFile, InvKeyWriter>::FlushLemma() {
    if (keys.empty()) {
       lem.PrepareForNextLemma();
       return; //
    }
    TVector<TOutputKey> outKeys;
    lem.ConstructOutKeys(outKeys,true);
    int *remap = nullptr;
    TVector<int> remapg(lem.GetTotalFormCount());
    TVector<int> remapk(lem.GetTotalFormCount());
    if (!no_forms) {
        remapv.resize(lem.GetTotalFormCount(), -1);
        remap = &remapv[0];
    }
    Y_ASSERT(outKeys.size());
    size_t iter_buf_sz = 0;
    const bool use_file = cur_offset != 0;
//    const bool two_pass = outKeys.size() > 20;
    if (form_pos_uniq)
       form_pos_uniq_eff = NeedRemoveHitDuplicates(outKeys[0].Key);
    if (use_file) {
        if (verbose)
            fprintf(stderr, "FlushLemma(\"%s%s\"): %lu -> %lu -> %lu [%s] disk %lu bytes\n",
              lem.GetCurLemma().szPrefix, lem.GetCurLemma().szLemma, (unsigned long)keys.size(), (unsigned long)lem.GetTotalFormCount(),
              (unsigned long)outKeys.size(), tmp_file_name.data(), (cur_offset + buf_offs) * (unsigned long)sizeof(SUPERLONG));
        Y_ASSERT(cur_offset >= pos_buffer_size);
        DumpBuf();
        iter_buf_sz = pos_buffer_size / keys.size();
        if (iter_buf_sz >= 512)
            iter_buf_sz &= ~511;
        else
            warnx("YxFileWBL::FlushLemma: buffer too small %lu / %lu = %lu", pos_buffer_size, keys.size(), iter_buf_sz);
        Y_ASSERT(iter_buf_sz);
    } else {
       if (verbose >= 3)
           fprintf(stderr, "FlushLemma(\"%s%s\"): %lu -> %lu -> %lu [%s] mem %lu bytes\n",
               lem.GetCurLemma().szPrefix, lem.GetCurLemma().szLemma,
               (unsigned long)keys.size(), (unsigned long)lem.GetTotalFormCount(), (unsigned long)outKeys.size(), tmp_file_name.data(),
               (unsigned long)(buf_offs * sizeof(SUPERLONG)));
       cur_offset = buf_offs;
    }
    iters4heap.resize(sorted_pieces + keys.size());
    iters.resize(sorted_pieces + keys.size());
    keyno_and_wpos kwp(0, 0);
    bool av = false;   // do not leave variables uninitialized (gcc61)
    TVector<ui32> hitsCount(outKeys.size());
    ui32 totalHitsCount = 0;
    TVector<bool> bigKeys(outKeys.size());
    bigKeys[0] = true;
    if (outKeys.size() > 1) {
       for (size_t k = 0; k != outKeys.size(); ++k) {
          hitsCount[k] = 0;
          bigKeys[k] = true;
          for (int i = 0; i < outKeys[k].KeyFormCount; ++i) {
             int gform = outKeys[k].FormIndexes[i];
             hitsCount[k] += lem.FormCount(gform);
          }
          totalHitsCount += hitsCount[k];
          for (int i = 0; i < outKeys[k].KeyFormCount; ++i) {
             int gform = outKeys[k].FormIndexes[i];
             remapg[gform] = i;
             remapk[gform] = k;
          }
       }
       for (size_t k = 0; k != outKeys.size(); ++k)
          bigKeys[k] = (hitsCount[k] * 2ULL > totalHitsCount);

//       TString tmp_file_name2 = tmp_file_name + "-2pass";
//       if (verbose)
//          fprintf(stderr, "FlushLemma (\"%s%s\") 2-pass: [%s *] disk\n", lem.GetCurLemma().szPrefix, lem.GetCurLemma().szLemma, ~tmp_file_name2);
       srt.Initialize();
    }

    ui32 kn = 0;

    SUPERLONG *cbuf = &pos_buffer[0];
    size_t iheap_size = 0;
    TVector<ui32>::iterator offset = offsets.begin();
    for (size_t n = 0; n < keys.size(); n++) {
        ui32 start = keys[n].offset;
        ui32 end = n + 1 != keys.size()? keys[n + 1].offset : cur_offset;
        if (start == end) // this can happen due to need_sort usage in DumpBuf
            continue;
        TNumFormArray* forms_map = &keys[n].forms_map;
        ui32 prevOff = start;
        ui32 sorted_count = keys[n].sorted_count;
        if (sorted_count == 0) {
            PRHI_FileOrMem& ph = iters4heap[iheap_size];
            iters[iheap_size++] = &ph;
            ph.forms_map = forms_map;
            if (use_file) {
                Y_ASSERT(end <= cur_offset);
                ph.RestartFile(f_buf_tmp, cbuf, iter_buf_sz, start, end);
                cbuf += iter_buf_sz;
                Y_ASSERT(cbuf <= &pos_buffer[0] + pos_buffer_size);
            } else {
                if (need_sort)
                    SortMt(&pos_buffer[start], &pos_buffer[end]);
                ph.RestartMem(&pos_buffer[0], start, end);
            }
            continue;
        }
        if (use_file) {
            Y_ASSERT(end <= cur_offset);
            PRHI_FileOrMem& ph = iters4heap[iheap_size];
            iters[iheap_size++] = &ph;
            ph.forms_map = forms_map;
            ph.RestartFile(f_buf_tmp, cbuf, iter_buf_sz, start, end);
            cbuf += iter_buf_sz;
            Y_ASSERT(cbuf <= &pos_buffer[0] + pos_buffer_size);
        }
        else {
            for (ui32 i = 0; i < sorted_count; i++, offset++) {
                PRHI_FileOrMem& ph = iters4heap[iheap_size];
                iters[iheap_size++] = &ph;
                ph.forms_map = forms_map;
                ph.RestartMem(&pos_buffer[0], prevOff, *offset);
                prevOff = *offset;
            }
        }
    }

    bool need_restart = false;
    for (TVector<TOutputKey>::const_iterator k = outKeys.begin(), e = outKeys.end(); k != e; ++k, ++kn) {
        Y_ASSERT(*k->Key);
        if (bigKeys[kn] || (kn == 0)) {
        if (need_restart) {
            for (ui32 i = 0; i < iheap_size; i++)
                iters4heap[i].Restart();
        }
//        first = false;
        if (verbose >= 4)
            fprintf(stderr, "iheap_size = %lu\n", (unsigned long)iheap_size);
        if (remap)
           for (int i = 0; i < k->KeyFormCount; ++i)
              remap[k->FormIndexes[i]] = i;
        MultHitHeap<PacketReadHitIterator> src_mhh(&iters[0], iheap_size);
        src_mhh.Restart();
        SUPERLONG prev_hit = ULL(-1);
        int local_hits_count = 0;
        TVector<int> dforms;
        ui32 thits = 0, whits = 0;
//        struct timeval st, fn;
//        gettimeofday(&st, NULL);
        for (; src_mhh.Valid(); ++src_mhh) {
            TWordPosition p(src_mhh.Current());
            thits++;
            if (remap) {
                int gform;
                int nForm = remap[gform = (*src_mhh.TopIter()->forms_map)[p.Form()]];
                if (nForm == -1) {
                     if (kn == 0) {
                         p.SetWordForm(remapg[gform]);
                         int outKey = remapk[gform];
                         if (!bigKeys[outKey])
                             srt.AddElement(keyno_and_wpos(outKey, p.SuperLong()));
                     }
                     continue;
                }

                whits++;
                p.SetWordForm(nForm);
                if ((p.SuperLong() ^ prev_hit) & ~((SUPERLONG)NFORM_LEVEL_Max)) { // Usual case, no collision
                   if (local_hits_count == 1)
                      WriteHitThr(prev_hit);
                   else if (local_hits_count > 1) // this path needs not be optimized
                      SaveFormPosCollision(prev_hit, dforms);
                   local_hits_count = 1;
                   prev_hit = p.SuperLong();
                } else { // Same lemma at the same position
                   dforms.push_back(nForm);
                   local_hits_count++;
                }
                continue;
            }
            if (form_pos_uniq_eff) {
               if (p.SuperLong() == prev_hit)
                  continue;

               prev_hit = p.SuperLong();
            }
            whits++;
            WriteHitThr(p.SuperLong());
        }
/*        if (thits >= 0x100000) {
            gettimeofday(&fn, NULL);
            fprintf(stderr, "writing %u elements from %u took %lu microseconds\n", whits, thits, fn.tv_sec * 1000000UL + fn.tv_usec - (st.tv_sec * 1000000UL + st.tv_usec));
        }*/
        if (remap) {
           if (local_hits_count == 1)
              WriteHitThr(prev_hit);
           else if (local_hits_count > 1) // this path needs not be optimized
              SaveFormPosCollision(prev_hit, dforms);
           for (int i = 0; i < k->KeyFormCount; ++i)
              remap[k->FormIndexes[i]] = -1;
        }
        WriteKeyThr(k->Key);
        if (WeightOutput) {
            WeightOutput->Write(&Weight, sizeof(Weight));
        }
        if (kn == 0) {
           srt.Finish();
           av = srt.NextNU(kwp);
        }
        } else {
           SUPERLONG prev_hit = ULL(-1);
           for (; av && (kwp.keyno == kn); av = srt.NextNU(kwp)) {
              if (form_pos_uniq_eff && kwp.pos == prev_hit)
                 continue;
              WriteHitThr(kwp.pos);
              prev_hit = kwp.pos;
           }
           WriteKeyThr(k->Key);
           if (WeightOutput) {
               WeightOutput->Write(&Weight, sizeof(Weight));
           }
        }
        need_restart = true;
    }
    //
   remapv.clear();
   lem.PrepareForNextLemma();
   kpool.restart();
   keys.clear();
   offsets.clear();
   if (use_file)
      fseek(f_buf_tmp, 0, SEEK_SET);
   no_forms = true;
   cur_offset = 0;
   cur_off = 0;
   if (buf_offs > pos_buffer_size_realloc) {
//      fprintf(stderr, "Reallocating buffer\n");
      if (pos_buffer == &pos_buffer_m[0]) {
         pos_buffer_m.Destroy();
         pos_buffer_m.Create(pos_buffer_size);
         pos_buffer = &pos_buffer_m[0];
      }
   }
   buf_offs = 0;
   buf_start_key = 0;
   sorted_pieces = 0;
   Weight = 0.;
}

template<class OutputIndexFile, class InvKeyWriter>
inline void YxFileWBLTmpl<OutputIndexFile, InvKeyWriter>::WriteHitThr(SUPERLONG hit) {
    if (Thr)
        Pipe.Put(hit);
    else
        Writer.WriteHit(hit);
}

template<class OutputIndexFile, class InvKeyWriter>
inline void YxFileWBLTmpl<OutputIndexFile, InvKeyWriter>::WriteKeyThr(const char* key) {
    if (Thr) {
        ui32 len = strlen(key);
        SUPERLONG len8 = (len >> 3) + 1;
        SUPERLONG* k = Pipe.Alloc(len8 + 2);
        k[0] = (SUPERLONG)-2;
        k[1] = len8;
        memcpy(k + 2, key, len + 1);
    }
    else
        Writer.WriteKey(key);
}

static YxFileWBL yx;
static YxFileWBLMemory yxm;
