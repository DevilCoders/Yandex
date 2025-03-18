#pragma once

#include "elements.h"
#include <library/cpp/json/writer/json_value.h>

// TODO
// - cache-optimal adjacency list walk vs memory usage
// - all elements in traits
// - oriented edges support

// INTERFACE

namespace NGraph {
    template <class Traits>
    class TAbstractGraph {
    public:
        typedef TAbstractGraph TSelf;
        typedef Traits TTraits;

        // this types are expected to be unsigned integers
        typedef typename TTraits::TCount TVertexId;
        typedef typename TTraits::TCount TEdgeId;
        typedef typename TTraits::TCount TCount;

        typedef typename TTraits::TVertexProps TVertexProps;
        typedef typename TTraits::TEdgeProps TEdgeProps;
        typedef typename TTraits::TStringT TStringType;
        typedef typename TTraits::TInputStreamT IInputStream;
        typedef typename TTraits::TOutputStreamT IOutputStream;

        template <EDirection Direction>
        class TAdjacentIterator: public NPrivate::TAdjacentIterator<TSelf, Direction> {
        public:
            typedef NPrivate::TAdjacentIterator<TSelf, Direction> TBase;
            typedef typename TBase::TParent TParent;

        public:
            TAdjacentIterator(TParent* parent, TVertexId vertexId, TCount offset)
                : TBase(parent, vertexId, offset)
            {
            }
        };
        template <EDirection Direction>
        class TConstAdjacentIterator: public NPrivate::TConstAdjacentIterator<TSelf, Direction> {
        public:
            typedef NPrivate::TConstAdjacentIterator<TSelf, Direction> TBase;
            typedef typename TBase::TParent TParent;

        public:
            TConstAdjacentIterator(TParent const* parent, TVertexId vertexId, TCount offset)
                : TBase(parent, vertexId, offset)
            {
            }
        };

#define ALL_FRIENDS(T)                        \
    friend class NPrivate::T<TSelf, ANY>;     \
    friend class NPrivate::T<TSelf, FORWARD>; \
    friend class NPrivate::T<TSelf, BACKWARD>;
        ALL_FRIENDS(TAdjacentIterator);
        ALL_FRIENDS(TConstAdjacentIterator);

        friend class NPrivate::TEdgeIterator<TSelf>;
        friend class NPrivate::TConstEdgeIterator<TSelf>;
        friend class NPrivate::TAdjacent<TSelf>;
        friend class NPrivate::TEdge<TSelf>;
        friend class NPrivate::TVertex<TSelf>;

        typedef NPrivate::TEdgeIterator<TSelf> TEdgeIterator;
        typedef NPrivate::TConstEdgeIterator<TSelf> TConstEdgeIterator;
        typedef NPrivate::TEdge<TSelf> TEdge;
        typedef NPrivate::TVertex<TSelf> TVertex;
        typedef NPrivate::TAdjacent<TSelf> TAdjacent;
        typedef NPrivate::TDefaultGluer<TSelf> TDefaultGluer;

    public:
        static const TCount COUNT_BIT_LENGTH() {
            return 8 * sizeof(TCount);
        }
        // reserve hightest bit in TCount for possible memory economy
        static const TCount MAX_VERTICES() {
            return (1 << (COUNT_BIT_LENGTH() - 1)) - 1;
        }
        static const TVertexId NULL_VERTEX() {
            return (1 << (COUNT_BIT_LENGTH() - 1));
        }
        static const TEdgeId NULL_EDGE() {
            return NULL_VERTEX();
        }

    public:
        void ReserveMemory(const double kfEReserve, const double kfVReserve, const ui32 minCount) {
            Vertices_.reserve(Max<ui32>(minCount, Vertices_.size() * Max<double>(1, kfVReserve)));
            Edges_.reserve(Max<ui32>(minCount, Edges_.size() * Max<double>(1, kfEReserve)));
        }

