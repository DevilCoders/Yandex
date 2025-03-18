#pragma once

#include <yandex/maps/idl/env.h>
#include <yandex/maps/idl/idl.h>
#include <yandex/maps/idl/nodes/doc.h>
#include <yandex/maps/idl/nodes/function.h>
#include <yandex/maps/idl/nodes/interface.h>
#include <yandex/maps/idl/nodes/struct.h>
#include <yandex/maps/idl/scope.h>

#include <ctemplate/template.h>

#include <optional>
#include <string>
#include <vector>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace common {

class EnumFieldMaker;
class FunctionMaker;
class ImportMaker;
class TypeNameMaker;

void addDocParagraph(
    std::vector<std::string>& generatedDoc,
    const std::string& paragraph);

class DocMaker {
public:
    DocMaker(
        const std::string& tplDir,
        const TypeNameMaker* typeNameMaker,
        ImportMaker* importMaker,
        const EnumFieldMaker* enumFieldMaker,
        FunctionMaker* functionMaker);

    virtual ~DocMaker();

    /**
     * Generates and inserts the documentation.
     *
     * Doc link tags can refer to types by their partially qualified names, so
     * to find those types we need the scope where documentation is written.
     */
    virtual void make(
        ctemplate::TemplateDictionary* dict,
        const std::string& tagName,
        const FullScope& scope,
        const std::optional<nodes::Doc>& doc) const;

    /**
     * Same as above, but for given field.
     */
    virtual void make(
        ctemplate::TemplateDictionary* dict,
        const std::string& tagName,
        const FullScope& scope,
        const nodes::StructField& field) const;

    /**
     * Same as above, but for given funcition.
     */
    virtual void make(
        ctemplate::TemplateDictionary* dict,
        const std::string& tagName,
        const FullScope& scope,
        const nodes::Function& function) const;

    /**
     * Same as above, but for interface property.
     */
    virtual void make(
        ctemplate::TemplateDictionary* dict,
        const std::string& tagName,
        const FullScope& scope,
        const nodes::Property& property) const;

protected:
    /**
     * Following methods convert documentation part-by-part into its target
     * format.
     */
    virtual std::vector<std::string> convertDoc(
        const FullScope& scope,
        const nodes::Doc& doc) const;
    virtual std::string convertDocBlock(
        const FullScope& scope,
        const nodes::DocBlock& docBlock) const;
    virtual std::string convertDocLink(
        const FullScope& scope,
        const nodes::DocLink& link) const;

    virtual std::string optionalFieldAnnotation() const;
    virtual std::string optionalPropertyAnnotation() const;
    virtual std::string docExclusionAnnotation() const;
    virtual std::string emptyDocAnnotation() const;

    template<typename Node>
    void make(
        ctemplate::TemplateDictionary* dict,
        const std::string& tagName,
        const FullScope& scope,
        const Node& node,
        const std::optional<std::string>& optionalAnnotation) const
    {
        std::vector<std::string> generatedDoc;

        if (docNode(node)) {
            if (isExcludeDoc(docNode(node)))
                generatedDoc.emplace_back(docExclusionAnnotation());

            auto convertedDoc = convertDoc(scope, docSource(node));
            generatedDoc.reserve(generatedDoc.size() + convertedDoc.size());
            std::move(convertedDoc.begin(), convertedDoc.end(), std::back_inserter(generatedDoc));
        }

        if (optionalAnnotation && isOptional(node)) {
            addDocParagraph(generatedDoc, *optionalAnnotation);
        }

        if constexpr (std::is_same_v<Node, nodes::Function>) {
            switch (node.threadRestriction) {
                case nodes::Function::ThreadRestriction::Ui:
                    // No documentation needed - it is a default, and would be added to most methods
                    break;
                case nodes::Function::ThreadRestriction::Bg:
                    addDocParagraph(
                        generatedDoc, "This method will be called on a background thread.");
                    break;
                case nodes::Function::ThreadRestriction::None:
                    addDocParagraph(
                        generatedDoc,
                        "This method may be called on any thread. "
                            "Its implementation must be thread-safe.");
                    break;
            }
        }

        if (generatedDoc.empty() && emptyDocAnnotation().size()) {
            generatedDoc.emplace_back(emptyDocAnnotation());
        }
        insertDoc(dict, tagName, generatedDoc);
    }

    /**
     * Same as methods above, but for function documentation.
     */
    virtual std::vector<std::string> convertDoc(
        const FullScope& scope,
        const nodes::Function& function) const;

    /**
     * Inserts generated documentation in given position.
     */
    void insertDoc(
        ctemplate::TemplateDictionary* dict,
        const std::string& tagName,
        std::vector<std::string> generatedDoc) const;

    const std::string tplDir_;

    const TypeNameMaker* typeNameMaker_;
    ImportMaker* importMaker_;

    const EnumFieldMaker* enumFieldMaker_;
    FunctionMaker* functionMaker_;

private:
    template<typename T>
    static const std::optional<nodes::Doc>& docNode(const T& node)
    {
        if constexpr (std::is_same_v<T, std::optional<nodes::Doc>>)
            return node;
        else
            return node.doc;
    }

    template <typename T>
    static const auto& docSource(const T& node)
    {
        if constexpr (std::is_same_v<T, nodes::Function>)
            return node;
        else
            return *docNode(node);
    }

    template <typename T>
    static bool isOptional(const T& node)
    {
        if constexpr (
                std::is_same_v<T, nodes::Function> ||
                std::is_same_v<T, std::optional<nodes::Doc>>)

            return false;
        else
            return node.typeRef.isOptional;
    }

    std::string commonConvertDoc(
        const FullScope& scope,
        const nodes::Doc& doc) const;
};

} // namespace common
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
