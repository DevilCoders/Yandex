#pragma once

#include <cstdio>
#include <iostream>

class linereader;
class IInputStream;

class RobotTxt {
public:
         RobotTxt(const char *name = nullptr);
         ~RobotTxt();

    int  Disallow(const char*) const;
    int  IsDisallowAll() const { return NR == -1; }
    int  Empty() const { return NR == 0; }
    void SetAgent(const char *name);
    void Parse(FILE*);
    void Parse(std::istream*, const char *hostname = nullptr);
    void Parse(std::istream*, const char *hostname, int port);
    void Parse(IInputStream*, const char *hostname = nullptr);
    void Parse(IInputStream*, const char *hostname, int port);
    void Load(const char *saved_data);
    void DisallowAll() { Clear(nullptr, nullptr); NR = -1; PackRules(nullptr, nullptr); }
    size_t GetPackedRules(const char *&out) const;
    void Clear() { Clear(nullptr, nullptr); }
    /// Compare returns: -1 if this is subset of r, 0 if equal, 1 if this > r, -2/2 if uncomparable
    int  Compare(const RobotTxt &r) const;
    int  GetNR() const { return NR; }
    int  GetNRR() const { return NRR; }
    int  GetCrawlDelay() const { return CrawlDelay; }
    const char *GetHostDirective() { return HostDirective; }
    const char *GetRule(int N) {
        if (N >= NR)
            return nullptr;
        return Rules[N];
    }
    const char *GetRevRule(int N) {
        if (N >= NRR)
            return nullptr;
        return RevRules[N];
    }

protected:
    RobotTxt(const RobotTxt&); //not implemented
    void Add(const char*, int len, char *&buf_start, char const *const buf_end);
    void Sort();
    void Parse(linereader &reader, const char *hostname, int port);
    void Parse(linereader &reader, const char *hostname); // parses out port
    void PackRules(char const* const buf_beg, char const *const buf_end);
    void Clear(char const* const buf_beg, char const *const buf_end);

    char **Rules,  **RevRules;
    char *HostDirective;
    char *Agent;
    int  lAgent;
    int  NR,       NRR;
    int  MaxNR,    MaxNRR;
    int  CrawlDelay;

    char *PackedRules;
};
