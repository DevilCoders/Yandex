#pragma once

#include <yandex/maps/idl/customizable.h>
#include <yandex/maps/idl/nodes/doc.h>
#include <yandex/maps/idl/nodes/nodes.h>

#include <string>
#include <utility>
#include <vector>

namespace yandex {
namespace maps {
namespace idl {
namespace parser {

/**
 * Holds information about syntax errors found during .idl parsing.
 *
 * Adding an error is done in two stages:
 *   1. setting line number and message (done by Bison)
 *   2. setting context and adding error (done by us)
 *
 * It is this way because we can't do it as one method call: at first Bison
 * calls yyerror with error's line number and message. Then, only after it
 * finishes we find out error's context.
 */
class Errors {
public:
    void setLineNumberAndMessage(int lineNumber, const std::string& message);
    void addWithContext(const std::string& context);

    /**
     * If there was an error, but its context was never set, it will not be
     * added to errors_ vector. This method makes sure it does.
     */
    void handlePendingError();

    const std::vector<std::string>& errors() const;

private:
    int lineNumber_;
    std::string message_;

    std::vector<std::string> errors_;
};

/**
 * Copies the data from a heap-allocated srcPtr to the referenced object
 * destRef and then destroys the pointer. Destruction is necessary because the
 * object is not going to be needed any more, and copying is needed because of
 * a mismatch between parser's temporary and output structures. E.g.
 * "ProtoMessage" has "pathToProto" and "pathInProto" fields as values but,
 * initially, the parser creates them on the heap.
 */
template <typename Destination, typename Source>
void move(Destination& destRef, Source* srcPtr)
{
    if (srcPtr) {
        destRef = std::move(*srcPtr);
        delete srcPtr;
    }
}

/**
 * Adds new item. For details about the transfer of ownership see
 * move(Destination&, Source*) method.
 */
template <typename Item>
void pushMoved(std::vector<Item>& v, Item* ptr)
{
    if (ptr) {
        v.push_back(std::move(*ptr));
        delete ptr;
    }
}
void pushMoved(Scope& scope, std::string* ptr);

template <typename Key, typename Value>
void pushPair(std::vector<std::pair<Key, Value>>& v, Key* key, Value* value)
{
    if (key && value) {
        v.emplace_back(std::move(*key), std::move(*value));
    }
    delete key;
    delete value;
}

/**
 * Appends suffix to the base value using + operator. For details about the
 * transfer of ownership see move(Destination&, Source*) method.
 */
template <typename Value>
void appendMoved(Value& baseValue, Value* suffix)
{
    if (suffix) {
        baseValue += *suffix;
        delete suffix;
    }
}

/**
 * Adds new doc link to given doc block. For details about the transfer of
 * ownership see move(Destination&, Source*) method.
 */
void addDocLink(nodes::DocBlock& docBlock, nodes::DocLink* link);

/**
 * Adds new node. For details about the transfer of ownership see
 * move(Destination&, Source*) method.
 */
template <typename Node>
void pushNode(nodes::Nodes& nodes, Node* nodePtr)
{
    if (nodePtr) {
        nodes.add(std::move(*nodePtr));
        delete nodePtr;
    }
}

/**
 * Creates customizable value (name or namespace) on the heap, and deletes
 * passed parameters. For details about the transfer of ownership see
 * move(Destination&, Source*) method.
 */
template <typename Value>
CustomizableValue<Value>* createCustomizableValue(
    Value* original,
    std::vector<TargetSpecificValue<Value>>* targetSpecificValues = nullptr)
{
    CustomizableValue<Value>* value = nullptr;
    if (original) {
        if (targetSpecificValues) {
            value = new CustomizableValue<Value>(std::move(*original),
                std::move(*targetSpecificValues));
            delete targetSpecificValues;
        } else {
            value = new CustomizableValue<Value>(std::move(*original), { });
        }
        delete original;
    }
    return value;
}

void initializeTypeRefFromName(nodes::TypeRef& typeRef, Scope* nameParts);

} // namespace parser
} // namespace idl
} // namespace maps
} // namespace yandex
