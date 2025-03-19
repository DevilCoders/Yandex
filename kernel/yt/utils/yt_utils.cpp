#include "yt_utils.h"

#include <kernel/yt/logging/log.h>
#include <kernel/yt/attrs/attrs.h>

#include <util/datetime/base.h>
#include <util/folder/path.h>
#include <util/string/vector.h>
#include <util/string/printf.h>
#include <util/string/split.h>

namespace NJupiter {

    TString JoinYtMeta(TString path, TString meta) {
        return StripStringRight(path, EqualsStripAdapter('/')) + "/@" + meta;
    }

    TString GetYtPathName(const TString& path) {
        return TFsPath(path).Basename(); // Isn't very good (because YT path starts with '//') but works
    }

    TString GetYtPathDirname(const TString& path) {
        return "/" + TFsPath(path).Dirname(); // Is ugly (because YT path starts with '//') but works
    }

    const NYT::TNode& GetYtNode(const NYT::TNode& node, const TString& relativePath) {
        TVector<TString> parts;
        StringSplitter(relativePath).Split('/').SkipEmpty().Collect(&parts);
        Y_VERIFY(parts.size() > 0, "There must be at least 1 part");

        const NYT::TNode * attrNode = &node;
        for (auto it = parts.begin(); it != parts.end(); ++it) {
            Y_ENSURE(attrNode->IsMap() && attrNode->HasKey(*it), TStringBuilder() << "There is no " << *it << " sub attribute");
            attrNode = &attrNode->AsMap().at(*it);
        }
        return *attrNode;
    }

    const NYT::TNode& GetYtNodeAttr(const NYT::TNode& node, const TString& attrPath) {
        TVector<TString> parts;
        StringSplitter(attrPath).Split('/').SkipEmpty().Limit(2).Collect(&parts);

        Y_VERIFY(parts.size() > 0, "There must be at least 1 part");
        Y_ENSURE(node.HasAttributes() && node.GetAttributes().HasKey(parts[0]), TStringBuilder() << "Node doesn't have " << parts[0] << " attribute");
        const auto& attrNode = node.GetAttributes()[parts[0]];
        return parts.size() == 1 ? attrNode : GetYtNode(attrNode, parts[1]);
    }

    NYT::TNode GetYtAttr(NYT::IClientBasePtr client, const TString& path, const TString& attrName) {
        return client->Get(JoinYtMeta(path, attrName));
    }

    TMaybe<NYT::TNode> GetYtAttrMaybe(NYT::IClientBasePtr client, const TString& path, const TString& attrName) {
        Y_ENSURE(client->Exists(path), "Path " << path << " doesn't exist");
        try {
            return TMaybe<NYT::TNode>(client->Get(JoinYtMeta(path, attrName)));
        } catch (const yexception& e) {
            return Nothing();
        }
    }

    bool HasYtAttr(NYT::IClientBasePtr client, const TString& path, const TString& attrName) {
        return client->Exists(JoinYtMeta(path, attrName));
    }

    void SetYtAttr(NYT::IClientBasePtr client, const TString& path, const TString& attrName, const NYT::TNode& value) {
        client->Set(JoinYtMeta(path, attrName), value);
    }

    TString GetClusterName(NYT::IClientBasePtr client) {
        return GetYtAttr(client, "//sys", "cluster_name").AsString();
    }

