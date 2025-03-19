#include "fio.h"

#include <util/string/cast.h>

const char FIOTypeLength[FIOTypeCount][2] = {
    "3",
    "2",
    "1",
    "2",
    "1",
    "2",
    "3",
    "3",
    "9"
};

const char* FioType2Length(EFIOType fioType) {
    if (fioType >= FIOTypeCount)
        ythrow yexception() << "Out of range error in function FioType2Length().";
    return FIOTypeLength[fioType];
}
