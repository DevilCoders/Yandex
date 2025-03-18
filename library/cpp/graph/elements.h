#pragma once

#ifdef USE_CASSERT
#include <cassert>
#define Y_ASSERT assert
#else
#include <util/system/yassert.h>
#endif

#include <util/stream/output.h>

namespace NGraph {
    template <class Traits>
    class TAbstractGraph;
};

// GLOBAL CONSTANTS

namespace NGraph {
    enum EDirection {
        ANY,
        FORWARD,
        BACKWARD
    };
};

// INTERFACE

namespace NGraph {
    namespace NPrivate {
        template <class X>
        struct THelper {
            typedef X Y;
        };

        template <class Graph>
        class TVertex: public Graph::TVertexProps {
        public:
            typedef Graph TParent;
            template <class Traits>
            friend class NGraph::TAbstractGraph;

            typedef typename Graph::TVertexProps TProps;

        public:
            typedef typename TParent::TAdjacentVector TAdjacentVector;
            typedef typename TParent::TCount TCount;

            TVertex();
            TVertex(const TProps& props);

            TCount Degree() const;
            TProps& Props() {
                return static_cast<TProps&>(*this);
            }
            TProps const& Props() const {
                return static_cast<TProps const&>(*this);
            }

        private:
            TAdjacentVector& Adjacents();
            TAdjacentVector const& Adjacents() const;
            TCount& DelCount();
            TCount const& DelCount() const;

            void Reset();
            void CheckCompress(TParent const* parent);

        private:
            TAdjacentVector Adjacents_;
            TCount DelCount_;
        };

        template <class Graph>
        class TEdge: public Graph::TEdgeProps {
        public:
            typedef Graph TParent;
            template <class Traits>
            friend class NGraph::TAbstractGraph;

        private:
            typedef typename TParent::TVertexId TVertexId;
            typedef typename TParent::TEdgeId TEdgeId;
            typedef typename TParent::TEdgeProps TProps;

        public:
            // for future serialization constructor with no parameters is necessary
            TEdge();
            TEdge(TVertexId first, TVertexId second, const TProps& props);

            Y_FORCE_INLINE TVertexId FirstId() const {
                return FirstId_;
            }

            Y_FORCE_INLINE TVertexId SecondId() const {
                return IdSum_ - FirstId_;
            }

            Y_FORCE_INLINE TVertexId OtherId(const TVertexId thisId) const {
                return IdSum_ - thisId;
            }

            Y_FORCE_INLINE TVertexId SafeOtherId(const TVertexId thisId) const {
                if (thisId == FirstId()) {
                    return SecondId();
                } else if (thisId == SecondId()) {
                    return FirstId();
                } else {
                    Y_FAIL("Incorrect vertex id");
                }
            }

            Y_FORCE_INLINE bool IsForward(TVertexId thisId) const;

            bool HasId(TVertexId thisId) const;
            bool Connects(const TEdge& edge) const {
                return HasId(edge.FirstId()) || HasId(edge.SecondId());
            }
            // direction insensitive
            Y_FORCE_INLINE bool Connects(TVertexId u, TVertexId v) const;

            TProps& Props() {
                return static_cast<TProps&>(*this);
            }
            TProps const& Props() const {
                return static_cast<const TProps&>(*this);
            }

        private:
            void SetDeleted();
            bool IsDeleted() const;

        private:
            TVertexId FirstId_, IdSum_;
        };

        template <class Graph>
        class TAdjacent {
        public:
            template <class Traits>
            friend class NGraph::TAbstractGraph;
            typedef Graph TParent;

        private:
            using TSelf = TAdjacent<Graph>;
            typedef typename TParent::TVertexId TVertexId;
            typedef typename TParent::TEdge TEdge;
            typedef typename TParent::TEdgeId TEdgeId;

        public:
            TAdjacent(TEdgeId edgeId);

            TVertexId VertexId(TParent const* parent, TVertexId ownerId) const;
            TEdgeId EdgeId() const;
            bool IsDeleted(TParent const* parent) const;
            bool operator<(const TSelf& item) const {
                return EdgeId_ < item.EdgeId_;
            }

        private:
            TEdgeId EdgeId_;
        };

        template <class ParentPointer>
        class TChild {
        public:
            TChild(ParentPointer parent);

            ParentPointer Parent() const;

        private:
            ParentPointer Parent_;
        };

        template <class Graph>
        class TEdgeIterator: public TChild<Graph*> {
        public:
            template <class Traits>
            friend class NGraph::TAbstractGraph;
            typedef TChild<Graph*> TBase;

