#pragma once

#include "map.h"
#include "maybe.h"
#include "set.h"
#include "string.h"
#include "type_traits.h"
#include "unordered_map.h"
#include "unordered_set.h"
#include "vector.h"

#include <util/generic/map.h>
#include <util/generic/maybe.h>
#include <util/generic/stack.h>
#include <util/generic/utility.h>
#include <util/generic/vector.h>
#include <util/stream/format.h>
#include <util/system/type_name.h>

// BEWARE mms pointers are not supported.

namespace NMms {
    namespace NImpl {
        template <class T>
        size_t PtrValue(const T* obj) {
            return reinterpret_cast<size_t>(obj);
        }

        template <class T>
        mms::impl::Yes _HasParasiteLoad(mms::impl::Check<size_t (T::*)() const, &T::parasiteLoad>*);

        template <class T>
        mms::impl::No _HasParasiteLoad(...);

        template <class T>
        struct THasParasiteLoad {
            static const bool Value = sizeof(_HasParasiteLoad<T>(0)) == sizeof(mms::impl::Yes);
        };

        template <class T, bool hasParasiteLoad>
        struct TParasiteHelper;

        template <class T>
        struct TParasiteHelper<T, false> {
            size_t operator()(...) const {
                return 0;
            }
        };

        template <class T>
        struct TParasiteHelper<T, true> {
            size_t operator()(const T* object) const {
                return object->parasiteLoad();
            }
        };

        template <class T>
        size_t ParasiteLoad(const T* object) {
            return TParasiteHelper<T, THasParasiteLoad<T>::Value>()(object);
        }

    }

    class TSizeReport {
    public:
        TSizeReport(size_t size, const std::type_info* type, const char* name, const TVector<TSizeReport>& children)
            : Size_(size)
            , Type_(type)
            , Name_(name)
            , Children_(children)
        {
        }

        size_t Size() const {
            return Size_;
        }

        friend IOutputStream& operator<<(IOutputStream& out, const TSizeReport& sizeReport) {
            sizeReport.PrettyPrint(&out);
            return out;
        }

        void PrettyPrint(IOutputStream* out, int maxDepth = -1) const {
            PrettyPrint(Size_, Size_, 0, maxDepth, out);
        }

    private:
        void PrettyPrint(size_t totalSize, size_t parentSize, int depth, int maxDepth, IOutputStream* out) const {
            DoIndent(depth, out);
            if (Name_) {
                Y_ASSERT(Type_);
                *out << Name_ << ": ";
            }

            *out << "["
                 << Prec(100.0 * Size_ / Max<size_t>(totalSize, 1), PREC_POINT_DIGITS, 2) << " total, "
                 << Prec(100.0 * Size_ / Max<size_t>(parentSize, 1), PREC_POINT_DIGITS, 2) << " relative, "
                 << HumanReadableSize(Size_, SF_BYTES) << "; "
                 << "efficiency " << Prec(100.0 * Efficiency(), PREC_POINT_DIGITS, 2)
                 << "] ";

            if (Type_) {
                *out << CppDemangle(Type_->name());
            }

            if (!Children_.empty() && (maxDepth == -1 || depth < maxDepth)) {
                *out << " {\n";
                for (const TSizeReport& report : Children_) {
                    report.PrettyPrint(totalSize, Size_, depth + 1, maxDepth, out);
                }
                DoIndent(depth, out);
                *out << "};\n";
            } else {
                *out << ";\n";
            }
        }

        double Efficiency() const {
            if (Children_.empty()) {
                return 1.0;
            }
            size_t childrenSize = 0;
            for (const TSizeReport& report : Children_) {
                childrenSize += report.Size();
            }

            return static_cast<double>(childrenSize) / Size();
        }

        static void DoIndent(int indent, IOutputStream* out) {
            indent *= 4;
            NFormatPrivate::WriteChars(*out, ' ', indent);
        }

    private:
        size_t Size_;
        const std::type_info* Type_;
        const char* Name_;
        TVector<TSizeReport> Children_;
    };

    namespace NImpl {
        class TSizeReportBuilder;

        template <class TMM>
        void DoIntrospect(const TMM* object, const char* name, TSizeReportBuilder* builder);

