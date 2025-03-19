#pragma once

#include <util/system/defaults.h>

#include <kernel/search_types/search_types.h>  // MAXKEY_BUF

struct YxRecord {
    char       TextPointer[MAXKEY_BUF]; // текущий вход в словарь
    ui32       Length = 0;              // длина ссылок (байтовая) от данного входа
    // в промежуточном формате не хранятся !!!
    i64        Offset = 0;              // смещение в инвертированном файле
    i64        Counter = 0;             // счетчик ссылок от данного входа

    YxRecord() {
        TextPointer[0] = 0;
    }
    i64 GetWeight() const {
        return Counter;
    }
    bool operator ==(const YxRecord &e) const {
        return 0 == strncmp(TextPointer, e.TextPointer, MAXKEY_BUF);
    }
    bool operator < (const YxRecord &e) const {
        return (strncmp(TextPointer, e.TextPointer, MAXKEY_BUF) < 0);
    }
};