    void PreCreateAttrPath(NYT::IClientBasePtr client, const TString& path, const TString& metaPath) {
        if (client->Exists(JoinYtMeta(path, metaPath))) {
            return;
        } else {
            auto txClientPtr = client->StartTransaction(
                    NYT::TStartTransactionOptions().Title("PreCreateAttrPath " + path + ", " + metaPath)
            );
            TVector<TString> metaNodes = SplitString(metaPath, "/");
            try {
                WaitAndLock(
                    txClientPtr,
                    path,
                    NYT::ELockMode::LM_SHARED,
                    TDuration::Minutes(5),
                    NYT::TLockOptions().AttributeKey(metaNodes[0]));
            } catch (yexception& e) {
                txClientPtr->Abort();
                throw;
            }
            TString intermediatePath;

            for (TString metaNodeName : metaNodes) {
                if (intermediatePath.empty()) {
                    intermediatePath = JoinYtMeta(path, metaNodeName);
                } else {
                    intermediatePath = JoinYtPath(intermediatePath, metaNodeName);
                }

                if (!client->Exists(intermediatePath)) {
                        txClientPtr->Set(intermediatePath, NYT::TNode::CreateMap());
                }
            }
            txClientPtr->Commit();
        }
    }

    TMaybe<NYT::TNode> GetAttributeMaybe(const NYT::TNode& node, const TString& attributePath) {
        const auto splitted = StringSplitter(attributePath).Split('/').ToList<TString>();
        if (!(splitted.begin() != splitted.end()
                && node.HasAttributes()
                && node.GetAttributes().HasKey(*splitted.begin())))
        {
            return {};
        }
        auto* current = &node.GetAttributes()[*splitted.begin()];
        for (auto token = std::next(splitted.begin()); token != splitted.end(); ++token) {
            if (!(current->IsMap() && current->HasKey(*token))) {
                return {};
            }
            current = &(*current)[*token];
        }
        return *current;
    }

    TInstant GetNodeTimestampAttribute(NYT::TNode const& node, TString const& name) {
        auto const& value = GetNodeAttribute<TString>(node, name);
        TInstant result;
        Y_ENSURE(TInstant::TryParseIso8601(value, result)
            , "Expected ISO8601 date time format, got something else" << value);
        return result;
    }

    NYT::ENodeType GetNodeType(NYT::IClientBasePtr client, const TString& path) {
        return FromString<NYT::ENodeType>(
            client->Get(JoinYtMeta(path, "type")).AsString());
    }

    bool IsYtLink(NYT::IClientBasePtr client, const TString& path) {
        return GetYtAttr(client, path, "path").AsString() != path;
    }

    bool WaitAndLock(
        NYT::ITransactionPtr transaction,
        const TString& path,
        NYT::ELockMode lockMode,
        TDuration timeout,
        NYT::TLockOptions lockOptions)
    {
        Y_VERIFY(transaction);
        Y_ENSURE(transaction->Exists(path), TStringBuilder() << "Attempt to acquire lock at not-existing " << path);

        lockOptions.Waitable(true);
        NYT::TLockId lockId = transaction->Lock(path, lockMode, lockOptions)->GetId();
        TInstant start = TInstant::Now();
        bool acquired = false;
        while ((timeout.GetValue() == 0) || ((TInstant::Now() - start) < timeout)) {
            if (transaction->Get(Sprintf("#%s/@state", GetGuidAsString(lockId).data())).AsString() == "acquired") { // avoid prepending YT_PREFIX in Get due to YT-6724
                acquired = true;
                TString lockFullPath = path;
                if (lockOptions.ChildKey_.Defined()) {
                    lockFullPath = JoinYtPath(lockFullPath, lockOptions.ChildKey_.GetRef());
                }
                if (lockOptions.AttributeKey_.Defined()) {
                    lockFullPath = JoinYtMeta(lockFullPath, lockOptions.AttributeKey_.GetRef());
                }
                L_INFO << lockMode << " lock for node " << lockFullPath << " has just been acquired.";
                break;
            }
            Sleep(TDuration::Seconds(2));
        }

        return acquired;
    }

    void UpdateNode(NYT::TNode& baseNode, const NYT::TNode& updateNode) {
        if (baseNode.IsMap() && updateNode.IsMap()) {
            for (const auto& [key, value] : updateNode.AsMap()) {
                UpdateNode(baseNode[key], value);
            }
        } else {
            baseNode = updateNode;
        }
    }

} // namespace NJupiter.
