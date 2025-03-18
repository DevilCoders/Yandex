#include "common/doc_maker.h"

#include "common/common.h"
#include "common/enum_field_maker.h"
#include "common/function_maker.h"
#include "common/import_maker.h"
#include "common/type_name_maker.h"
#include "tpl/tpl.h"

#include <yandex/maps/idl/functions.h>
#include <yandex/maps/idl/utils.h>
#include <yandex/maps/idl/nodes/enum.h>
#include <yandex/maps/idl/nodes/function.h>

#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <boost/regex.hpp>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace common {

void addDocParagraph(
    std::vector<std::string>& doc,
    const std::string& paragraph)
{
    if (!doc.empty()) {
        doc.push_back("");
    }

    doc.push_back(paragraph);
}

DocMaker::DocMaker(
    const std::string& tplDir,
    const TypeNameMaker* typeNameMaker,
    ImportMaker* importMaker,
    const EnumFieldMaker* enumFieldMaker,
    FunctionMaker* functionMaker)
    : tplDir_(tplDir),
      typeNameMaker_(typeNameMaker),
      importMaker_(importMaker),
      enumFieldMaker_(enumFieldMaker),
      functionMaker_(functionMaker)
{
}

DocMaker::~DocMaker()
{
}

std::string DocMaker::optionalFieldAnnotation() const
{
    return "";
}
std::string DocMaker::optionalPropertyAnnotation() const
{
    return "";
}

std::string DocMaker::docExclusionAnnotation() const
{
    return "";
}

std::string DocMaker::emptyDocAnnotation() const
{
    return "";
}

void DocMaker::make(
    ctemplate::TemplateDictionary* parentDict,
    const std::string& tagName,
    const FullScope& scope,
    const std::optional<nodes::Doc>& doc) const
{
    make(parentDict, tagName, scope, doc, std::nullopt);
}

void DocMaker::make(
    ctemplate::TemplateDictionary* parentDict,
    const std::string& tagName,
    const FullScope& scope,
    const nodes::StructField& field) const
{
    make(parentDict, tagName, scope, field, optionalFieldAnnotation());
}

void DocMaker::make(
    ctemplate::TemplateDictionary* parentDict,
    const std::string& tagName,
    const FullScope& scope,
    const nodes::Function& function) const
{
    if (!needGenerateNode(scope, function))
        return;
    make(parentDict, tagName, scope, function, std::nullopt);
}

void DocMaker::make(
    ctemplate::TemplateDictionary* parentDict,
    const std::string& tagName,
    const FullScope& scope,
    const nodes::Property& property) const
{
    make(parentDict, tagName, scope, property, optionalPropertyAnnotation());
}

namespace {

/**
 * Removes decorations, so that only important content remains.
 */
std::string undecorate(const std::string& comment)
{
    // Remove spaces (except for new-line), slashes and stars at line starts
    std::string value = boost::regex_replace(comment,
        boost::regex("^([^\\S\\n]*[/*]*)+"), "");

    value = boost::regex_replace(value, boost::regex("[/*]+\\z"), "");

    // Remove spaces at line ends
    value = boost::regex_replace(value, boost::regex("[^\\S\\n]+$"), "");

    // Unwrap the lines: single new-line symbol denotes the line wrap
    value = boost::regex_replace(value,
        boost::regex("(?<!\\n)\\n(?!\\n)"), " ");

    // Replace multiple \n-s with \n\n (\n\n separates paragraphs)
    value = boost::regex_replace(value, boost::regex("\\n{3,}"), "\\n\\n");

    return boost::algorithm::trim_copy(value);
}

/**
 * Wraps long lines in given text and returns them as vector. By default, max
 * line length is 75, because every line in our generated documentation starts
 * with " * " (which is 3 chars), and lines are limited by 78 chars.
 */
std::vector<std::string> wrap(
    const std::string& text,
    std::size_t maxLineLength = 69)
{
    std::vector<std::string> result;

    // There are two types of indivisible tokens: "whitespace character" and
    // "non-whitespace string".
    boost::sregex_iterator iterator(
        text.begin(), text.end(), boost::regex("(\\s)|(\\S+)"));
    boost::sregex_iterator end;

    std::string line;
    for (; iterator != end; ++iterator) { // for each indivisible token
        const std::string& token = (*iterator)[0];

        if (token == (*iterator)[1]) { // whitespace character
            if (line.size() == 0 && token != "\n") {
                // ignore whitespace other then \n at line start
            } else {
                if (token == "\n") {
                    result.push_back(boost::algorithm::trim_right_copy(line));
                    line.clear();
                } else {
                    line += token;
                }
            }
        } else { // not-whitespace string
            if (line.size() + token.size() > maxLineLength) {
                result.push_back(boost::algorithm::trim_right_copy(line));
                line.clear();
            }
            line += token;
        }
    }

    // Last line
    boost::algorithm::trim_right(line);
    if (!line.empty()) {
        result.push_back(line);
    }

    return result;
}

} // namespace

std::vector<std::string> DocMaker::convertDoc(
    const FullScope& scope,
    const nodes::Doc& doc) const
{
    return wrap(commonConvertDoc(scope, doc));
}

