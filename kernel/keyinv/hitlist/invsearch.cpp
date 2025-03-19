#include <kernel/search_types/search_types.h>
#include <util/system/defaults.h>
#include <util/generic/string.h>
#include <util/generic/yexception.h>
#include <kernel/keyinv/invkeypos/keychars.h>
#include <kernel/keyinv/invkeypos/keycode.h>

#include "invsearch.h"
#include "positerator.h"

void GetLemmaInterval(const IKeysAndPositions* y, TRequestContext &rc, ui64 keyPrefix, const TString& lemma, ui32 maxInterval,
                      TRecordInterval* interval, TLowerBoundCache *lowerBoundCache)
{
    *interval = TRecordInterval();
    if (rc.GetCachedInterval(keyPrefix, lemma, maxInterval, *interval))
        return;
    rc.ClearInterval();

    size_t searchLen = lemma.size();
    if (searchLen && lemma.back() == TITLECASECOLLATOR)
        searchLen--;

    Y_ASSERT(lemma.size()); // TODO: add punctuation processing
    TString prefixedSearchForLow;
    prefixedSearchForLow.reserve(1 + MAXKEY_BUF + searchLen + 1);
    if (keyPrefix) {
        char buf[MAXKEY_BUF];
        int prefixSize = EncodePrefix(keyPrefix, buf);
        prefixedSearchForLow.append(buf, prefixSize);
        prefixedSearchForLow += KEY_PREFIX_DELIM;
    }
    prefixedSearchForLow.append(lemma.data(), searchLen);
    TString szSearchForHigh(prefixedSearchForLow.data(), prefixedSearchForLow.size()); // prevent stupid refcounting, it will be changed
    szSearchForHigh += '\x0F';

    if (lowerBoundCache && !lowerBoundCache->LastCmp.empty() && lowerBoundCache->Num == rc.GetKeyNumber() && lowerBoundCache->Block == rc.GetBlockNumber()) {
        if (lowerBoundCache->LastCmp <= prefixedSearchForLow) {
            const YxRecord *record = &rc.GetRecord();
            if (TRequestContext::CompareKeyWithLemma(record->TextPointer, szSearchForHigh.data()) >= 0) {
                rc.SetCachedInterval(keyPrefix, lemma, maxInterval, *interval);
                return;
            }
        }
    }

    i32 num = y->LowerBound(prefixedSearchForLow.data(), rc);
    i32 block = rc.GetBlockNumber(); // sequential access to key file

    if (lowerBoundCache) {
        lowerBoundCache->Num = num;
        lowerBoundCache->Block = block;
        lowerBoundCache->LastCmp = prefixedSearchForLow;
    }

    TSubIndexInfo subIndexInfo = y->GetSubIndexInfo();
    bool isFirst = true;
    rc.ClearInterval();
    for (;; ++num) {
        const YxRecord *record = y->EntryByNumber(rc, num, block);
        if (!record)
            break;
        if (TRequestContext::CompareKeyWithLemma(record->TextPointer, prefixedSearchForLow.data()) < 0) {
            if (!isFirst) {
                Cerr << "unordered keys '" << record->TextPointer << "' '" << prefixedSearchForLow << "' " << num << " " << interval->FirstNumber << " " << isFirst << Endl;
                Y_ASSERT(false);
                ythrow TUnorderedKeysException() << "unordered keys '" << record->TextPointer << "' '" << prefixedSearchForLow << "'";
            }
            continue; // this should not happen imho but it does
        }
        if (TRequestContext::CompareKeyWithLemma(record->TextPointer, szSearchForHigh.data()) >= 0)
            break;
        if (isFirst) {
            interval->FirstBlock = block;
            interval->FirstNumber = num;
            interval->StartOffset = record->Offset;
            isFirst = false;
        } else {
            if ((ui64)num > (ui64)maxInterval + (ui64)interval->FirstNumber) {
                Cerr << "max interval restriction in loadword for '" << lemma << "' " << interval->FirstNumber << " " << maxInterval << " " << num << Endl;
                break;
            }
        }
        rc.AddRecordToInterval(*record);
        if (interval->TotalCounter < INT_MAX && record->Counter < INT_MAX) { // stopword mark
            interval->TotalLength += hitsSize(record->Offset, record->Length, record->Counter, subIndexInfo);
            interval->TotalCounter += record->Counter;
        } else {
            interval->TotalLength = 0;
            interval->TotalCounter = INT_MAX;
        }
    }
    interval->LastNumber = num;
    rc.SetCachedInterval(keyPrefix, lemma, maxInterval, *interval);
}

void IKeysAndPositions::Scan(IKeyPosScanner& scanner) const {
    TRequestContext rc;
    i32 pos = LowerBound(scanner.GetStartPos(), rc);

    while (true) {
        i32 block = UNKNOWN_BLOCK;
        const YxRecord* rec = EntryByNumber(rc, pos, block);
        if (!rec)
            break;

        TString key = rec->TextPointer;

        IKeyPosScanner::EContinueLogic contLogic = scanner.TestKey(rec->TextPointer);

        if (contLogic == IKeyPosScanner::clStop) {
            break;
        } else if (contLogic == IKeyPosScanner::clSkip) {
            ++pos;
        } else if (contLogic == IKeyPosScanner::clCheckDocids) {
            TPosIterator<> it;
            it.Init(*this, rec->Offset, rec->Length, rec->Counter, RH_DEFAULT);
            for (; it.Valid(); ++it) {
                scanner.ProcessPos(rec->TextPointer, it);
            }
            ++pos;
        } else {
            Y_FAIL("Icorrect enum value");
        }
    }
}