        void ReserveMemory(const ui32 verticesNum, const ui32 edgesNum) {
            Vertices_.reserve(verticesNum);
            Edges_.reserve(edgesNum);
        }

        TAbstractGraph(TCount numVertices);
        TAbstractGraph(TCount numVertices, TVertexProps const& c);
        template <class OtherTraits>
        TAbstractGraph(TAbstractGraph<OtherTraits> other);

        // O(1) amortized for all ADD operations
        TVertexId AddVertex(TVertexProps const& c);
        TVertexId AddVertex() {
            return AddVertex(TVertexProps());
        }
        void AddVertices(TCount numVertices) {
            for (TCount i = 0; i < numVertices; i++)
                AddVertex();
        }

        // no multiedges and loops, please!
        TEdgeIterator AddEdge(TVertexId u, TVertexId v, TEdgeProps const& c);
        TEdgeIterator AddEdge(TVertexId u, TVertexId v) {
            return AddEdge(u, v, TEdgeProps());
        }

        // O(1) worst case complexity for all GET methods
        TCount NumVertices() const;
        TCount NumEdges() const;

        // O(1) worst case
        TCount Degree(TVertexId u) const;

        // O(1) worst case complexity
        // all edge ids can be broken by remove operations and fixed by compress
        TEdge& Edge(TEdgeId id);
        TEdge const& Edge(TEdgeId id) const;

        TVertex& Vertex(TVertexId id);
        TVertex const& Vertex(TVertexId id) const;

        TEdgeId EdgeId(const TEdge& edge) const {
            TConstAdjacentIterator<NGraph::ANY> adj(this, edge.FirstId(), 0);
            for (; adj.IsValid(); ++adj) {
                if (&edge == &adj.Edge()) {
                    return adj.EdgeId();
                }
            }
            Y_FAIL();
        }

        bool GetEdgeIdByInternalIdx(const TVertexId v, const ui32 idxFind, ui32& result) const {
            TConstAdjacentIterator<NGraph::ANY> adj(this, v, idxFind);
            if (adj.IsValid()) {
                result = adj.EdgeId();
                return true;
            } else {
                return false;
            }
        }

        bool FindEdgeIdx(const TVertexId v, const TEdgeId edgeId, ui32& result) const {
            const auto actorCmp = [&edgeId](const TAdjacent& adj) -> bool {
                return adj.EdgeId() == edgeId;
            };
            const auto& adjs = Vertices_[v].Adjacents();
            auto it = std::find_if(adjs.begin(), adjs.end(), actorCmp);
            if (it->EdgeId() == edgeId) {
                result = it - adjs.begin();
                return true;
            }
            return false;
        }

        TVector<const TEdge*> Edge(const TVertexId& from, const TVertexId& to) const {
            TVector<const TEdge*> result;
            TConstAdjacentIterator<NGraph::ANY> adj(this, from, 0);
            for (; adj.IsValid(); ++adj) {
                TVertexId otherVertex = adj.VertexId();
                if (otherVertex == to) {
                    result.push_back(&adj.Edge());
                }
            }
            return result;
        }

        TVector<TEdge*> Edge(const TVertexId& from, const TVertexId& to) {
            TVector<TEdge*> result;
            TAdjacentIterator<NGraph::ANY> adj(this, from, 0);
            for (; adj.IsValid(); ++adj) {
                TVertexId otherVertex = adj.VertexId();
                if (otherVertex == to) {
                    result.push_back(&adj.Edge());
                }
            }
            return result;
        }

        const TEdge* EdgeOnce(const TVertexId& from, const TVertexId& to) const {
            TVector<const TEdge*> result = Edge(from, to);
            Y_VERIFY(result.size() == 1, "%lu", result.size());
            return result[0];
        }

        TEdge* EdgeOnce(const TVertexId& from, const TVertexId& to) {
            TVector<TEdge*> result = Edge(from, to);
            Y_VERIFY(result.size() == 1, "%lu", result.size());
            return result[0];
        }

