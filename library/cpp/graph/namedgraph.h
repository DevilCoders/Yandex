#pragma once

#include "graph.h"

#include <util/stream/output.h>

namespace NGraph {
    namespace NPrivate {
        template <class N, class TVertexProps>
        class TNamedVertexProps: public TVertexProps {
        public:
            TNamedVertexProps() = default;

            TNamedVertexProps(N const& name)
                : Name(name)
            {
            }

            TNamedVertexProps(N const& name, TVertexProps const& props)
                : TVertexProps(props)
                , Name(name)
            {
            }

            template <class OtherProps>
            TNamedVertexProps(TNamedVertexProps<N, OtherProps> const& props)
                : TVertexProps(static_cast<OtherProps const&>(props))
                , Name(props.Name)
            {
            }

            TVertexProps& Base() {
                return static_cast<TVertexProps&>(*this);
            }
            TVertexProps const& Base() const {
                return static_cast<TVertexProps const&>(*this);
            }

            void Deserialize(const NJson::TJsonValue& info) {
                Name = info["name_graph"].GetStringRobust();
                TVertexProps::Deserialize(info);
            }

            NJson::TJsonValue Serialize() const {
                NJson::TJsonValue result = TVertexProps::Serialize();
                result["name_graph"] = Name;
                return result;
            }

        public:
            N Name;
        };
    }

    // TODO - private inheritance is more suitable here
    template <class Name, class VertexProps = TNothing, class EdgeProps = TNothing>
    class TNamedGraph: public TGraph<NPrivate::TNamedVertexProps<Name, VertexProps>, EdgeProps> {
    public:
        template <class N, class VP, class EP>
        friend class TNamedGraph;

        typedef TGraph<NPrivate::TNamedVertexProps<Name, VertexProps>, EdgeProps> TBase;

        typedef typename TBase::TCount TCount;

        typedef typename TBase::TVertex TVertex;
        typedef typename TBase::TVertexId TVertexId;

        using TName = Name;
        typedef VertexProps TUserProps;
        typedef typename TBase::TVertexProps TVertexProps;
        typedef typename TBase::TEdgeProps TEdgeProps;

        typedef typename TBase::TEdgeIterator TEdgeIterator;

        typedef typename TBase::TTraits::template TMapT<Name, TVertexId>::T TNameToIdMapping;

        typedef typename TBase::TStringType TStringType;
        typedef typename TBase::IInputStream IInputStream;

        template <EDirection Direction>
        using TConstAdjacentIterator = typename TBase::template TConstAdjacentIterator<Direction>;

    public:
        TNamedGraph() = default;

        TNamedGraph(TBase const& base)
            : TBase(base)
        {
        }

        template <class OtherVertexProps, class OtherEdgeProps>
        TNamedGraph(TNamedGraph<Name, OtherVertexProps, OtherEdgeProps> const& other)
            : TBase(other)
            , Mapping(other.Mapping)
        {
        }

        TVertexId AddVertex(Name const& name) {
            Mapping[name] = TBase::NumVertices();
            return TBase::AddVertex(TVertexProps(name));
        }
        TVertexId AddVertex(Name const& name, TUserProps const& props) {
            Mapping[name] = TBase::NumVertices();
            return TBase::AddVertex(TVertexProps(name, props));
        }

        TEdgeIterator AddEdge(Name const& first, Name const& second) {
            return TBase::AddEdge(NameToIdAdd(first), NameToIdAdd(second));
        }
        TEdgeIterator AddEdge(Name const& first, Name const& second, TEdgeProps const& props) {
            return TBase::AddEdge(NameToIdAdd(first), NameToIdAdd(second), props);
        }

        TEdgeIterator AddEdgeById(const TVertexId first, const TVertexId second, TEdgeProps const& props) {
            return TBase::AddEdge(first, second, props);
        }

        TEdgeIterator AddEdgeByName(const Name& first, const Name& second, TEdgeProps const& props) {
            return TBase::AddEdge(NameToIdAdd(first), NameToIdAdd(second), props);
        }

        void ResetVertexProps(const TUserProps& props) {
            for (TCount i = 0; i < TBase::NumVertices(); i++)
                static_cast<TUserProps&>(Vertex(i).Props()) = props;
        }
        void ResetVertexProps() {
            ResetVertexProps(TUserProps());
        }
        void Clear() {
            TBase::Clear();
            Mapping.clear();
        }

        using TBase::Vertex;
        TVertex& Vertex(Name const& name) {
            return TBase::Vertex(NameToId(name));
        }
        TVertex const& Vertex(Name const& name) const {
            return TBase::Vertex(NameToId(name));
        }

        TVertex const& VertexById(const TVertexId id) const {
            return TBase::Vertex(id);
        }

        TVertex& VertexById(const TVertexId id) {
            return TBase::Vertex(id);
        }

        TVertex const& VertexByName(const TVertexId name) const {
            return TBase::Vertex(NameToId(name));
        }

        TVertex& VertexByName(const TVertexId name) {
            return TBase::Vertex(NameToId(name));
        }

        TVertexId NameToId(Name const& name) const {
            typename TNameToIdMapping::const_iterator it = Mapping.find(name);
            return it != Mapping.end() ? it->second : TBase::NULL_VERTEX();
        }

        Name IdToName(const TVertexId id) const {
            return TBase::Vertex(id).Name;
        }

        void DeserializeFromJson(const NJson::TJsonValue& in) {
            Y_ASSERT(Mapping.empty());
            TBase::DeserializeFromJson(in);
            for (TCount i = 0; i < TBase::NumVertices(); i++)
                Mapping[Vertex(i).Props().Name] = i;
        }

        void InitFromDescription(IInputStream& in) {
            Y_ASSERT(Mapping.empty());
            TBase::InitFromDescription(in);
            for (TCount i = 0; i < TBase::NumVertices(); i++)
                Mapping[Vertex(i).Props().Name] = i;
        }
        void InitFromDescription(TStringType const& string) {
            TStringInput in(string);
            InitFromDescription(in);
        }

        void InitFromEdgeList(IInputStream& in) {
            Y_ASSERT(TBase::NumVertices() == 0 && TBase::NumEdges() == 0);
            Name first, second;
            TEdgeProps props;
            while (true) {
                in >> first >> second >> props;
                if (first.empty() || second.empty())
                    return;
                AddEdge(first, second, props);
            }
        }
        void InitFromEdgeList(TStringType const& string) {
            TStringInput in(string);
            InitFromEdgeList(in);
        }

    private:
        typedef typename TBase::TTraits::TStringInputT TStringInput;

    private:
        TVertexId NameToIdAdd(Name const& name) {
            typename TNameToIdMapping::iterator it = Mapping.find(name);
            if (it == Mapping.end())
                return AddVertex(name);
            return it->second;
        }

    private:
        TNameToIdMapping Mapping;
    };

}

template <class Name, class Props>
IOutputStream& operator<<(IOutputStream& out,
                          NGraph::NPrivate::TNamedVertexProps<Name, Props> const& props) {
    out << props.Name << ' ' << props.Base();
    return out;
}

template <class Name, class Props>
IInputStream& operator>>(IInputStream& out,
                         NGraph::NPrivate::TNamedVertexProps<Name, Props>& props) {
    out >> props.Name >> props.Base();
    return out;
}
