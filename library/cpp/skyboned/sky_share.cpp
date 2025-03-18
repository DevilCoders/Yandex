#include "sky_share.h"
#include "api.h"

#include <contrib/libs/bencode/include/bencode.hpp>

#include <library/cpp/json/json_value.h>
#include <util/charset/utf8.h>
#include <util/folder/path.h>
#include <util/generic/algorithm.h>
#include <util/string/hex.h>

#include <algorithm>
#include <filesystem>
#include <sstream>

namespace NSkyboneD {

namespace {
    TString CalcSha1(TStringBuf data) {
        auto digest = NOpenSsl::NSha1::Calc(data);
        return TString{reinterpret_cast<const char*>(digest.data()), digest.size()};
    }

    TString CalcSha1Hex(TStringBuf data) {
        return ToLowerUTF8(HexEncode(CalcSha1(data)));
    }
}

TSkyShare::TSkyShare(const TString& tvmTicket, const NSkyboneD::NApi::TApiConfig& config)
    : TvmHeader_("X-Ya-Service-Ticket: " + tvmTicket)
    , ApiConfig_(config)
{}

void TSkyShare::SetSourceId(TString sourceId) {
    SourceId_ = std::move(sourceId);
}

TShareObjectPtr TSkyShare::AddFile(TString path, TString httpLink, bool executable) {
    if (ApiRequest_.IsDefined()) {
        ythrow yexception() << "can't reuse TSkyShare object after api request generation";
    }
    auto objectPtr = MakeAtomicShared<TShareObject>(std::move(path), std::move(httpLink), executable);
    Files_.push_back(objectPtr);
    return objectPtr;
}

void TSkyShare::AddEmptyDirectory(TString path) {
    if (ApiRequest_.IsDefined()) {
        ythrow yexception() << "can't reuse TSkyShare object after api request generation";
    }
    EmptyDirs_.push_back(std::move(path));
}

void TSkyShare::AddSymlink(TString target, TString linkName) {
    if (ApiRequest_.IsDefined()) {
        ythrow yexception() << "can't reuse TSkyShare object after api request generation";
    }
    Links_.push_back(std::make_pair(std::move(target), std::move(linkName)));
}

void TSkyShare::GenerateApiRequest() {
    Y_ENSURE(!ApiRequest_.IsDefined());

    bencode::dict structure;
    bencode::dict torrents;

    auto addAllDirectories = [&structure](std::string_view path) {
        std::filesystem::path currentDir{"", std::filesystem::path::format::generic_format};
        for (const auto dir : std::filesystem::path{path}) {
            if (dir == ".") {
                continue;
            }
            currentDir /= dir;

            if (structure.contains(currentDir)) {
                const auto& dict = boost::get<bencode::dict>(structure[currentDir]);
                const auto& type = boost::get<bencode::string>(dict.at("type"));
                Y_ENSURE(type == "dir", TString{"directory conflict for path "} + currentDir);
            } else {
                structure[currentDir] = bencode::dict {
                        {"type",       "dir"},
                        {"path",       currentDir},
                        {"size",       -1},
                        {"executable", 1},
                        {"mode",       "rwxr-xr-x"},
                        {"resource",   bencode::dict {
                                {"type", "mkdir"},
                                {"data", "mkdir"},
                                {"id",   TString{"mkdir:"} + currentDir}
                        }}
                };
            }
        }
    };

    for (const TShareObjectPtr& file : Files_) {
        Y_ENSURE(file->GetPath(), "empty path is not allowed");
        Y_ENSURE(file->GetMagicSha1String() && file->GetMd5(), "not finished file is not allowed");
        Y_ENSURE(!structure.contains(file->GetPath().c_str()), "duplicate entry " + file->GetPath());
        addAllDirectories(TFsPath{file->GetPath()}.Dirname());

        bencode::dict resource;
        if (file->GetSize() > 0) {
            bencode::dict torrent {
                    {"name",         "data"},
                    {"piece length", RBTORRENT_BLOCK_SIZE},
                    {"pieces",       file->GetMagicSha1String()},
                    {"length",       static_cast<std::int64_t>(file->GetSize())}
            };
            const TString infoHash = CalcSha1Hex(bencode::encode(torrent));
            torrents[infoHash] = bencode::dict {
                    {"info", std::move(torrent)}
            };
            resource = bencode::dict {
                    {"type", "torrent"},
                    {"id",   infoHash}
            };
        } else {
            resource = bencode::dict {
                    {"type", "touch"}
            };
        }

        structure[file->GetPath()] = bencode::dict {
                {"type",       "file"},
                {"path",       file->GetPath()},
                {"md5sum",     HexDecode(file->GetMd5())},
                {"size",       static_cast<std::int64_t>(file->GetSize())},
                {"executable", file->GetExecutable() ? 1 : 0},
                {"mode",       file->GetExecutable() ? "rwxr-xr-x" : "rw-r--r--"},
                {"resource",   std::move(resource)}
        };
    }

    for (const auto& [target, linkName] : Links_) {
        Y_ENSURE(!structure.contains(linkName.c_str()));
        addAllDirectories(TFsPath{linkName}.Dirname());
        structure[linkName] = bencode::dict {
                {"type",       "file"},
                {"path",       linkName},
                {"size",       0},
                {"mode",       ""},
                {"executable", 1},
                {"resource",   bencode::dict {
                        {"type", "symlink"},
                        {"data", "symlink:" + target}
                }}
        };
    }

    for (const TString& emptyDir : EmptyDirs_) {
        addAllDirectories(emptyDir);
    }

    bencode::dict head {
            {"structure", std::move(structure)},
            {"torrents",  std::move(torrents)}
    };

    std::string headBinary = bencode::encode(head);

    std::string headPieces;
    for (size_t offset = 0; offset < headBinary.size(); offset += RBTORRENT_BLOCK_SIZE) {
        headPieces += CalcSha1(TStringBuf(headBinary.data() + offset,
                                          Min(size_t(RBTORRENT_BLOCK_SIZE), headBinary.size() - offset)));
    }

    bencode::dict headTi {
            {"name",         "metadata"},
            {"piece length", RBTORRENT_BLOCK_SIZE},
            {"pieces",       headPieces},
            {"length",       static_cast<std::int64_t>(headBinary.size())}
    };

    TString uid = CalcSha1Hex(bencode::encode(headTi));
    TString headBinaryHex = ToLowerUTF8(HexEncode(headBinary));

    ApiRequest_["uid"] = uid;
    ApiRequest_["head"] = headBinaryHex;
    ApiRequest_["info"].SetType(NJson::JSON_MAP);
    for (const auto& file : Files_) {
        if (file->GetSize() > 0) {
            Y_ENSURE(file->GetPublicHttpLink(), "file without public http link is not allowed");
            ApiRequest_["info"][file->GetMd5()].AppendValue(file->GetPublicHttpLink());
        }
    }
    ApiRequest_["mode"] = "plain";
    ApiRequest_["no_wait"] = false;
    if (SourceId_) {
        ApiRequest_["source_id"] = SourceId_;
    }
}

TString TSkyShare::GetTemporaryRBTorrentWithoutSkybonedRegistration() {
    if (!ApiRequest_.IsDefined()) {
        GenerateApiRequest();
    }
    Y_ENSURE(ApiRequest_.Has("uid"));
    Y_ENSURE(ApiRequest_["uid"].IsString());
    Y_ENSURE(ApiRequest_["uid"].GetString().Size() == 40);
    return "rbtorrent:" + ApiRequest_["uid"].GetString();
}

TString TSkyShare::FinalizeAndGetRBTorrent() {
    if (!ApiRequest_.IsDefined()) {
        GenerateApiRequest();
    }
    return NSkyboneD::NApi::AddResource(ApiConfig_,
                                        TvmHeader_,
                                        ApiRequest_);
}

}
