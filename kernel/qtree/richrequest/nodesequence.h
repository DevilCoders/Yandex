#pragma once

#include "richnode_fwd.h"
#include "proxim.h"
#include "range.h"
#include "markup/markup.h"

#include <util/generic/noncopyable.h>
#include <util/generic/vector.h>

class TNodeSequence : TNonCopyable {
public:
    TNodeSequence() = default;

    bool operator==(const TNodeSequence& seq) const;

    // compatibility

    operator const TVector<TRichNodePtr>& () const {
        return Nodes;
    }

    explicit operator bool() const {
        return !empty();
    }

    const TRichNodePtr* data() const {
        return Nodes.data();
    }

    size_t size() const {
        return Nodes.size();
    }

    bool empty() const {
        return Nodes.empty();
    }

    const TRichNodePtr& back() const {
        return Nodes.back();
    }


    // const indexing

    const TRichNodePtr& operator[](size_t index) const {
        return Nodes[index];
    }

    const TProximity& ProxBefore(size_t index) const {
        return Proxes[index];
    }

    const TProximity& ProxAfter(size_t index) const {
        return Proxes[index + 1];
    }

    // mutable indexing

    TRichNodePtr& MutableNode(size_t index) {
        return Nodes[index];
    }

    TProximity& ProxBefore(size_t index) {
        return Proxes[index];
    }

    TProximity& ProxAfter(size_t index) {
        return Proxes[index + 1];
    }

    // iteration (only const)

    typedef TVector<TRichNodePtr>::const_iterator const_iterator;

    const_iterator begin() const {
        return Nodes.begin();
    }

    const_iterator end() const {
        return Nodes.end();
    }


    size_t FindPosition(const TRichRequestNode* node) const;



    // markup

    const NSearchQuery::TMarkup& Markup() const {
        return Markup_;
    }

    NSearchQuery::TMarkup& MutableMarkup() {
        return Markup_;
    }

    void AddMarkup(size_t beg, size_t end, NSearchQuery::TMarkupDataPtr data);          // @end is the last node of markup range, same as TRange::End

    void CopyTo(TNodeSequence& seq) const;

    // TODO: move to filter_tree
    bool IsMultitokenPart(size_t pos) const;

    bool VerifySize() const;

public: // modification

    void Clear();

    // obsolete
    void Remove(size_t start, size_t stop);         // if start <= stop then RemoveRight(start, stop); otherwise RemoveLeft(stop + 1, start + 1)

    void RemoveLeft(size_t start, size_t stop);     // remove Nodes[start:stop) with intermediate proxes + prox before Nodes[start]
    void RemoveRight(size_t start, size_t stop);    // remove Nodes[start:stop) with intermediate proxes + prox after Nodes[stop-1]
    void RemoveWeak(size_t start, size_t stop);      // remove Nodes[start:stop) with intermidiate proxes, leaving maximal distances for surrounding proxes

    // same for single node
    void RemoveLeft(size_t index) {
        RemoveLeft(index, index + 1);
    }
    void RemoveRight(size_t index) {
        RemoveRight(index, index + 1);
    }
    void RemoveWeak(size_t index) {
        RemoveWeak(index, index + 1);
    }

    void Insert(size_t pos, TRichNodePtr node, const TProximity& distance = TProximity());            // distance to preceding node
    void Insert(size_t pos, const TNodeSequence& nodes, const TProximity& distance = TProximity());    // distance to preceding node

    void Append(TRichNodePtr node, const TProximity& distance = TProximity()) {             // distance to preceding node
        Insert(Nodes.size(), node, distance);
    }
    void Append(const TNodeSequence& nodes, const TProximity& distance = TProximity()) {    // distance to preceding node
        Insert(Nodes.size(), nodes, distance);
    }

    // Arbitrary replacement, all intersecting markup is removed
    void Replace(size_t start, size_t stop, const TRichNodePtr& node);
    void Replace(size_t start, size_t stop, const TNodeSequence& nodes);

    // Use this method in case of more or less _equivalent_ substitutions
    // when replacement does not break borders of existing markup.
    // (e.g. a word is replaced with its translation and you want to preserve all extensions assigned earlier to this word)
    // Note that ambiguous markup (partially intersecting [start,stop) range) will be removed anyway.
    void EquivalentReplace(size_t start, size_t stop, const TRichNodePtr& node);
    void EquivalentReplace(size_t start, size_t stop, const TNodeSequence& node);

    // simple raw replace, all markup is preserved
    void ReplaceSingle(size_t index, TRichNodePtr node) {
        Nodes[index] = node;
    }

    // Replace nodes from range [start, stop) with new single node, replaced nodes are moved lower as children of the new node.
    // Preserve intersecting markup if it is unambiguos (i.e. includes whole range of moved node).
    void Group(size_t start, size_t stop);
    // Same but uses specified node instead of creating new node (takes ownership)
    void Group(size_t start, size_t stop, TRichNodePtr groupNode);


    // Single level flattening: replace @index'th node with its own children (deleting the node itself)
    // Do nothing if the node does not have any children.
    // Return number of inserted children (i.e. Children_new_size - Children_old_size)
    size_t FlattenOneLevel(size_t index);

    // Complete flattening of single node: replace @index'th node with sequence of all its leaves (lowest sub-nodes)
    size_t Flatten(size_t index);

    // Flatten all nodes
    void Flatten();


    // Proxes
    void ResetDistances(int beg, int end, WORDPOS_LEVEL lev, TDistanceType dist);
    void ResetDistances(const TProximity& distance);


    void Swap(TNodeSequence& seq) {
        ::DoSwap(Nodes, seq.Nodes);
        ::DoSwap(Proxes, seq.Proxes);
        ::DoSwap(Markup_, seq.Markup_);
    }

private:
    void RemoveImpl(size_t start, size_t stop, size_t proxShift);

    void AddMarkupImpl(size_t beg, size_t end, NSearchQuery::TMarkupDataPtr data);

    // add all markup data from @markup to Markup_, shifted by @shift
    template <bool clone>
    void CopyMarkupFrom(const NSearchQuery::TMarkup& markup, int shift);

    NSearchQuery::TRange CopyRange(const NSearchQuery::TRange& src, int shift = 0) const;


private:
    TVector<TRichNodePtr> Nodes;

    TVector<TProximity> Proxes;     // i-th Proxes is proximity between Nodes[i-1] and Nodes[i]
                                    // Proxes[0], Proxes[Nodes.size()] are default (fake).

    NSearchQuery::TMarkup Markup_;
};

