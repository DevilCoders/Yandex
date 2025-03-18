#pragma once

#include <util/system/types.h>
#include <string>
#include <vector>
#include <map>

namespace cookiemy {

class Block {
public:
    Block();
private:
    const char* buf_;
    ui64 length_;
};

class CookiemyHandler {
public:
    typedef std::vector<unsigned int> BlockData;
    typedef unsigned char BlockID;

    CookiemyHandler();

    void parse(const std::string &cookie);

    void insert(BlockID id, const BlockData &data);
    const BlockData& find(BlockID id) const;
    std::vector<BlockID> keys() const;
    void erase(BlockID id);

    std::string toString() const;

    void clear();

private:
    std::map<BlockID, BlockData> setup_;
};

}