        void ResetVertexProps(const TVertexProps& props);
        void ResetVertexProps() {
            ResetVertexProps(TVertexProps());
        }
        void Clear();

        // O(m) worst case complexity for all DELETE
        void DeleteEdge(TEdgeId id);
        template <EDirection Direction>
        void DeleteVertexEdges(TVertexId u);

        // glue v to u
        // O(m * log(m))
        // only for undirected graph
        template <class EdgeGluer>
        void GlueVerticesWithGivenGluer(TVertexId u, TVertexId v, const EdgeGluer& gluer);
        template <class EdgeGluer>
        void GlueVerticesWithGluer(TVertexId u, TVertexId v) {
            GlueVerticesWithGivenGluer(u, v, EdgeGluer());
        }
        void GlueVertices(TVertexId u, TVertexId v) {
            GlueVerticesWithGluer<TDefaultGluer>(u, v);
        }

        // O(1) worst case complexity for iterator creation

        TEdgeIterator EdgeIterator(TEdgeId id = 0);
        TConstEdgeIterator EdgeIterator(TEdgeId id = 0) const;

        template <EDirection Direction>
        typename TAbstractGraph<Traits>::template TAdjacentIterator<Direction> AdjacentIterator(TVertexId u);
        template <EDirection Direction>
        typename TAbstractGraph<Traits>::template TConstAdjacentIterator<Direction> AdjacentIterator(TVertexId u) const;

        // accepts sequence of vertex numbers
        template <class Iterator>
        TAbstractGraph CreateSubGraph(Iterator begin, Iterator end);

        // compressed graph takes less memory and has consisitent
        // edge ids less than number of edges
        void Compress();

        // human readable text serialization
        void DeserializeFromJson(const NJson::TJsonValue& in);
        void InitFromDescription(IInputStream& in);
        void InitFromDescription(const TStringType& string) {
            TStringInput in(string);
            InitFromDescription(in);
        }
        void SerializeToJson(NJson::TJsonValue& out) const;
        void PrintDescription(IOutputStream& out) const;
        void PrintDescription(TStringType& result) const {
            TStringOutput out(result);
            PrintDescription(out);
        }

        bool EdgeIdByVertexOffset(TVertexId u, TCount offset, TEdgeId& result) const {
            const auto& adjs = Vertices_[u].Adjacents();
            if (adjs.size() > offset) {
                result = adjs[offset].EdgeId();
                return true;
            } else {
                return false;
            }
        }

    private:
        using TEdgeVector = typename Traits::template TVectorT<TEdge>::T;
        using TVertexVector = typename Traits::template TVectorT<TVertex>::T;
        using TAdjacentVector = typename Traits::template TVectorT<TAdjacent>::T;

        typedef typename Traits::TStringOutputT TStringOutput;
        typedef typename Traits::TStringInputT TStringInput;

    private:
        typename TAdjacentVector::const_iterator AdjacentIt(TVertexId u, TCount offset) const {
            if (Vertices_[u].Adjacents().size() > offset) {
                return Vertices_[u].Adjacents().begin() + offset;
            } else {
                return Vertices_[u].Adjacents().end();
            }
        }

        typename TAdjacentVector::const_iterator AdjacentEnd(TVertexId u) const {
            return Vertices_[u].Adjacents().end();
        }

        template <bool ShouldCheckCompress>
        void DeleteEdgeInternal(TEdgeId edgeId);

        void CheckCompress();
        TCount NumEdgesWithDeleted() const;
        // also with deleted
        TCount AdjacentCount(TVertexId u) const;
        TAdjacent& Adjacent(TVertexId u, TCount offset);
        TAdjacent const& Adjacent(TVertexId u, TCount offset) const;
        void AddAdjacents(TVertexId u, TVertexId v, TEdgeId id);
        bool IsDeleted(TEdgeId edgeId) const;