        class TSizeReportBuilder {
            struct TNode {
                explicit TNode(const char* name, const std::type_info* type)
                    : Name(name)
                    , Type(type)
                    , Size(0)
                {
                }

                template <class T>
                TNode& Node(size_t offset, const char* name) {
                    auto node = children.find(offset);
                    if (node == children.end()) {
                        node = children.insert(std::make_pair(offset, TNode(name, &typeid(T)))).first;
                    }

                    return node->second;
                }

                const char* Name;
                const std::type_info* Type;
                size_t Size;
                TMap<size_t, TNode> children; // offset -> node
            };

        public:
            TSizeReportBuilder()
                : Root_(nullptr, nullptr)
                , MinAddr_(Max<size_t>())
                , MaxAddr_(0)
            {
                Nodes_.push(&Root_);
            }

            void UpdateAddressRange(size_t from, size_t to) {
                MinAddr_ = Min(MinAddr_, from);
                MaxAddr_ = Max(MaxAddr_, to);
            }

            template <class T>
            void UpdateAddressRange(T* t) {
                UpdateAddressRange(PtrValue(t), PtrValue(t) + sizeof(T));
            }

            template <class T>
            void Add(size_t offset, const char* name) {
                Top()->Node<T>(offset, name).Size += sizeof(T);
            }

            void AddParasiteLoad(size_t size) {
                Top()->Size += size;
            }

            template <class T>
            void PushContext(const char* name, size_t offset) {
                Nodes_.push(&Top()->Node<T>(offset, name));
            }

            void PopContext() {
                Nodes_.pop();
            }

            TSizeReport Build() const {
                Y_ASSERT(Root_.children.size() == 1);
                size_t realSize = MaxAddr_ - MinAddr_;
                return Build(&Root_.children.begin()->second, realSize);
            }

        private:
            TSizeReport Build(const TNode* node, const ::TMaybe<size_t>& overrideSize) const {
                size_t size = node->Size;
                TVector<TSizeReport> children;

                for (const auto& kv : node->children) {
                    TSizeReport child = Build(&kv.second, ::Nothing());
                    size += child.Size();
                    children.push_back(child);
                }

                if (overrideSize) {
                    Y_ASSERT(*overrideSize >= size);
                    size = *overrideSize;
                }

                return TSizeReport(size, node->Type, node->Name, children);
            }

            TNode* Top() {
                return Nodes_.top();
            }

        private:
            TNode Root_;
            TStack<TNode*> Nodes_;
            size_t MinAddr_;
            size_t MaxAddr_;
        };

        template <class Seq>
        struct TSequenceIntrospectionHelper {
            static void Introspect(const Seq* object, const char* name, size_t offset, TSizeReportBuilder* builder) {
                builder->PushContext<Seq>(name, offset);
                builder->AddParasiteLoad(ParasiteLoad(object) + sizeof(Seq));
                builder->UpdateAddressRange(object);
                for (const auto& v : *object) {
                    DoIntrospect(&v, "<payload>", 0, builder);
                }
                builder->PopContext();
            }
        };

        template <class TMM, bool hasTraverseFields>
        struct TIntrospectionHelper;

        template <class TMM>
        struct TIntrospectionHelper<TVectorType<TMmapped, TMM>, false> : TSequenceIntrospectionHelper<TVectorType<TMmapped, TMM>> {};

        template <class TMMKey, class TMMValue, template <class> class Cmp>
        struct TIntrospectionHelper<TMapType<TMmapped, TMMKey, TMMValue, Cmp>, false> : TSequenceIntrospectionHelper<TMapType<TMmapped, TMMKey, TMMValue, Cmp>> {};

        template <class TMM, template <class> class Cmp>
        struct TIntrospectionHelper<TSetType<TMmapped, TMM, Cmp>, false> : TSequenceIntrospectionHelper<TSetType<TMmapped, TMM, Cmp>> {};

        template <
            class TMMKey,
            class TMMValue,
            template <class> class Hash,
            template <class> class Eq>
        struct TIntrospectionHelper<TUnorderedMap<TMmapped, TMMKey, TMMValue, Hash, Eq>, false> : TSequenceIntrospectionHelper<TUnorderedMap<TMmapped, TMMKey, TMMValue, Hash, Eq>> {};

