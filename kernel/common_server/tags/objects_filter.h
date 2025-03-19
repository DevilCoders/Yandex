#pragma once

#include <kernel/common_server/processors/common/handler.h>
#include <kernel/common_server/rt_background/settings.h>
#include <kernel/common_server/tags/manager.h>
namespace NCS {
    namespace NTags {
        class TTagChecker {
        private:
            CSA_MAYBE(TTagChecker, bool, Busy);
            CS_ACCESS(TTagChecker, bool, Exists, true);
            CSA_DEFAULT(TTagChecker, TString, TagName);
        public:
            TString SerializeToString() const;
            bool DeserializeFromString(const TString& info);
            template <class TDBObjectTag>
            bool Check(const TVector<TDBObjectTag>& tags) const {
                for (auto&& i : tags) {
                    if (i.GetName() == GetTagName()) {
                        if (!Exists) {
                            return false;
                        }
                        if (Busy) {
                            if (*Busy) {
                                if (!i.GetPerformerId()) {
                                    return false;
                                }
                            } else {
                                if (!!i.GetPerformerId()) {
                                    return false;
                                }
                            }
                        }
                        return true;
                    }
                }
                return !Exists;
            }
        };

        class TTagsCondition {
        private:
            CSA_DEFAULT(TTagsCondition, TVector<TTagChecker>, Checkers);
            TString OriginalString;
        public:
            template <class TDBObjectTag>
            bool Check(const TVector<TDBObjectTag>& tags) const {
                for (auto&& i : Checkers) {
                    if (!i.Check(tags)) {
                        return false;
                    }
                }
                return true;
            }
            TString SerializeToString() const;
            Y_WARN_UNUSED_RESULT bool DeserializeFromString(const TString& condition);
            TSet<TString> GetAffectedTagNames() const;
        };

        class TObjectFilter {
        private:
            CSA_DEFAULT(TObjectFilter, TVector<TTagsCondition>, Conditions);
            TString OriginalString;
        public:
            template <class TDBObjectTag>
            bool Check(const TTaggedObject<TDBObjectTag>& taggedObject) const {
                return Check(taggedObject.GetTags());
            }

            template <class TDBObjectTag>
            bool Check(const TVector<TDBObjectTag>& tags) const {
                for (auto&& i : Conditions) {
                    if (i.Check(tags)) {
                        return true;
                    }
                }
                return false;
            }

            TObjectFilter& Merge(const TObjectFilter& tf);
            TString SerializeToString() const;
            Y_WARN_UNUSED_RESULT bool DeserializeFromString(const TString& condition);
            TSet<TString> GetAffectedTagNames() const;
        };
    }
}