    private:
        TEdgeVector Edges_;
        TVertexVector Vertices_;
        TCount DelCount_ = 0;
    };

}

// IMPLEMENTATION

namespace NGraph {
    template <class Traits>
    TAbstractGraph<Traits>::TAbstractGraph(TCount numVertices) {
        ReserveMemory(10000, 10000);
        for (TCount i = 0; i < numVertices; i++)
            AddVertex();
    }

    template <class Traits>
    TAbstractGraph<Traits>::TAbstractGraph(TCount numVertices, TVertexProps const& props) {
        ReserveMemory(10000, 10000);
        for (TCount i = 0; i < numVertices; i++)
            AddVertex(props);
    }

    template <class Traits>
    template <class OtherTraits>
    TAbstractGraph<Traits>::TAbstractGraph(TAbstractGraph<OtherTraits> other)
        : DelCount_(0)
    {
        Vertices_.reserve(other.NumVertices());
        for (TCount i = 0; i < other.NumVertices(); i++)
            AddVertex(TVertexProps(other.Vertex(i).Props()));
        Edges_.reserve(other.NumEdges());
        for (typename TAbstractGraph<OtherTraits>::TEdgeIterator it = other.EdgeIterator();
             it.IsValid(); ++it)
            AddEdge(it.FirstId(), it.SecondId(), TEdgeProps(it.Edge().Props()));
    }

    template <class Traits>
    typename TAbstractGraph<Traits>::TCount
    TAbstractGraph<Traits>::NumVertices() const {
        return Vertices_.size();
    }

    template <class Traits>
    typename TAbstractGraph<Traits>::TCount
    TAbstractGraph<Traits>::NumEdges() const {
        return Edges_.size() - DelCount_;
    }

    template <class Traits>
    typename TAbstractGraph<Traits>::TCount
    TAbstractGraph<Traits>::Degree(TVertexId u) const {
        return Vertex(u).Degree();
    }

    template <class Traits>
    typename TAbstractGraph<Traits>::TCount
    TAbstractGraph<Traits>::NumEdgesWithDeleted() const {
        return Edges_.size();
    }

    template <class Traits>
    void TAbstractGraph<Traits>::CheckCompress() {
        //if (2 * DelCount_ > Edges_.size())
        //    Compress();
    }

    template <class Traits>
    typename TAbstractGraph<Traits>::TVertexId
    TAbstractGraph<Traits>::AddVertex(TVertexProps const& props) {
        TVertexId newId = Vertices_.size();
        Vertices_.emplace_back(props);
        return newId;
    }

    template <class Traits>
    typename TAbstractGraph<Traits>::TEdgeIterator
    TAbstractGraph<Traits>::AddEdge(TVertexId u, TVertexId v, TEdgeProps const& props) {
        Y_ASSERT(u < NumVertices());
        Y_ASSERT(v < NumVertices());
        Y_ASSERT(u != v);

        TEdgeId id = Edges_.size();
        AddAdjacents(u, v, id);
        Edges_.emplace_back(u, v, props);
        return TEdgeIterator(this, id);
    }

    template <class Traits>
    typename TAbstractGraph<Traits>::TEdge&
    TAbstractGraph<Traits>::Edge(TEdgeId id) {
        return Edges_[id];
    }

    template <class Traits>
    typename TAbstractGraph<Traits>::TEdge const&
    TAbstractGraph<Traits>::Edge(TEdgeId id) const {
        return Edges_[id];
    }

    template <class Traits>
    typename TAbstractGraph<Traits>::TEdgeIterator
    TAbstractGraph<Traits>::EdgeIterator(TEdgeId id) {
        return TEdgeIterator(this, id);
    }

    template <class Traits>
    typename TAbstractGraph<Traits>::TConstEdgeIterator
    TAbstractGraph<Traits>::EdgeIterator(TEdgeId id) const {
        return TConstEdgeIterator(this, id);
    }