        private:
            typedef Graph TParent;
            typedef typename TParent::TVertex TVertex;
            typedef typename TParent::TVertexId TVertexId;
            typedef typename TParent::TEdge TEdge;
            typedef typename TParent::TEdgeId TEdgeId;
            typedef typename TParent::TCount TCount;

        public:
            Y_FORCE_INLINE bool IsValid() const;

            Y_FORCE_INLINE TEdge& Edge() const;
            Y_FORCE_INLINE TEdgeId EdgeId() const;

            Y_FORCE_INLINE TVertexId FirstId() const {
                return Edge().FirstId();
            }
            Y_FORCE_INLINE TVertexId SecondId() const {
                return Edge().SecondId();
            }
            Y_FORCE_INLINE TVertex& First() const;
            Y_FORCE_INLINE TVertex& Second() const;

            TEdge const& operator*() {
                return Edge();
            }
            TEdge const* operator->() {
                return &Edge();
            }

            TEdgeIterator& operator++();
            TEdgeIterator& operator++(int) {
                return operator++();
            }

        private:
            TEdgeIterator(TParent* parent, TEdgeId edgeId);
            void SkipDeleted();

        private:
            TEdgeId EdgeId_;
        };

        template <class Graph>
        class TConstEdgeIterator: public TChild<Graph const*> {
        private:
            template <class Traits>
            friend class NGraph::TAbstractGraph;
            typedef TChild<Graph const*> TBase;
            typedef Graph TParent;
            typedef typename TParent::TVertex TVertex;
            typedef typename TParent::TEdge TEdge;
            typedef typename TParent::TVertexId TVertexId;
            typedef typename TParent::TEdgeId TEdgeId;
            typedef typename TParent::TCount TCount;

        public:
            bool IsValid() const;

            TEdge const& Edge() const;
            TEdgeId EdgeId() const;

            TVertexId FirstId() const {
                return Edge().FirstId();
            }
            TVertexId SecondId() const {
                return Edge().SecondId();
            }
            TVertex const& First();
            TVertex const& Second();

            TEdge const& operator*() {
                return Edge();
            }
            TEdge const* operator->() {
                return &Edge();
            }

            TConstEdgeIterator& operator++();
            TConstEdgeIterator& operator++(int) {
                operator++();
            }

        private:
            TConstEdgeIterator(TParent const* parent, TEdgeId edgeId);
            void SkipDeleted();

        private:
            TEdgeId EdgeId_;
        };

        template <class Graph, EDirection Direction>
        class TAdjacentIterator: public TChild<Graph*> {
        public:
            typedef Graph TParent;

        protected:
            typedef TChild<Graph*> TBase;
            typedef typename TParent::TCount TCount;
            typedef typename TParent::TVertex TVertex;
            typedef typename TParent::TEdge TEdge;
            typedef typename TParent::TVertexId TVertexId;
            typedef typename TParent::TEdgeId TEdgeId;

        public:
            TAdjacentIterator(TParent* parent, TVertexId vertexId, TCount offset);

            bool IsValid() const;

            // O(1) worst case complexity for all GET methods
            TVertexId VertexId() const;
            TEdgeId EdgeId() const;

            TVertex& Vertex() const;
            TEdge& Edge() const;

            TVertex& operator*() const {
                return Vertex();
            }
            TVertex* operator->() const {
                return &Vertex();
            }

            // O(1) amortized complexity
            // rare cache faults are desirable
            TAdjacentIterator& operator++();
            TAdjacentIterator& operator++(int) {
                return operator++();
            }

        private:
            void SkipDeleted();

        public:
            static const EDirection DIRECTION = Direction;

        private:
            TVertexId VertexId_;
            typename Graph::TAdjacentVector::const_iterator Offset_;
            typename Graph::TAdjacentVector::const_iterator OffsetEnd_;
        };

        template <class Graph, EDirection Direction>
        class TConstAdjacentIterator: public TChild<Graph const*> {
        public:
            typedef Graph TParent;

        protected:
            typedef TChild<Graph const*> TBase;
            typedef typename TParent::TCount TCount;
            typedef typename TParent::TVertex TVertex;
            typedef typename TParent::TEdge TEdge;
            typedef typename TParent::TVertexId TVertexId;
            typedef typename TParent::TEdgeId TEdgeId;
            typedef typename TParent::TVertexProps TVertexProps;

        public:
            TConstAdjacentIterator(TParent const* parent, TVertexId vertexId, TCount offset);

            bool IsValid() const;