        template <
            class TMM,
            template <class> class Hash,
            template <class> class Eq>
        struct TIntrospectionHelper<TUnorderedSet<TMmapped, TMM, Hash, Eq>, false> : TSequenceIntrospectionHelper<TUnorderedSet<TMmapped, TMM, Hash, Eq>> {};

        template <>
        struct TIntrospectionHelper<TStringType<TMmapped>, false> : TSequenceIntrospectionHelper<TStringType<TMmapped>> {};

        template <class TMM>
        struct TIntrospectionHelper<TMM, false> {
            static void Introspect(const TMM* object, const char* name, size_t offset, TSizeReportBuilder* builder) {
                builder->Add<TMM>(offset, name);
                builder->UpdateAddressRange(object);
            }
        };

        template <class TMM>
        struct TIntrospectionHelper<TMaybe<TMmapped, TMM>, false> {
            typedef TMaybe<TMmapped, TMM> Type;
            static void Introspect(const Type* object, const char* name, size_t offset, TSizeReportBuilder* builder) {
                builder->PushContext<Type>(name, offset);
                builder->AddParasiteLoad(sizeof(Type));
                builder->UpdateAddressRange(object);
                if (*object) {
                    DoIntrospect(object->Get(), "<payload>", 0, builder);
                }
                builder->PopContext();
            }
        };

        template <class TMM>
        struct TIntrospectionHelper<TMM, true> {
            template <class T>
            class TRef {
            public:
                explicit TRef(T* t)
                    : T_(t)
                {
                }

                template <class... Args>
                TRef& operator()(Args&&... args) {
                    T_->operator()(std::forward<Args>(args)...);
                    return *this;
                }

            private:
                T* T_;
            };

            class TAction {
            public:
                TAction(size_t parentAddr, size_t expectedSize, TSizeReportBuilder* builder)
                    : ParentAddr_(parentAddr)
                    , ExpectedSize_(expectedSize)
                    , PredictedOffset_(0)
                    , Builder_(builder)
                {
                }

                ~TAction() {
                    Y_ASSERT(PredictedOffset_ <= ExpectedSize_);
                    Builder_->AddParasiteLoad(ExpectedSize_ - PredictedOffset_);
                }

                template <class U, class... Us>
                TAction& operator()(const U& object, const Us&... us) {
                    operator()(object);
                    return operator()(us...);
                }

                template <class U>
                TAction& operator()(const U& object) {
                    AccountForField(&object, nullptr);
                    return *this;
                }

                template <class U>
                TAction& operator()(const mms::impl::FieldDescriptor<U>& desc) {
                    AccountForField(desc.object, desc.name);
                    return *this;
                }

            private:
                template <class U>
                void AccountForField(const U* object, const char* name) {
                    size_t addr = PtrValue(object);
                    Y_ASSERT(addr >= ParentAddr_);

                    size_t offset = addr - ParentAddr_;
                    Y_ASSERT(offset >= PredictedOffset_);

                    Builder_->AddParasiteLoad(offset - PredictedOffset_);
                    PredictedOffset_ = offset + sizeof(U);
                    DoIntrospect(object, name, addr - ParentAddr_, Builder_);
                }

            private:
                size_t ParentAddr_;
                size_t ExpectedSize_;
                size_t PredictedOffset_;
                TSizeReportBuilder* Builder_;
            };

            static void Introspect(const TMM* object, const char* name, size_t offset, TSizeReportBuilder* builder) {
                builder->PushContext<TMM>(name, offset);
                builder->UpdateAddressRange(object);
                {
                    TAction action(PtrValue(object), sizeof(TMM), builder);
                    mms::impl::traverseFields(*object, TRef<TAction>(&action));
                }
                builder->PopContext();
            }
        };

        template <class TMM>
        void DoIntrospect(const TMM* object, const char* name, size_t offset, TSizeReportBuilder* builder) {
            TIntrospectionHelper<TMM, mms::impl::HasTraverseFields<TMM>::value>::Introspect(object, name, offset, builder);
        }

    }

    template <class TMM>
    TSizeReport IntrospectSize(const TMM* object) {
        NImpl::TSizeReportBuilder builder;

        NImpl::DoIntrospect(object, nullptr, 0, &builder);

        return builder.Build();
    }

}