    template <class Traits>
    template <EDirection Direction>
    typename TAbstractGraph<Traits>::template TAdjacentIterator<Direction>
    TAbstractGraph<Traits>::AdjacentIterator(TVertexId u) {
        return TAdjacentIterator<Direction>(this, u, 0);
    }

    template <class Traits>
    template <EDirection Direction>
    typename TAbstractGraph<Traits>::template TConstAdjacentIterator<Direction>
    TAbstractGraph<Traits>::AdjacentIterator(TVertexId u) const {
        return TConstAdjacentIterator<Direction>(this, u, 0);
    }

    template <class Traits>
    typename TAbstractGraph<Traits>::TVertex&
    TAbstractGraph<Traits>::Vertex(TVertexId u) {
        return Vertices_[u];
    }

    template <class Traits>
    typename TAbstractGraph<Traits>::TVertex const&
    TAbstractGraph<Traits>::Vertex(TVertexId u) const {
        return Vertices_[u];
    }

    template <class Traits>
    void TAbstractGraph<Traits>::ResetVertexProps(const TVertexProps& props) {
        for (TCount i = 0; i < NumVertices(); i++)
            static_cast<TVertexProps&>(Vertex(i)) = props;
    }

    template <class Traits>
    void TAbstractGraph<Traits>::Clear() {
        Vertices_.clear();
        Edges_.clear();
        DelCount_ = 0;
    }

    template <class Traits>
    typename TAbstractGraph<Traits>::TCount
    TAbstractGraph<Traits>::AdjacentCount(TVertexId u) const {
        return Vertices_[u].Adjacents().size();
    }

    template <class Traits>
    typename TAbstractGraph<Traits>::TAdjacent&
    TAbstractGraph<Traits>::Adjacent(TVertexId u, TCount offset) {
        return Vertices_[u].Adjacents()[offset];
    }

    template <class Traits>
    typename TAbstractGraph<Traits>::TAdjacent const&
    TAbstractGraph<Traits>::Adjacent(TVertexId u, TCount offset) const {
        return Vertices_[u].Adjacents()[offset];
    }

    template <class Traits>
    void TAbstractGraph<Traits>::AddAdjacents(TVertexId u, TVertexId v, TEdgeId id) {
        Y_ASSERT(Vertices_[u].Adjacents().empty() || Vertices_[u].Adjacents().back() < TAdjacent(id));
        Y_ASSERT(Vertices_[v].Adjacents().empty() || Vertices_[v].Adjacents().back() < TAdjacent(id));
        Vertices_[u].Adjacents().push_back(TAdjacent(id));
        Vertices_[v].Adjacents().push_back(TAdjacent(id));
    }

    template <class Traits>
    template <EDirection Direction>
    void TAbstractGraph<Traits>::DeleteVertexEdges(TVertexId u) {
        for (TAdjacentIterator<Direction> it = AdjacentIterator<Direction>(u);
             it.IsValid(); ++it)
            DeleteEdgeInternal<false>(it.EdgeId());
        Vertices_[u].CheckCompress(this);
        CheckCompress();
    }

    template <class Traits>
    void TAbstractGraph<Traits>::DeleteEdge(TEdgeId edgeId) {
        DeleteEdgeInternal<true>(edgeId);
    }

    template <class Traits>
    template <bool ShouldCheckCompress>
    void TAbstractGraph<Traits>::DeleteEdgeInternal(TEdgeId edgeId) {
        TEdge& edge = Edge(edgeId);
        TVertexId first = edge.FirstId();
        TVertexId second = edge.SecondId();

        Vertices_[first].DelCount()++;
        Vertices_[second].DelCount()++;
        edge.SetDeleted();
        DelCount_++;

        if (ShouldCheckCompress) {
            CheckCompress();

            Vertices_[first].CheckCompress(this);
            Vertices_[second].CheckCompress(this);
        }
    }