            // O(1) worst case complexity for all GET methods
            TVertexId VertexId() const;
            TEdgeId EdgeId() const;

            TVertex const& Vertex() const;
            TEdge const& Edge() const;

            TVertex const& operator*() const {
                return Vertex();
            }
            TVertex const* operator->() const {
                return Vertex();
            }

            // O(1) amortized complexity
            // rare cache faults are desirable
            Y_FORCE_INLINE TConstAdjacentIterator& Next() {
                return ++(*this);
            }

            Y_FORCE_INLINE TConstAdjacentIterator& operator++() {
                ++Offset_;
                SkipDeleted();
                return *this;
            }

            TConstAdjacentIterator& operator++(int) {
                return operator++();
            }

        private:
            void SkipDeleted();

        public:
            static const EDirection DIRECTION = Direction;

        private:
            TVertexId VertexId_;
            typename Graph::TAdjacentVector::const_iterator Offset_;
            typename Graph::TAdjacentVector::const_iterator OffsetEnd_;
        };

        template <class Graph>
        struct TDefaultGluer {
            typedef typename Graph::TEdgeId TEdgeId;

            void GlueEdges(Graph&, TEdgeId, TEdgeId) const {
            }
        };
    }

}

// IMPLEMENTATION

namespace NGraph {
    namespace NPrivate {
        // TChild

        template <class ParentPointer>
        TChild<ParentPointer>::TChild(ParentPointer parent)
            : Parent_(parent)
        {
        }

        template <class ParentPointer>
        ParentPointer TChild<ParentPointer>::Parent() const {
            return Parent_;
        }

        // TEdgeIterator

        template <class Graph>
        TEdgeIterator<Graph>::TEdgeIterator(TParent* parent, TEdgeId edgeId)
            : TBase(parent)
            , EdgeId_(edgeId)
        {
            SkipDeleted();
        }

        template <class Graph>
        bool TEdgeIterator<Graph>::IsValid() const {
            return EdgeId_ < TBase::Parent()->NumEdgesWithDeleted();
        }

        template <class Graph>
        typename TEdgeIterator<Graph>::TEdge& TEdgeIterator<Graph>::Edge() const {
            return TBase::Parent()->Edge(EdgeId());
        }

        template <class Graph>
        typename TEdgeIterator<Graph>::TEdgeId TEdgeIterator<Graph>::EdgeId() const {
            return EdgeId_;
        }

        template <class Graph>
        typename TEdgeIterator<Graph>::TVertex& TEdgeIterator<Graph>::First() const {
            return TBase::Parent()->Vertex(Edge().FirstId());
        }

        template <class Graph>
        typename TEdgeIterator<Graph>::TVertex& TEdgeIterator<Graph>::Second() const {
            return TBase::Parent()->Vertex(Edge().SecondId());
        }

        template <class Graph>
        TEdgeIterator<Graph>& TEdgeIterator<Graph>::operator++() {
            Y_ASSERT(IsValid());
            EdgeId_++;
            SkipDeleted();
            return *this;
        }

        template <class Graph>
        void TEdgeIterator<Graph>::SkipDeleted() {
            for (; IsValid() && TBase::Parent()->IsDeleted(EdgeId_); EdgeId_++)
                ;
        }

        // TConstEdgeIterator

        template <class Graph>
        TConstEdgeIterator<Graph>::TConstEdgeIterator(TParent const* parent, TEdgeId edgeId)
            : TBase(parent)
            , EdgeId_(edgeId)
        {
            SkipDeleted();
        }

        template <class Graph>
        bool TConstEdgeIterator<Graph>::IsValid() const {
            return EdgeId_ < TBase::Parent()->NumEdgesWithDeleted();
        }

        template <class Graph>
        typename TConstEdgeIterator<Graph>::TEdge const& TConstEdgeIterator<Graph>::Edge() const {
            return TBase::Parent()->Edge(EdgeId());
        }

        template <class Graph>
        typename TConstEdgeIterator<Graph>::TEdgeId TConstEdgeIterator<Graph>::EdgeId() const {
            return EdgeId_;
        }

        template <class Graph>
        TConstEdgeIterator<Graph>& TConstEdgeIterator<Graph>::operator++() {
            Y_ASSERT(IsValid());
            EdgeId_++;
            SkipDeleted();
            return *this;
        }

        template <class Graph>
        void TConstEdgeIterator<Graph>::SkipDeleted() {
            for (; IsValid() && TBase::Parent()->IsDeleted(EdgeId_); EdgeId_++)
                ;
        }

