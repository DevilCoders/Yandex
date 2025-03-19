#pragma once

#include <cstring>

enum DeleteZeroSentencesEnum {DZ_NONE, DZ_OLD, DZ_ALL};

class TAttrDeleteLogic {
private:
    const char** DelPfxs;
    unsigned DelLen;
    int IsNotExternalAttr;
    bool IsAttr;
    bool Invert;
    DeleteZeroSentencesEnum DeleteZero;

public:
    //! @param delPfxs    prefixes must be sorted and the last pointer must be equal to NULL
    TAttrDeleteLogic(const char** delPfxs, DeleteZeroSentencesEnum deleteZero, bool invert = false) {
        Reset(delPfxs, deleteZero, invert);
    }

    //! @param delPfxs    prefixes must be sorted and the last pointer must be equal to NULL
    void Reset(const char** delPfxs, DeleteZeroSentencesEnum deleteZero, bool invert = false) {
        Invert = invert;
        DelPfxs = delPfxs;
        DelLen = DelPfxs && *DelPfxs ? (unsigned int)strlen(*DelPfxs) : 0;
        IsAttr = false;
        DeleteZero = deleteZero;
    }

    inline bool OnKey(const char* key) {
        IsAttr = key && ('#' == key[0]);

        bool result = false;
        IsNotExternalAttr = -1;
        while (DelLen) {
            IsNotExternalAttr = strncmp(*DelPfxs, key, DelLen);
            if (IsNotExternalAttr >= 0) {
                if (0 == IsNotExternalAttr)
                    result = true;
                break;
            }
            ++DelPfxs;
            Y_ASSERT(DelPfxs[0] == nullptr || strcmp(DelPfxs[-1], DelPfxs[0]) < 0);
            DelLen = *DelPfxs ? (unsigned int)strlen(*DelPfxs) : 0;
        }
        if (Invert) {
            IsNotExternalAttr = !IsNotExternalAttr;
        }
        return Invert ? !result : result;
    }

    inline bool OnPos(SUPERLONG pos, bool isOld) {
        if (!IsNotExternalAttr && isOld)
            return true;

        bool process = false;
        if (DeleteZero == DZ_ALL)
            process = true;
        else if ((DeleteZero == DZ_OLD) && isOld)
            process = true;
        if (process)
            return !IsAttr && (0 == TWordPosition::Break(pos));
        else
            return false;
    }
};