    template <class T>
    class TCompareByVertexId {
    private:
        typedef typename T::TParent TParent;
        typedef typename TParent::TVertexId TVertexId;

    public:
        TCompareByVertexId(TParent const* parent, TVertexId vertexId)
            : Parent_(parent)
            , VertexId_(vertexId)
        {
        }

        bool operator()(const T& lft, const T& rgh) const {
            return lft.VertexId(Parent_, VertexId_) <
                   rgh.VertexId(Parent_, VertexId_);
        }

    private:
        TParent const* Parent_;
        TVertexId VertexId_;
    };

    template <class Traits>
    template <class EdgeGluer>
    void TAbstractGraph<Traits>::GlueVerticesWithGivenGluer(TVertexId u, TVertexId v,
                                                            EdgeGluer const& gluer) {
        // WARNING: for undirected graphs

        TAdjacentVector& uVector = Vertices_[u].Adjacents();
        TAdjacentVector& vVector = Vertices_[v].Adjacents();
        TTraits::Sort(uVector.begin(), uVector.end(), TCompareByVertexId<TAdjacent>(this, u));
        TTraits::Sort(vVector.begin(), vVector.end(), TCompareByVertexId<TAdjacent>(this, v));

        TCount uIndex = 0, vIndex = 0;
        TCount uLimit = uVector.size(), vLimit = vVector.size();
        while (uIndex < uLimit || vIndex < vLimit) {
            TVertexId uVertex, vVertex;
            TEdgeId uEdgeId, vEdgeId;

            uVertex = vVertex = NumVertices();
            if (uIndex < uLimit) {
                uEdgeId = uVector[uIndex].EdgeId();
                uVertex = uVector[uIndex].VertexId(this, u);
                if (Edge(uEdgeId).IsDeleted()) {
                    uIndex++;
                    continue;
                }
            }
            if (vIndex < vLimit) {
                vEdgeId = vVector[vIndex].EdgeId();
                vVertex = vVector[vIndex].VertexId(this, v);
                if (Edge(vEdgeId).IsDeleted()) {
                    vIndex++;
                    continue;
                }
            }

            if (uVertex < vVertex) {
                if (uVertex == v)
                    DeleteEdgeInternal<false>(uVector[uIndex].EdgeId());
                uIndex++;
            } else if (uVertex > vVertex) {
                if (vVertex != u) {
                    AddEdge(u, vVertex, Edge(vVector[vIndex].EdgeId()));
                }
                vIndex++;
            } else {
                gluer.GlueEdges(*this, uVector[uIndex].EdgeId(), vVector[vIndex].EdgeId());
                uIndex++;
                vIndex++;
            }
        }

        DeleteVertexEdges<ANY>(v);
    }

    template <class Traits>
    bool TAbstractGraph<Traits>::IsDeleted(TEdgeId edgeId) const {
        return Edge(edgeId).IsDeleted();
    }

    template <class Traits>
    void TAbstractGraph<Traits>::Compress() {
        for (TCount i = 0; i < NumVertices(); i++) {
            TCount degree = Degree(i);
            Vertices_[i].Reset();
            Vertices_[i].Adjacents().reserve(degree);
        }

        TEdgeVector newEdges;
        newEdges.reserve(NumEdges());
        for (TEdgeIterator it = EdgeIterator(); it.IsValid(); ++it)
            newEdges.push_back(*it);

        DelCount_ = 0;

        Edges_.swap(newEdges);
        for (TCount i = 0; i < NumEdges(); i++)
            AddAdjacents(Edge(i).FirstId(), Edge(i).SecondId(), i);
    }