        // TAdjacentIterator

        template <class Graph, EDirection Dir>
        TAdjacentIterator<Graph, Dir>::TAdjacentIterator(TParent* parent, TVertexId vertexId, TCount offset)
            : TBase(parent)
            , VertexId_(vertexId)
            , Offset_(TBase::Parent()->AdjacentIt(VertexId_, offset))
            , OffsetEnd_(TBase::Parent()->AdjacentEnd(VertexId_))
        {
            SkipDeleted();
        }

        template <class Graph, EDirection Dir>
        Y_FORCE_INLINE bool TAdjacentIterator<Graph, Dir>::IsValid() const {
            return Offset_ != OffsetEnd_;
        }

        template <class Graph, EDirection Dir>
        typename TAdjacentIterator<Graph, Dir>::TVertex& TAdjacentIterator<Graph, Dir>::Vertex() const {
            Y_ASSERT(IsValid());
            return TBase::Parent()->Vertex(VertexId());
        }

        template <class Graph, EDirection Dir>
        typename TAdjacentIterator<Graph, Dir>::TEdge& TAdjacentIterator<Graph, Dir>::Edge() const {
            Y_ASSERT(IsValid());
            return TBase::Parent()->Edge(EdgeId());
        }

        template <class Graph, EDirection Dir>
        typename TAdjacentIterator<Graph, Dir>::TVertexId TAdjacentIterator<Graph, Dir>::VertexId() const {
            Y_ASSERT(IsValid());
            return Offset_->VertexId(TBase::Parent(), VertexId_);
        }

        template <class Graph, EDirection Dir>
        Y_FORCE_INLINE typename TAdjacentIterator<Graph, Dir>::TEdgeId TAdjacentIterator<Graph, Dir>::EdgeId() const {
            Y_ASSERT(IsValid());
            return Offset_->EdgeId();
        }

        template <class Graph, EDirection Dir>
        Y_FORCE_INLINE TAdjacentIterator<Graph, Dir>& TAdjacentIterator<Graph, Dir>::operator++() {
            ++Offset_;
            SkipDeleted();
            return *this;
        }

        template <class Graph, EDirection Dir>
        Y_FORCE_INLINE void TAdjacentIterator<Graph, Dir>::SkipDeleted() {
            TParent const* parent = TBase::Parent();
            for (; Offset_ != OffsetEnd_; ++Offset_) {
                TEdgeId id = Offset_->EdgeId();
                TEdge const& edge = parent->Edge(id);
                Y_UNUSED(edge);
                if (!parent->IsDeleted(id) &&
                    (((DIRECTION == ANY)) ||
                     ((DIRECTION == FORWARD && edge.IsForward(VertexId_))) ||
                     ((DIRECTION == BACKWARD && !edge.IsForward(VertexId_)))))
                    return;
            }
        }

        // TConstAdjacentIterator

        template <class Graph, EDirection Dir>
        TConstAdjacentIterator<Graph, Dir>::
            TConstAdjacentIterator(TParent const* parent, TVertexId vertexId, TCount offset)
            : TBase(parent)
            , VertexId_(vertexId)
            , Offset_(TBase::Parent()->AdjacentIt(vertexId, offset))
            , OffsetEnd_(TBase::Parent()->AdjacentEnd(vertexId))
        {
            SkipDeleted();
        }

        template <class Graph, EDirection Dir>
        Y_FORCE_INLINE bool TConstAdjacentIterator<Graph, Dir>::IsValid() const {
            return Offset_ != OffsetEnd_;
        }

        template <class Graph, EDirection Dir>
        typename TConstAdjacentIterator<Graph, Dir>::TVertex const&
        TConstAdjacentIterator<Graph, Dir>::Vertex() const {
            Y_ASSERT(IsValid());
            return TBase::Parent()->Vertex(VertexId());
        }

        template <class Graph, EDirection Dir>
        typename TConstAdjacentIterator<Graph, Dir>::TEdge const&
        TConstAdjacentIterator<Graph, Dir>::Edge() const {
            Y_ASSERT(IsValid());
            return TBase::Parent()->Edge(EdgeId());
        }

        template <class Graph, EDirection Dir>
        typename TConstAdjacentIterator<Graph, Dir>::TVertexId
        TConstAdjacentIterator<Graph, Dir>::VertexId() const {
            Y_ASSERT(IsValid());
            return Offset_->VertexId(TBase::Parent(), VertexId_);
        }

