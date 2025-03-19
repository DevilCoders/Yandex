#pragma once

const unsigned int WHOIS_BUF_SIZE = 4096;

struct THostAttrsInfo {
    int IP;
    const char* Whois;

    THostAttrsInfo(int a = 0, const char* w = nullptr) {
        IP = a;
        Whois =  w;
    }

    size_t SizeOf() const {
        return sizeof(ui32) + (Whois == nullptr ? 0 : strlen(Whois));
    }
};