    template <class Traits>
    void TAbstractGraph<Traits>::PrintDescription(IOutputStream& out) const {
        out << NumVertices() << ' ' << NumEdges() << '\n';
        for (TCount i = 0; i < NumVertices(); i++)
            out << static_cast<TVertexProps const&>(Vertex(i)) << '\n';
        for (TConstEdgeIterator it = EdgeIterator(); it.IsValid(); ++it)
            out << it.FirstId() << ' ' << it.SecondId() << ' '
                << static_cast<TEdgeProps const&>(it.Edge()) << '\n';
    }

    template <class Traits>
    void TAbstractGraph<Traits>::InitFromDescription(IInputStream& in) {
        Y_ASSERT(NumVertices() == 0 && NumEdges() == 0);

        TCount numVertices, numEdges;
        in >> numVertices >> numEdges;

        Vertices_.reserve(numVertices);
        for (TCount i = 0; i < numVertices; i++) {
            TVertexProps props;
            in >> props;
            AddVertex(props);
        }

        for (TCount i = 0; i < numEdges; i++) {
            TVertexId u, v;
            TEdgeProps props;
            in >> u >> v >> props;
            AddEdge(u, v, props);
        }
    }

    template <class Traits>
    void TAbstractGraph<Traits>::SerializeToJson(NJson::TJsonValue& out) const {
        NJson::TJsonValue result;
        NJson::TJsonValue edges(NJson::JSON_ARRAY);
        NJson::TJsonValue vertices(NJson::JSON_ARRAY);
        for (TCount i = 0; i < NumVertices(); i++) {
            vertices.AppendValue(static_cast<TVertexProps const&>(Vertex(i)).Serialize());
        }
        for (TConstEdgeIterator it = EdgeIterator(); it.IsValid(); ++it) {
            NJson::TJsonValue edge;
            edge.InsertValue("f", it.FirstId());
            edge.InsertValue("t", it.SecondId());
            edge.InsertValue("p", static_cast<TEdgeProps const&>(it.Edge()).Serialize());
            edges.AppendValue(edge);
        }
        result.InsertValue("edges", edges);
        result.InsertValue("vertices", vertices);
        out = result;
    }

    template <class Traits>
    void TAbstractGraph<Traits>::DeserializeFromJson(const NJson::TJsonValue& in) {
        Y_ASSERT(NumVertices() == 0 && NumEdges() == 0);

        NJson::TJsonValue::TArray vertices;
        NJson::TJsonValue::TArray edges;
        if (!in["edges"].GetArray(&edges))
            ythrow yexception() << "Incorrect edges value: " << in["edges"].GetStringRobust();

        if (!in["vertices"].GetArray(&vertices))
            ythrow yexception() << "Incorrect vertices value: " << in["vertices"].GetStringRobust();

        Vertices_.reserve(vertices.size());
        for (auto& i : vertices) {
            TVertexProps props;
            props.Deserialize(i);
            AddVertex(props);
        }

        for (auto& i : edges) {
            TVertexId u = i["f"].GetUInteger();
            TVertexId v = i["t"].GetUInteger();
            TEdgeProps props;
            props.Deserialize(i["p"]);
            AddEdge(u, v, props);
        }
    }

    template <class Traits>
    template <class Iterator>
    TAbstractGraph<Traits> TAbstractGraph<Traits>::CreateSubGraph(Iterator begin, Iterator end) {
        typedef typename TTraits::template TMapT<TVertexId, TVertexId>::T TMapType;
        TMapType map;

        TVertexId num = 0;
        for (Iterator cur = begin; cur != end; cur++, num++)
            map[*cur] = num;

        TAbstractGraph result(map.size());
        num = 0;
        for (Iterator cur = begin; cur != end; cur++, num++)
            for (TAdjacentIterator<ANY> it = AdjacentIterator<ANY>(*cur); it.IsValid(); ++it) {
                typename TMapType::iterator jt = map.find(it.VertexId());
                if (jt == map.end() || it.Edge().IsForward(*cur))
                    continue;
                result.AddEdge(num, jt->second, it.Edge());
            }

        return result;
    }

}
