/*
 */

#include <kernel/search_types/search_types.h>
#include <cstring>

#include <util/generic/vector.h>
#include <util/memory/segmented_string_pool.h>

#include "seqreader.h"

//------------------------------------------------------------//
//                      TKeysPool
//------------------------------------------------------------//

class TKeysPool
{
private:
   typedef TVector<YxKey> TYxKeys;
   TYxKeys YxKeys;
   segmented_pool<char> KeysPool;
   TYxKeys::iterator curKey;
   ui32 HitsMapSize;
public:
   SUPERLONG Offset;
   ui32 Length;

public:
   TKeysPool(ui32 mapSize, size_t segsize) : KeysPool(segsize), HitsMapSize(mapSize)
   {
      YxKeys.reserve(segsize/MAXKEY_BUF);
      Offset = -1;
      Length = 0;
      curKey = YxKeys.end();
   }
   //returns 0 if OK, returns 1 if map size is overflow
   // ATTENTION! Invalidates curKey, needs Restart().
   int Attach(const YxRecord &e, const TSubIndexInfo &si) {
      if (Offset < 0)
         Offset = e.Offset;
      ui32 eLen = hitsSize(e.Offset, e.Length, e.Counter, si);
      i64 off = e.Offset - Offset + eLen;
      ui32 len = ui32(off);
      assert(len == off);
      if (!YxKeys.empty() && (ui32)len > HitsMapSize)
         return 1;
      Length = len;
      size_t n = strlen(e.TextPointer);
      char* text = KeysPool.append(e.TextPointer, n + 2); // + 2 - after applying format it can grow for 1 character

      YxKey k;
      k.Text = text;
      k.Length = e.Length;
      k.Offset = e.Offset;
      k.Counter = e.Counter;
      YxKeys.push_back(k);
      return 0;
   }

   bool Valid() const {
      return curKey != YxKeys.end();
   }
   void Next() {
      ++curKey;
   }
   const YxKey &CurKey() const {
      assert(Valid());
      return *curKey;
   }
   void Flush() {
      YxKeys.erase(YxKeys.begin(), YxKeys.end());
      KeysPool.restart();
      Offset = -1;
      Length = 0;
      curKey = YxKeys.end();
   }
   void Restart() {
      curKey = YxKeys.begin();
   }
};

//------------------------------------------------------------//
//                     TSequentKeyPosReader
//------------------------------------------------------------//

TSequentKeyPosReader::TSequentKeyPosReader(const IKeysAndPositions *y, const char *keyLow, const char *keyHigh,
                                                          ui32 mapSize, size_t segsize)
  : Yndex(y)
  , KPool(new TKeysPool(mapSize, segsize))
  , LowNumKey(-1)
  , HighNumKey(-1)
  , CurNumKey(-1)
  , YBlock(-1)
  , KeyLow(keyLow)
  , KeyHigh(keyHigh)
{
   // Restart();  -- not pertinent in constructor
}

TSequentKeyPosReader::~TSequentKeyPosReader()
{
}

void TSequentKeyPosReader::Init(const IKeysAndPositions *y, const char *keyPreffix,
                           ui32 mapSize, size_t segsize)
{
   char highKey[MAXKEY_BUF*2];
   strlcpy(highKey, keyPreffix, MAXKEY_BUF);
   size_t l = strlen(highKey);
   memset(highKey + l, '\xff', MAXKEY_BUF-1);
   highKey[l+MAXKEY_BUF-1] = 0;
   Init(y, keyPreffix, highKey, mapSize, segsize);
}

void TSequentKeyPosReader::Init(const IKeysAndPositions *y, const char *keyLow,
                           const char *keyHigh, ui32 mapSize, size_t segsize)
{
   Yndex = y;
   KeyLow = keyLow;
   KeyHigh = keyHigh;
   KPool.Reset(new TKeysPool(mapSize, segsize));
   Restart();
}

void TSequentKeyPosReader::Restart() {
   LowNumKey =  Yndex->LowerBound(KeyLow.data(), RC);
   HighNumKey = Yndex->LowerBound(KeyHigh.data(), RC);
   CurNumKey = LowNumKey;
   YBlock = -1;
   NextPortion();
}

void TSequentKeyPosReader::NextPortion() {
   KPool->Flush();
   SuperHits.Drop();
   for (; CurNumKey < HighNumKey; ++CurNumKey) {
      const YxRecord *e = Yndex->EntryByNumber(RC, CurNumKey, YBlock);
      if (!(e && e->TextPointer[0]))
         continue;
      if (KPool->Attach(*e, Yndex->GetSubIndexInfo()))
         break;
   }
   KPool->Restart();
   if (Valid()) {
       Yndex->GetBlob(SuperHits, KPool->Offset, KPool->Length, RH_DEFAULT);
   }
}

bool TSequentKeyPosReader::Valid() const {
   return CurNumKey < HighNumKey || KPool->Valid();
}

void TSequentKeyPosReader::Next() {
   KPool->Next();
   if (!KPool->Valid())
        NextPortion();
}

const YxKey &TSequentKeyPosReader::CurKey() const {
   return KPool->CurKey();
}

void TSequentKeyPosReader::InitIterator(TPosIterator<>& iter) {
   if (Valid()) {
      const YxKey& yk = KPool->CurKey();
      iter.Init(Yndex->GetIndexInfo(), SuperHits, KPool->Offset, yk.Offset, yk.Length, yk.Counter);
   }
}

void TSequentYandReader::Init(const char* file, const char *keyPreffix, ui32 mapSize, size_t segsize, READ_HITS_TYPE defaultReadHitsType/* = RH_DEFAULT*/) {
    Yndex.Reset(new TYndex4Searching);
    Yndex->InitSearch(file, defaultReadHitsType);
    TSequentKeyPosReader::Init(Yndex.Get(), keyPreffix, mapSize, segsize);
}

void TSequentYandReader::Init(const char* file, const char *keyLow, const char *keyHigh, ui32 mapSize, size_t segsize, READ_HITS_TYPE defaultReadHitsType/* = RH_DEFAULT*/) {
    Yndex.Reset(new TYndex4Searching);
    Yndex->InitSearch(file, defaultReadHitsType);
    TSequentKeyPosReader::Init(Yndex.Get(), keyLow, keyHigh, mapSize, segsize);
}
