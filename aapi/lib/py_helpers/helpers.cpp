#include "helpers.h"

#include <util/stream/str.h>
#include <util/stream/file.h>
#include <util/string/vector.h>
#include <library/cpp/blockcodecs/stream.h>
#include <library/cpp/blockcodecs/codecs.h>
#include <aapi/lib/node/svn_commit.fbs.h>
#include <aapi/lib/node/tree.fbs.h>
#include <aapi/lib/node/hg_heads.fbs.h>
#include <aapi/lib/node/hg_changeset.fbs.h>


const size_t DEFAULT_BUFFER_LENGTH = 10 * 1024 * 1024;
const TString DEFAULT_CODEC = "zstd06_1";


ui64 Compress(IInputStream* input, IOutputStream* output) {
    const NBlockCodecs::ICodec* codec = NBlockCodecs::Codec(DEFAULT_CODEC);
    NBlockCodecs::TCodedOutput compressedStream(output, codec, DEFAULT_BUFFER_LENGTH);
    return TransferData(input, &compressedStream);
}


ui64 Decompress(IInputStream* input, IOutputStream* output) {
    NBlockCodecs::TDecodedInput decompressedStream(input);
    return TransferData(&decompressedStream, output);
}


ui64 CompressFile(const TString& src, const TString& dst) {
    TUnbufferedFileInput in(src);
    TUnbufferedFileOutput out(dst);
    return Compress(&in, &out);
}


ui64 DecompressFile(const TString& src, const TString& dst) {
    TUnbufferedFileInput in(src);
    TUnbufferedFileOutput out(dst);
    return Decompress(&in, &out);
}


TString Compress(const TString& s) {
    TStringStream out;
    TStringStream in(s);
    Compress(&in, &out);
    return out.Str();
}


TString Decompress(const TString& s) {
    TStringStream out;
    TStringStream in(s);
    Decompress(&in, &out);
    return out.Str();
}


TString DumpSvnCommit(const TSvnCommit& commit) {
    flatbuffers::FlatBufferBuilder builder(1024);
    auto author = builder.CreateString(commit.author);
    auto date = builder.CreateString(commit.date);
    auto msg = builder.CreateString(commit.msg);
    auto tree = builder.CreateVector(reinterpret_cast<const ui8*>(commit.tree.c_str()), commit.tree.size());
    auto parent = builder.CreateVector(reinterpret_cast<const ui8*>(commit.parent.c_str()), commit.parent.size());
    auto ci = NAapi::NNode::CreateSvnCommit(builder, commit.revision, author, date, msg, tree, parent);
    builder.Finish(ci);
    return TString(reinterpret_cast<char*>(builder.GetBufferPointer()), builder.GetSize());
}


TSvnCommit LoadSvnCommit(const TString& buf) {
    auto ci = NAapi::NNode::GetSvnCommit(reinterpret_cast<const ui8*>(buf.c_str()));
    TSvnCommit commit;
    commit.revision = ci->revision();
    commit.author = TString(ci->author()->c_str());
    commit.date = TString(ci->date()->c_str());
    commit.msg = TString(ci->msg()->c_str());

    commit.tree = TString();
    for (size_t i = 0; i < ci->tree()->size(); ++i) {
        commit.tree.push_back(ci->tree()->Get(i));
    }

    commit.parent = TString();
    for (size_t i = 0; i < ci->parent()->size(); ++i) {
        commit.parent.push_back(ci->parent()->Get(i));
    }

    return commit;
}


TString DumpTree(const TTree& tree) {
    flatbuffers::FlatBufferBuilder builder(1024);
    TVector<flatbuffers::Offset<NAapi::NNode::TreeEntry>> entries;
    for (const TTreeEntry& entry: tree.entries) {
        auto name = builder.CreateString(entry.name);
        auto hash = builder.CreateVector(reinterpret_cast<const ui8*>(entry.hash.c_str()), entry.hash.size());
        TString blobsJoin = JoinStrings(entry.blobs.cbegin(), entry.blobs.cend(), "");
        auto blobs = builder.CreateVector(reinterpret_cast<const ui8*>(blobsJoin.c_str()), blobsJoin.size());
        entries.push_back(NAapi::NNode::CreateTreeEntry(builder, name, entry.mode, entry.size, hash, blobs));
    }
    auto t = NAapi::NNode::CreateTree(builder, builder.CreateVector(entries));
    builder.Finish(t);
    return TString(reinterpret_cast<char*>(builder.GetBufferPointer()), builder.GetSize());
}


