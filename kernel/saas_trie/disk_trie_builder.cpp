#include "disk_trie_builder.h"

#include "config.h"
#include "disk_io.h"

#include <library/cpp/containers/comptrie/comptrie_builder.h>
#include <library/cpp/logger/global/global.h>

namespace NSaasTrie {
    using TTrie = TCompactTrieBuilder<char, ui64>;

    namespace {
        void WriteTrie(const TString& path, const IDiskIO& disk, const TTrie& trie) {
            auto output = disk.CreateWriter(path);
            output->Write(TrieFileHeader, sizeof(TrieFileHeader));
            output->Write(reinterpret_cast<const char*>(&TrieVersion), sizeof(TrieVersion));
            ui64 trieSize = trie.MeasureByteSize();
            output->Write(reinterpret_cast<const char*>(&trieSize), sizeof(trieSize));
            trie.Save(*output);
        }
    }

    void BuildTrieFromIterator(const TString& path, const IDiskIO& disk, ITrieStorageIterator& inputIterator) {
        TTrie trie;
        if (!inputIterator.AtEnd()) {
            do {
                trie.Add(inputIterator.GetKey(), inputIterator.GetValue());
            } while (inputIterator.Next());
        }
        WriteTrie(path, disk, trie);
    }

    struct TTrieBuilder : ITrieStorageWriter {
        TTrieBuilder(TString path, const IDiskIO& disk)
            : Path(std::move(path))
            , Disk(disk)
        {
        }

        ~TTrieBuilder() {
            try {
                if (!Discarded) {
                    WriteTrie(Path, Disk, Trie);
                }
            } catch (...) {
                ERROR_LOG << "Can not write trie " << Path << Endl;
            }
        }
        bool IsReadOnly() const override {
            return false;
        }
        bool Put(TStringBuf key, ui64 value) override {
            Trie.Add(key, value);
            return true;
        }
        bool Delete(TStringBuf /*key*/) override {
            FAIL_LOG("Not implemented");
        }
        bool DeleteIfEqual(TStringBuf /*key*/, ui64 /*value*/) override {
            FAIL_LOG("Not implemented");
        }
        void Discard() override {
            Discarded = true;
        }

    private:
        TString Path;
        const IDiskIO& Disk;
        TTrie Trie;
        bool Discarded = false;
    };

    THolder<ITrieStorageWriter> CreateTrieBuilder(TString path, const IDiskIO& disk) {
        return MakeHolder<TTrieBuilder>(std::move(path), disk);
    }
}
