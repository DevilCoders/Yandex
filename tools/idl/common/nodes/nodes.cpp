#include <yandex/maps/idl/nodes/nodes.h>

namespace yandex {
namespace maps {
namespace idl {
namespace nodes {

void Nodes::traverse(Visitor* visitor) const
{
    for (const auto& node : vector_) {
        node->traverse(visitor);
    }
}

namespace {

class NodeCounter : public Visitor {
public:
    /**
     * Counts the number of nodes in given Nodes object.
     */
    std::size_t count(const Nodes& nodes)
    {
        count_ = 0;
        nodes.traverse(this);
        return count_;
    }

private:
    std::size_t count_;

    void onVisited(const Enum&) override { ++count_; }
    void onVisited(const Interface&) override { ++count_; }
    void onVisited(const Function&) override { ++count_; }
    void onVisited(const Listener&) override { ++count_; }
    void onVisited(const Property&) override { ++count_; }
    void onVisited(const Struct&) override { ++count_; }
    void onVisited(const StructField&) override { ++count_; }
    void onVisited(const Variant&) override { ++count_; }
};

} // namespace

std::size_t Nodes::count() const
{
    return NodeCounter().count(*this);
}

} // namespace nodes
} // namespace idl
} // namespace maps
} // namespace yandex