        template <class Graph, EDirection Dir>
        typename TConstAdjacentIterator<Graph, Dir>::TEdgeId
        TConstAdjacentIterator<Graph, Dir>::EdgeId() const {
            Y_ASSERT(IsValid());
            return Offset_->EdgeId();
        }

        template <class Graph, EDirection Dir>
        void TConstAdjacentIterator<Graph, Dir>::SkipDeleted() {
            TParent const* parent = TBase::Parent();
            for (; Offset_ != OffsetEnd_; ++Offset_) {
                TEdgeId id = Offset_->EdgeId();
                TEdge const& edge = parent->Edge(id);
                Y_UNUSED(edge);
                if (!parent->IsDeleted(id) &&
                    (((DIRECTION == ANY)) ||
                     ((DIRECTION == FORWARD && edge.IsForward(VertexId_))) ||
                     ((DIRECTION == BACKWARD && !edge.IsForward(VertexId_)))))
                    return;
            }
        }

        // TVertex

        template <class Graph>
        TVertex<Graph>::TVertex()
            : DelCount_(0)
        {
        }

        template <class Graph>
        TVertex<Graph>::TVertex(TProps const& props)
            : TProps(props)
            , DelCount_(0)
        {
        }

        template <class Graph>
        typename TVertex<Graph>::TAdjacentVector& TVertex<Graph>::Adjacents() {
            return Adjacents_;
        }

        template <class Graph>
        typename TVertex<Graph>::TAdjacentVector const& TVertex<Graph>::Adjacents() const {
            return Adjacents_;
        }

        template <class Graph>
        typename TVertex<Graph>::TCount& TVertex<Graph>::DelCount() {
            return DelCount_;
        }

        template <class Graph>
        typename TVertex<Graph>::TCount const& TVertex<Graph>::DelCount() const {
            return DelCount_;
        }

        template <class Graph>
        typename TVertex<Graph>::TCount TVertex<Graph>::Degree() const {
            return Adjacents_.size() - DelCount_;
        }

        template <class Graph>
        void TVertex<Graph>::Reset() {
            TAdjacentVector tmp;
            Adjacents_.swap(tmp);
            DelCount_ = 0;
        }

        template <class Graph>
        void TVertex<Graph>::CheckCompress(TParent const* parent) {
            if (2 * DelCount_ > Adjacents_.size()) {
                TAdjacentVector newAdjacents;
                newAdjacents.reserve(Degree());
                for (TCount i = 0; i < Adjacents_.size(); i++)
                    if (!Adjacents_[i].IsDeleted(parent))
                        newAdjacents.push_back(Adjacents_[i]);
                Adjacents_.swap(newAdjacents);

                DelCount_ = 0;
            }
        }

        // TEdge

        template <class Graph>
        TEdge<Graph>::TEdge() = default;

        template <class Graph>
        TEdge<Graph>::TEdge(TVertexId first, TVertexId second, const TProps& props)
            : TProps(props)
            , FirstId_(first)
            , IdSum_(first + second)
        {
        }

        template <class Graph>
        bool TEdge<Graph>::IsForward(TVertexId thisId) const {
            return thisId == FirstId();
        }

        template <class Graph>
        bool TEdge<Graph>::HasId(TVertexId thisId) const {
            return thisId == FirstId() || thisId == SecondId();
        }

        template <class Graph>
        void TEdge<Graph>::SetDeleted() {
            FirstId_ = TParent::NULL_VERTEX();
        }

        template <class Graph>
        bool TEdge<Graph>::IsDeleted() const {
            return FirstId_ == TParent::NULL_VERTEX();
        }

        template <class Graph>
        bool TEdge<Graph>::Connects(TVertexId u, TVertexId v) const {
            return (FirstId() == u && SecondId() == v) ||
                   (FirstId() == v && SecondId() == u);
        }

        // TAdjacent

        template <class Graph>
        TAdjacent<Graph>::TAdjacent(TEdgeId edgeId)
            : EdgeId_(edgeId)
        {
        }

        template <class Graph>
        bool TAdjacent<Graph>::IsDeleted(TParent const* parent) const {
            return parent->IsDeleted(EdgeId_);
        }

        template <class Graph>
        typename TAdjacent<Graph>::TVertexId Y_FORCE_INLINE TAdjacent<Graph>::VertexId(TParent const* parent, TVertexId ownerId) const {
            return parent->Edge(EdgeId_).OtherId(ownerId);
        }

        template <class Graph>
        Y_FORCE_INLINE typename TAdjacent<Graph>::TEdgeId TAdjacent<Graph>::EdgeId() const {
            return EdgeId_;
        }

    }

}
