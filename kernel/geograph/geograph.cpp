#include "geograph.h"

#include <kernel/gazetteer/articlepool.h>

#include <util/stream/mem.h>

static TString GEOPART = TString("geopart");

namespace NGeoGraph {
    TGeoGraph::TGeoGraph()
        : MemoryPool(1024 * 1024, TMemoryPool::TLinearGrow::Instance())
        , Index(&MemoryPool)
    {
    }

    void TGeoGraph::Init(const NGzt::TGazetteer* gazetteer) {
        const NGzt::TArticlePool& pool = gazetteer->ArticlePool();
        const NProtoBuf::Descriptor* messageType = pool.ProtoPool().FindMessageTypeByName("TGeoArticle");
        if (!messageType)
            return;

        TVector<ui32> articles;
        TVector<const NProtoBuf::FieldDescriptor*> fields;
        for (NGzt::TArticlePool::TIterator it(pool); it.Ok(); ++it) {
            const NGzt::TArticlePtr article = it.GetArticle();
            if (!article.IsInstance(messageType) || article.GetTitle().Contains('/'))
                continue;
            Nodes.push_back({MemoryPool.AppendString(TWtringBuf(article.GetTitle())), 0, 0});
            fields.push_back(article.FindField(GEOPART));
            articles.push_back(*it);
        }

        // reserve some space for nodes that do have slashes in titles after all
        const size_t hashSize = static_cast<size_t>(Nodes.size() * 1.1);
        Index.reserve(hashSize);
        THashMap<ui32, size_t> byArticle(hashSize);
        for (size_t i = 0; i < Nodes.size(); i++) {
            Index[Nodes[i].Title] = i;
            byArticle[articles[i]] = i;
        }

        for (size_t i = 0; i < Nodes.size(); i++) {
            Nodes[i].ParentBegin = Parents.size();
            auto body = pool.LoadArticleAtOffset(articles[i]);
            for (auto it = NGzt::TRefIterator(pool, *body, fields[i]); it.Ok(); ++it) {
                const size_t parentId = byArticle.emplace(*it, Nodes.size()).first->second;
                if (parentId == Nodes.size()) {
                    const NGzt::TArticlePtr parent = it.LoadArticle();
                    const TWtringBuf title = MemoryPool.AppendString(TWtringBuf(parent.GetTitle()));
                    Index[title] = parentId;
                    Nodes.push_back({title, 0, 0});
                    fields.push_back(parent.FindField(GEOPART));
                    articles.push_back(*it);
                }
                Parents.push_back(parentId);
            }
            Nodes[i].ParentEnd = Parents.size();
        }
        ParentsPtr = Parents.data();
    }

    void TGeoGraph::Init(TBlob source) {
        Source = source;
        TMemoryInput in(source.Data(), source.Size());
        size_t size = ::LoadSize(&in);
        Parents.clear();
        Index.clear();
        Index.reserve(size);
        Nodes.resize(size);
        for (size_t i = 0; i < size; i++) {
            size_t keySize = ::LoadSize(&in);
            Nodes[i].Title = TWtringBuf(reinterpret_cast<const wchar16*>(in.Buf()), keySize);
            in.Skip(keySize * sizeof(wchar16));
            Nodes[i].ParentBegin = ::LoadSize(&in);
            Nodes[i].ParentEnd = ::LoadSize(&in);
            Index[Nodes[i].Title] = i;
        }
        ParentsPtr = reinterpret_cast<const size_t*>(in.Buf());
    }

    void TGeoGraph::Save(IOutputStream& out) const {
        size_t parents = 0;
        ::SaveSize(&out, Nodes.size());
        for (const auto& node : Nodes) {
            ::SaveSize(&out, node.Title.size());
            ::SavePodArray(&out, node.Title.data(), node.Title.size());
            ::SaveSize(&out, node.ParentBegin);
            ::SaveSize(&out, node.ParentEnd);
            parents = Max(parents, node.ParentEnd);
        }
        // TWtringBuf serialization is already endianness-dependent, so might as well go all the way
        ::SavePodArray(&out, ParentsPtr, parents);
    }
}