TTree LoadTree(const TString& buf) {
    auto t = NAapi::NNode::GetTree(reinterpret_cast<const ui8*>(buf.c_str()));
    TTree tree;
    tree.entries = TVector<TTreeEntry>();

    for (size_t i1 = 0; i1 < t->entries()->size(); ++i1) {
        auto e = t->entries()->Get(i1);
        TTreeEntry entry;
        entry.name = TString(e->name()->c_str());
        entry.mode = e->mode();
        entry.size = e->size();
        TString hash;
        for (size_t i2 = 0; i2 < e->hash()->size(); ++i2) {
            hash.push_back(e->hash()->Get(i2));
        }
        entry.hash = hash;
        TVector<TString> blobs;
        size_t hashSize = hash.size();
        size_t blobsCount = e->blobs()->size() / hashSize;
        for (size_t i3 = 0; i3 < blobsCount; ++i3) {
            TString blob;
            for (size_t j = 0; j < hashSize; ++j) {
                blob.push_back(e->blobs()->Get(i3 * hashSize + j));
            }
            blobs.push_back(blob);
        }
        entry.blobs = blobs;
        tree.entries.push_back(entry);
    }

    return tree;
}


TString DumpHgHeads(const THgHeads& heads) {
    flatbuffers::FlatBufferBuilder builder(1024);
    TVector<flatbuffers::Offset<NAapi::NNode::HgHead>> fbs_heads;
    for (const THgHead& head: heads.heads) {
        auto branch = builder.CreateString(head.branch);
        auto hash = builder.CreateVector(reinterpret_cast<const ui8*>(head.hash.c_str()), head.hash.size());
        fbs_heads.push_back(NAapi::NNode::CreateHgHead(builder, branch, hash));
    }
    auto h = NAapi::NNode::CreateHgHeads(builder, builder.CreateVector(fbs_heads));
    builder.Finish(h);
    return TString(reinterpret_cast<char*>(builder.GetBufferPointer()), builder.GetSize());
}


THgHeads LoadHgHeads(const TString& buf) {
    auto hs = NAapi::NNode::GetHgHeads(reinterpret_cast<const ui8*>(buf.c_str()));
    THgHeads heads;
    heads.heads = TVector<THgHead>();

    for (size_t i = 0; i < hs->heads()->size(); ++i) {
        auto h = hs->heads()->Get(i);
        THgHead head;
        head.branch = TString(h->branch()->c_str());
        head.hash = TString(reinterpret_cast<const char*>(h->hash()->data()), h->hash()->size());
        heads.heads.push_back(head);
    }

    return heads;
}


TString DumpHgChangeset(const THgChangeset& changeset) {
    flatbuffers::FlatBufferBuilder builder(1024);
    auto hash = builder.CreateVector(reinterpret_cast<const ui8*>(changeset.hash.c_str()), changeset.hash.size());
    auto author = builder.CreateString(changeset.author);
    auto date = builder.CreateString(changeset.date);
    auto msg = builder.CreateString(changeset.msg);
    auto branch = builder.CreateString(changeset.branch);
    auto extra = builder.CreateString(changeset.extra);
    auto tree = builder.CreateVector(reinterpret_cast<const ui8*>(changeset.tree.c_str()), changeset.tree.size());
    TVector<flatbuffers::Offset<flatbuffers::String>> files;
    for (const TString& file: changeset.files) {
        files.push_back(builder.CreateString(file));
    }
    const TString parentsJoin = JoinStrings(changeset.parents.cbegin(), changeset.parents.cend(), "");
    auto parents = builder.CreateVector(reinterpret_cast<const ui8*>(parentsJoin.c_str()), parentsJoin.size());

    auto cs = NAapi::NNode::CreateHgChangeset(
        builder,
        hash,
        author,
        date,
        msg,
        builder.CreateVector(files),
        branch,
        changeset.close_branch,
        extra,
        tree,
        parents
     );

    builder.Finish(cs);
    return TString(reinterpret_cast<char*>(builder.GetBufferPointer()), builder.GetSize());
}


THgChangeset LoadHgChangeset(const TString& buf) {
    auto cs = NAapi::NNode::GetHgChangeset(reinterpret_cast<const ui8*>(buf.c_str()));
    THgChangeset changeset;

    changeset.hash = TString(reinterpret_cast<const char*>(cs->hash()->data()), cs->hash()->size());
    changeset.author = TString(cs->author()->c_str());
    changeset.date = TString(cs->date()->c_str());
    changeset.msg = TString(cs->msg()->c_str());

    changeset.files = TVector<TString>();
    for (size_t i1 = 0; i1 < cs->files()->size(); ++i1) {
        changeset.files.emplace_back(cs->files()->Get(i1)->c_str());
    }

    changeset.branch = TString(cs->branch()->c_str());
    changeset.close_branch = cs->close_branch();
    changeset.extra = TString(cs->extra()->c_str());
    changeset.tree = TString(reinterpret_cast<const char*>(cs->tree()->data()), cs->tree()->size());

    changeset.parents = TVector<TString>();
    for (size_t i2 = 0; i2 < cs->parents()->size(); i2 += 20) {
        changeset.parents.emplace_back(reinterpret_cast<const char*>(cs->parents()->data() + i2), 20);
    }

    return changeset;
}
