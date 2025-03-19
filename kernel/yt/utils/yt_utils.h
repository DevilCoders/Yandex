#pragma once

#include <mapreduce/yt/interface/client.h>

#include <util/generic/string.h>
#include <util/string/builder.h>
#include <util/string/split.h>
#include <util/string/strip.h>

namespace NJupiter {
    namespace NYtJoinDetail {

        inline TString JoinYtPathImpl(TString head) {
            return TStringBuilder() << head;
        }

        template <typename ... T>
        TString JoinYtPathImpl(TString head, T ... tail) {
            return TStringBuilder() << StripString(head, EqualsStripAdapter('/')) << "/" << JoinYtPathImpl(tail...);
        }

        template <typename ... T>
        TString JoinYtPathImplCutRight(TString head, T ... tail) {
            if (head == "//") {
                return TStringBuilder() << head << JoinYtPathImpl(tail...);
            }
            return TStringBuilder() << StripStringRight(head, EqualsStripAdapter('/')) << "/" << JoinYtPathImpl(tail...);
        }

    } // namespace NYtJoinDetail.

    template <typename ... T>
    TString JoinYtPath(TString head, T ... tail) {
        return TStringBuilder() << NYtJoinDetail::JoinYtPathImplCutRight(head, tail...);
    }

    TString JoinYtMeta(TString path, TString meta);

    TString GetYtPathName(const TString& path);
    TString GetYtPathDirname(const TString& path);

    // Returns nested node. Multiple nesting levels are supported.
    const NYT::TNode& GetYtNode(const NYT::TNode& node, const TString& relativePath);
    // Returns attr, supports nested paths.
    const NYT::TNode& GetYtNodeAttr(const NYT::TNode& node, const TString& attrName);
    NYT::TNode GetYtAttr(NYT::IClientBasePtr client, const TString& path, const TString& attrName);
    TMaybe<NYT::TNode> GetYtAttrMaybe(NYT::IClientBasePtr client, const TString& path, const TString& attrName);
    bool HasYtAttr(NYT::IClientBasePtr client, const TString& path, const TString& attrName);
    void SetYtAttr(NYT::IClientBasePtr client, const TString& path, const TString& attrName, const NYT::TNode& value);
    TString GetClusterName(NYT::IClientBasePtr client);
    void PreCreateAttrPath(NYT::IClientBasePtr client, const TString& path, const TString& metaPath);

    /** \brief Returns a typed attribute (if present) or the default value (otherwise)
     *
     *  \tparam T           The attribute type
     *  \param client       The YT client interface
     *  \param path         The path to the node with the attribute
     *  \param attrName     The attribute name
     *  \param defaultValue The default value
     *  \return The attribute value or default
     */
    template<typename T>
    T GetYtAttributeOrDefault(NYT::IClientBasePtr client, const TString& path, const TString& attrName
        , T const& defaultValue = T())
    {
        auto const fullPath = JoinYtMeta(path, attrName);
        if (!client->Exists(fullPath)) {
            return defaultValue;
        }
        auto const& node = client->Get(fullPath);
        Y_ENSURE(node.IsOfType<T>(), "Unexpected attribute type");
        return node.As<T>();
    }

    /** \brief Gets a typed node attribute
     *
     *  \tparam T   The attribute type
     *  \param node The node to get the attribute from
     *  \param name The attribute name
     *  \return The attribute value
     */
    template<typename T>
    T const& GetNodeAttribute(NYT::TNode const& node, TString const& name) {
        auto const& attributeNode = node.GetAttributes()[name];
        Y_ENSURE(attributeNode.IsOfType<T>(), "Unexpected attribute type");
        return attributeNode.As<T>();
    }
    /** \brief Returns a typed node attribute (if present) or the default value (otherwise)
     *
     *  \tparam T           The attribute type
     *  \param node         The node to get the attribute from
     *  \param name         The attribute name
     *  \param defaultValue The default value
     *  \return The attribute value
     */
    template<typename T>
    T const& GetNodeAttributeOrDefault(NYT::TNode const& node, TString const& name, T const& defaultValue = T()) {
        auto const& attributeNode = node.GetAttributes()[name];
        if (attributeNode.IsUndefined()) {
            return defaultValue;
        }
        Y_ENSURE(attributeNode.IsOfType<T>(), "Unexpected attribute type");
        return attributeNode.As<T>();
    }

    TMaybe<NYT::TNode> GetAttributeMaybe(const NYT::TNode& node, const TString& attributePath);

    template <typename T>
    TMaybe<T> GetAttributeMaybe(const NYT::TNode& node, const TString& attributePath) {
        const auto& attributeNode = GetAttributeMaybe(node, attributePath);
        if (attributeNode) {
            Y_ENSURE(attributeNode->IsOfType<T>());
            return attributeNode->As<T>();
        }

        return {};
    }

    /** \brief Gets a timestamp node attribute
     *
     *  \param node The node to get the attribute from
     *  \param name The attribute name
     *  \return The attribute value
     */
    TInstant GetNodeTimestampAttribute(NYT::TNode const& node, TString const& name);

    NYT::ENodeType GetNodeType(NYT::IClientBasePtr client, const TString& path);

    bool IsYtLink(NYT::IClientBasePtr client, const TString& path);

    /*
     * Creates waitable lock and wait until it's aquired.
     * Returns true when lock is acquired. Return false when time is out.
     * This nice function was taken from robot/lemur/algo/ytlib/yt_utils.cpp
     * TODO: when yt_utils are merged into some common library, leave only one version.
     * The difference from lemur version is that it takes canonical YT Path (FixYTPath is not called)
     */
    bool WaitAndLock(NYT::ITransactionPtr transaction, const TString& path, NYT::ELockMode lockMode, TDuration timeout, NYT::TLockOptions lockOptions = NYT::TLockOptions());

    void UpdateNode(NYT::TNode& baseNode, const NYT::TNode& updateNode);
} // namespace NJupiter.
