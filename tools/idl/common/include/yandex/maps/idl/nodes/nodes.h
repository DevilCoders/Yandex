#pragma once

#include <yandex/maps/idl/utils/common.h>
#include <yandex/maps/idl/utils/exception.h>
#include <yandex/maps/idl/utils/make_unique.h>

#include <functional>
#include <memory>
#include <utility>
#include <vector>

namespace yandex {
namespace maps {
namespace idl {
namespace nodes {

struct Enum;
struct Interface;
struct Function;
struct Listener;
struct Property;
struct Struct;
struct StructField;
struct Variant;

/**
 * Visitor's subclasses are used to iterate over the "Nodes" object.
 */
class Visitor {
public:
    virtual ~Visitor() { }

    virtual void onVisited(const Enum&) { }
    virtual void onVisited(const Interface&) { }
    virtual void onVisited(const Function&) { }
    virtual void onVisited(const Listener&) { }
    virtual void onVisited(const Property&) { }
    virtual void onVisited(const Struct&) { }
    virtual void onVisited(const StructField&) { }
    virtual void onVisited(const Variant&) { }
};

/**
 * Visits all nodes and fails on all of them - override onVisited functions
 * for those nodes that can occur during the iteration. Useful mostly in tests.
 */
class StrictVisitor : public Visitor {
protected:
    virtual void onVisited(const Enum&) override
    {
        handleUnexpectedNode();
    }
    virtual void onVisited(const Interface&) override
    {
        handleUnexpectedNode();
    }
    virtual void onVisited(const Function&) override
    {
        handleUnexpectedNode();
    }
    virtual void onVisited(const Listener&) override
    {
        handleUnexpectedNode();
    }
    virtual void onVisited(const Property&) override
    {
        handleUnexpectedNode();
    }
    virtual void onVisited(const Struct&) override
    {
        handleUnexpectedNode();
    }
    virtual void onVisited(const StructField&) override
    {
        handleUnexpectedNode();
    }
    virtual void onVisited(const Variant&) override
    {
        handleUnexpectedNode();
    }

    virtual void handleUnexpectedNode() const
    {
        INTERNAL_ERROR("Strict visitor bumped into an unexpected node");
    }
};

namespace internal {

template <typename FirstLambda, typename ...RemainingLambdas>
class MultiLambdaVisitor : public MultiLambdaVisitor<RemainingLambdas...> {
public:
    MultiLambdaVisitor(FirstLambda firstLambda, RemainingLambdas... remainingLambdas)
        : MultiLambdaVisitor<RemainingLambdas...>(remainingLambdas...),
          firstLambda_(firstLambda)
    {
    }
    virtual ~MultiLambdaVisitor() { }

protected:
    using MultiLambdaVisitor<RemainingLambdas...>::onVisited;
    using Node = typename utils::LambdaTraits<FirstLambda>::FirstArgument;
    virtual void onVisited(const Node& n) override { firstLambda_(n); }

private:
    FirstLambda firstLambda_;
};

template <typename Lambda>
class MultiLambdaVisitor<Lambda> : public Visitor {
public:
    MultiLambdaVisitor(Lambda lambda) : lambda_(lambda) { }

protected:
    using Visitor::onVisited;
    using Node = typename utils::LambdaTraits<Lambda>::FirstArgument;
    virtual void onVisited(const Node& n) override { lambda_(n); }

private:
    Lambda lambda_;
};

} // namespace internal

/**
 * Contains a collection of nodes.
 */
class Nodes {
public:
    Nodes() = default;
    Nodes(Nodes&&) = default;
    Nodes& operator=(Nodes&&) = default;

    template <typename FirstNode, typename ...RemainingNodes>
    Nodes(FirstNode&& firstNode, RemainingNodes&&... remainingNodes)
    {
        addAll(std::move(firstNode), std::move(remainingNodes)...);
    }

    void addAll() { } // Base case (must be defined for main case to compile)
    template <typename FirstNode, typename ...RemainingNodes>
    void addAll(FirstNode&& firstNode, RemainingNodes&&... remainingNodes)
    {
        add(std::move(firstNode));
        addAll(std::move(remainingNodes)...);
    }

    template <typename T>
    void add(T&& node)
    {
        vector_.push_back(utils::make_unique<Wrapper<T>>(std::move(node)));
    }

    void traverse(Visitor* visitor) const;
    void traverse(Visitor&& visitor) const // For inline traversal
    {
        traverse(&visitor);
    }

    template <typename ...Lambdas>
    void lambdaTraverse(Lambdas... lambdas) const
    {
        traverse(internal::MultiLambdaVisitor<Lambdas...>(lambdas...));
    }

    /**
     * Count the number of all child nodes.
     */
    std::size_t count() const;

    /**
     * Count child nodes of given type.
     */
    template <typename T>
    std::size_t count() const
    {
        std::size_t count = 0;
        lambdaTraverse([&count] (const T&) { ++count; });
        return count;
    }

    /**
     * Count child nodes of given type, for which given predicate returns true.
     */
    template <typename T, typename Predicate>
    std::size_t count(Predicate p) const
    {
        std::size_t count = 0;
        lambdaTraverse([&count, &p] (const T& node) { if (p(node)) ++count; });
        return count;
    }

private:
    /**
     * An iterable node.
     */
    class Node {
    public:
        virtual ~Node() { }

        /**
         * Overridden in derived classes to call appropriate "onVisited"
         * function in given visitor.
         */
        virtual void traverse(Visitor* visitor) const = 0;
    };

    /**
     * Wraps an item to make it "iterable".
     */
    template <typename T>
    class Wrapper : public Node {
    public:
        explicit Wrapper(T&& item) noexcept : item_(std::move(item)) { }

        virtual ~Wrapper() { }

        /**
         * See description of "Node::traverse(Visitor*)".
         */
        virtual void traverse(Visitor* visitor) const override
        {
            visitor->onVisited(item_);
        }

    private:
        T item_;
    };

    std::vector<std::unique_ptr<Node>> vector_;
};

} // namespace nodes
} // namespace idl
} // namespace maps
} // namespace yandex
