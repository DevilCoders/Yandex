#include <stdexcept>

#include "setup.h"
#include "util.h"

namespace cookiemy {

static const CookiemyHandler::BlockData empty_data;

CookiemyHandler::CookiemyHandler()
{}

void
CookiemyHandler::insert(BlockID id, const BlockData &data) {
    if (!data.empty()) {
        setup_[id] = data;
    }
}

const CookiemyHandler::BlockData&
CookiemyHandler::find(BlockID id) const {
    std::map<BlockID, BlockData>::const_iterator it = setup_.find(id);
    if (setup_.end() == it) {
        return empty_data;
    }
    return it->second;
}

void
CookiemyHandler::erase(BlockID id) {
    setup_.erase(id);
}

void
CookiemyHandler::parse(const std::string &cookie) {
    setup_.clear();
    if (cookie.empty()) {
        return;
    }
    std::vector<char> bytes(Utils::unbase64Length(cookie.c_str(), cookie.size()));

    if (Utils::cookieBase64Decode(cookie.c_str(), cookie.size(), &bytes[0], bytes.size()) < 0) {
        throw std::runtime_error("Cannot parse cookie");
    }

    if (bytes[0] != 'c') {
        throw std::runtime_error("Wrong cookie version");
    }

    std::vector<char>::iterator it = bytes.begin();
    std::vector<char>::iterator it_end = it + bytes.size();

    if (it_end == it) {
        return;
    }
    ++it;
    if (it_end == it) {
        return;
    }

    while (it_end != it) {
        BlockID block_id = *it;
        ++it;
        if (0 == block_id || it_end == it) {
            return;
        }
        unsigned char count = *it;
        ++it;
        if (count > std::distance(it, it_end)) {
            return;
        }
        BlockData ids(count);
        i64 shift = Utils::parseCookieBlock(&(*it), std::distance(it, it_end), &(ids[0]), count);
        if (0 > shift) {
            throw std::runtime_error("Cannot parse cookie block");
        }
        std::advance(it, shift);
        insert(block_id, ids);
    }
}


std::string
CookiemyHandler::toString() const {
    std::string result;
    result.push_back('c');
    for(std::map<BlockID, BlockData>::const_iterator it = setup_.begin();
        it != setup_.end();
        ++it) {
        result.push_back(it->first);
        int size = it->second.size();
        result.push_back(size);
        for (int i = 0; i < size; ++i) {
            unsigned int value = it->second[i];
            if (value < 0x80) {
                result.push_back(value);
            }
            else if (value < 0x4000) {
                value |= 0x8000;
                unsigned int c = value & 0xFF;
                value >>= 8;
                result.push_back(value);
                result.push_back(c);
            }
            else if (value < 0x10000000) {
                value |= 0xE0000000;
                unsigned char c1, c2, c3, c4;
                c1 = value % 0x100;
                value /= 0x100;
                c2 = value % 0x100;
                value /= 0x100;
                c3 = value % 0x100;
                c4 = value /= 0x100;
                result.push_back(c4);
                result.push_back(c3);
                result.push_back(c2);
                result.push_back(c1);
            }
            else {
                throw std::runtime_error("Cannot serialize cookie my");
            }
        }
    }
    result.push_back(0);

    std::vector<char> buf(Utils::base64Length(result.size()) + 1);
    Utils::cookieBase64Encode((const unsigned char*)result.c_str(), result.size(), &buf[0]);
    return std::string(&buf[0], buf.size() - 1);
}

void
CookiemyHandler::clear() {
    setup_.clear();
}

std::vector<CookiemyHandler::BlockID>
CookiemyHandler::keys() const {
    std::vector<CookiemyHandler::BlockID> keys;
    keys.reserve(setup_.size());
    for(std::map<BlockID, BlockData>::const_iterator it = setup_.begin(), end = setup_.end();
        it != end; ++it) {
        keys.push_back(it->first);
    }
    return keys;
}

}