std::string DocMaker::convertDocBlock(
    const FullScope& scope,
    const nodes::DocBlock& docBlock) const
{
    boost::format format(undecorate(docBlock.format));
    for (const auto& link : docBlock.links) {
        format % convertDocLink(scope, link);
    }
    return format.str();
}

namespace {

/**
 * Creates a copy of the real function, suitable for link generation.
 *
 * Real function, found with nodes::findFunction(...), may be declared in
 * namespace and scope, completely different from those where doc link is
 * declared. This creates a problem, because link makers will not find
 * function's parameter types.
 */
nodes::Function functionForLink(
    const nodes::Function& realFunction,
    const std::vector<nodes::TypeRef>& linkParameterTypeRefs)
{
    auto functionForLink = realFunction;

    // Return value is not important, and may not be found because of a scope
    // change, so we simply make it void:
    functionForLink.result.typeRef =
        { nodes::TypeId::Void, std::nullopt, false, false, { } };

    // Parameters can simply be replaced:
    for (std::size_t i = 0; i < linkParameterTypeRefs.size(); ++i) {
        functionForLink.parameters[i].typeRef = linkParameterTypeRefs[i];
    }

    return functionForLink;
}

} // namespace

std::string DocMaker::convertDocLink(
    const FullScope& scope,
    const nodes::DocLink& link) const
{
    tpl::DirGuard guard(tplDir_);

    ctemplate::TemplateDictionary dict("DOC_LINK");

    if (!link.scope.isEmpty()) {
        FullTypeRef linkTypeRef(scope, link.scope);
        dict.SetValue("SCOPE", typeNameMaker_->make(linkTypeRef));
    }

    if (!link.memberName.empty()) {
        dict.ShowSection("HAS_MEMBER");
        if (!link.scope.isEmpty()) {
            dict.ShowSection("HAS_SCOPE_AND_MEMBER");
        }

        // Member name is not empty, so link scope is a custom type
        FullTypeRef linkTypeRef(scope, link.scope);
        if (link.parameterTypeRefs) {
            dict.ShowSection("NOT_ENUM_CONSTANT");

            auto realFunction = findFunction(scope, link.memberName,
                *link.parameterTypeRefs, linkTypeRef.info());
            auto function =
                functionForLink(*realFunction, *link.parameterTypeRefs);

            auto isLambda = linkTypeRef.isLambdaListener();

            functionMaker_->make(&dict, scope, function, isLambda);
        } else {
            std::string name = link.memberName;
            if (linkTypeRef.is<nodes::Enum>()) {
                name = enumFieldMaker_->makeReference(
                    linkTypeRef, link.memberName);
                dict.SetValue("ENUM_CONSTANT", name);
            } else {
                dict.ShowSection("NOT_ENUM_CONSTANT");
            }
            dict.SetValue("NON_FUNCTION_MEMBER_NAME", name);
        }
    } else {
        dict.ShowSection("NOT_ENUM_CONSTANT");
    }

    return tpl::expand("link.tpl", &dict);
}

std::vector<std::string> DocMaker::convertDoc(
    const FullScope& scope,
    const nodes::Function& function) const
{
    auto docText = commonConvertDoc(scope, *function.doc);

    bool hasOptionalParam = false;
    std::string optionalRemark = "\n\nRemark:";
    for (const auto& param : function.parameters) {
        if (param.typeRef.isOptional) {
            optionalRemark += "\n@param " + param.name +
                " has optional type, it may be uninitialized.";
            hasOptionalParam = true;
        }
    }
    if (hasOptionalParam) {
        docText += optionalRemark;
    }

    return wrap(docText);
}

void DocMaker::insertDoc(
    ctemplate::TemplateDictionary* parentDict,
    const std::string& tagName,
    std::vector<std::string> generatedDoc) const
{
    if (!generatedDoc.empty()) {
        parentDict->ShowSection("HAS_DOCS");

        auto dict = tpl::addInclude(
            parentDict, tagName, "/common/documentation.tpl");
        for (const auto& line : generatedDoc) {
            dict->AddSectionDictionary("LINE")->SetValue(
                "CONTENT", line.empty() ? "" : (' ' + line));
        }
    }
}

std::string DocMaker::commonConvertDoc(
    const FullScope& scope,
    const nodes::Doc& doc) const
{
    // Commercial note
    std::string commercialNote;
    if (doc.status == nodes::Doc::Status::Commercial) {
        commercialNote = "@attention This feature is not available in the free MapKit version.\n";
    }

    // Description
    std::string docText = convertDocBlock(scope, doc.description);
    docText = commercialNote + (docText.empty() || commercialNote.empty() ? "" : "\n\n") + docText;

    // Parameters
    std::string paramsText;
    for (const auto& param : doc.parameters) {
        paramsText += "\n@param " + param.first + " " +
            convertDocBlock(scope, param.second);
    }
    docText += docText.empty() || paramsText.empty() ? "" : "\n";
    docText += paramsText;

    // Return value
    std::string resultText = convertDocBlock(scope, doc.result);
    docText += docText.empty() || resultText.empty() ? "" : "\n\n";
    docText += resultText.empty() ? "" : ("@return " + resultText);

    return docText;
}

} // namespace common
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
