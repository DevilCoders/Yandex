#include "fake_disk_trie.h"

#include <kernel/saas_trie/disk_trie.h>
#include <kernel/saas_trie/disk_trie_builder.h>

#include <util/stream/str.h>

#include <functional>

namespace NSaasTrie {
    namespace NTesting {
        struct TTestOutputStream : TStringOutput {
            using TFinishCallback = std::function<void (const TString& s)>;

            TTestOutputStream(TFinishCallback callback, THolder<TString> buffer = MakeHolder<TString>())
                : TStringOutput(*buffer)
                , Buffer(std::move(buffer))
                , Callback(std::move(callback))
            {
            }

            ~TTestOutputStream() {
                Callback(*Buffer);
            }

        private:
            THolder<TString> Buffer;
            TFinishCallback Callback;
        };

        TBlob TFakeDisk::Map(const TString& path) const {
            auto& content = FakeFiles.at(path);
            return TBlob::NoCopy(content.data(), content.size());
        }

        THolder<IOutputStream> TFakeDisk::CreateWriter(const TString& path) const {
            return MakeHolder<TTestOutputStream>([this, path](const TString& content) {
                FakeFiles[path] = content;
            });
        }

        TTestTrieIterator::TTestTrieIterator(const TVector<std::pair<TString, ui64>>& data)
            : Iterator(data.begin())
            , End(data.end())
        {
        }

        bool TTestTrieIterator::AtEnd() const {
            return Iterator == End;
        }
        TString TTestTrieIterator::GetKey() const {
            return Iterator->first;
        }
        ui64 TTestTrieIterator::GetValue() const {
            return Iterator->second;
        }
        bool TTestTrieIterator::Next() {
            return ++Iterator != End;
        }

        TFakeDiskTrieContext CreateFakeDiskTrie(const TVector<std::pair<TString, ui64>>& testData) {
            const TString triePath = "test.trie";
            auto disk = MakeHolder<TFakeDisk>();
            TTestTrieIterator iterator{testData};
            BuildTrieFromIterator(triePath, *disk, iterator);
            auto trie = OpenDiskTrie(triePath, *disk);
            return {
                std::move(trie),
                std::move(disk)
            };
        }
    }
}
