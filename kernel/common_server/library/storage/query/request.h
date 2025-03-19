#pragma once

#include "abstract.h"

#include <util/generic/set.h>
#include <kernel/common_server/library/interfaces/container.h>
#include <kernel/common_server/library/storage/records/abstract.h>
#include <kernel/common_server/library/storage/records/t_record.h>
#include <kernel/common_server/library/storage/abstract/statement.h>
#include <util/generic/maybe.h>

namespace NCS {
    namespace NStorage {
        namespace NRequest {
#define CSA_INIT(ClassName, FieldName)\
public:\
    template <class T, typename... Args>\
    T& Ret ## FieldName(Args... args) {\
        FieldName = new T(args...);\
        return FieldName.VGet<T>();\
    }\
    template <class T, typename... Args>\
    ClassName& Init ## FieldName(Args... args) {\
        FieldName = new T(args...);\
        return *this;\
    }\
private:

            class IExternalMethods {
            protected:
                virtual TString QuoteImpl(const TDBValueInput& v) const = 0;
                virtual bool DoIsValidFieldName(const TString& f) const = 0;
                virtual bool DoIsValidTableName(const TString& t) const = 0;
            public:
                virtual ~IExternalMethods() = default;
                TString Quote(const TDBValueInput& v) const {
                    return QuoteImpl(v);
                }

                template <class T>
                typename std::enable_if<std::is_enum<T>::value, TString>::type Quote(const T& v) const {
                    return Quote(::ToString(v));
                }

                bool IsValidFieldName(const TString& f) const {
                    if (!DoIsValidFieldName(f)) {
                        TFLEventLog::Error("invalid field name")("field_name", f);
                        return false;
                    }
                    return true;
                }

                bool IsValidTableName(const TString& t) const {
                    if (!DoIsValidTableName(t)) {
                        TFLEventLog::Error("invalid table name")("table_name", t);
                        return false;
                    }
                    return true;
                }
            };

            class TDebugExternalMethods: public IExternalMethods {
            protected:
                virtual TString QuoteImpl(const TDBValueInput& v) const override {
                    return TDBValueInputOperator::SerializeToString(v);
                }
                virtual bool DoIsValidFieldName(const TString& /*f*/) const override {
                    return true;
                }
                virtual bool DoIsValidTableName(const TString& /*t*/) const override {
                    return true;
                }
            public:
            };

            class INode {
            protected:
                virtual bool DoToString(const IExternalMethods& external, IOutputStream& os) const = 0;
            public:
                using TPtr = TAtomicSharedPtr<INode>;
                virtual ~INode() = default;
                bool ToString(const IExternalMethods& external, IOutputStream& os) const;
                TString GetStringDef(const IExternalMethods& external, const TString& def = "") const;
                TString DebugString() const;
            };

            class TNode: public TBaseInterfaceContainer<INode> {
            private:
                using TBase = TBaseInterfaceContainer<INode>;
            public:
                using TBase::TBase;
                template <class T, typename... Args>
                T& Ret(Args... args) {
                    return RetObject<T>(args...);
                }
                template <class T, typename... Args>
                TNode& Init(Args... args) {
                    InitObject<T>(args...);
                    return *this;
                }
                bool ToString(const IExternalMethods& external, IOutputStream& os) const;
                TString DebugString() const {
                    if (!Object) {
                        return "NO_OBJECT";
                    } else {
                        return Object->DebugString();
                    }
                }
            };

            class TValue: public INode {
            private:
                CSA_DEFAULT(TValue, TDBValue, Value);
            protected:
                virtual bool DoToString(const IExternalMethods& external, IOutputStream& os) const override;
            public:
                template <class T>
                TValue(const T& v, const typename std::enable_if<std::is_enum<T>::value, TNull>::type& vFlag = TNull())
                    : Value(::ToString(v)) {
                    Y_UNUSED(vFlag);
                }

                template <class T>
                TValue(const T& v, const typename std::enable_if<!std::is_enum<T>::value, TNull>::type& vFlag = TNull())
                    : Value(TDBValueOperator::Build(v)) {
                    Y_UNUSED(vFlag);
                }

                TValue(const TDBValue& v)
                    : Value(v) {
                }
            };

            class TNullValue: public INode {
            protected:
                virtual bool DoToString(const IExternalMethods& /*external*/, IOutputStream& os) const override {
                    os << "NULL" << Endl;
                    return true;
                }
            public:
            };

            class TExistsChecker: public INode {
            private:
                CSA_DEFAULT(TExistsChecker, TNode, Selection);
                CSA_INIT(TExistsChecker, Selection);
                CSA_FLAG(TExistsChecker, NegativeChecker, false);
            protected:
                virtual bool DoToString(const IExternalMethods& external, IOutputStream& os) const override;
            public:
                TExistsChecker(const bool negative = false)
                    : NegativeCheckerFlag(negative)
                {

                }
            };

            class TFieldName: public INode {
            private:
                CSA_DEFAULT(TFieldName, TString, TableName);
                CSA_DEFAULT(TFieldName, TVector<TString>, FieldNames);
            protected:
                virtual bool DoToString(const IExternalMethods& external, IOutputStream& os) const override;
            public:
                TFieldName& InitFromSet(const TSet<TString>& fieldNamesSet) {
                    TVector<TString> local(fieldNamesSet.begin(), fieldNamesSet.end());
                    std::swap(FieldNames, local);
                    return *this;
                }
                bool operator!() const {
                    return FieldNames.empty();
                }
                TFieldName() = default;
                TFieldName(const TString& fieldName) {
                    FieldNames.emplace_back(fieldName);
                }
                TFieldName(const std::initializer_list<TString>& fieldNames)
                    : FieldNames(fieldNames) {

                }
            };

            class TTableName: public INode {
            private:
                CSA_DEFAULT(TTableName, TString, TableName);
            protected:
                virtual bool DoToString(const IExternalMethods& external, IOutputStream& os) const override;
            public:
                TTableName() = default;
                TTableName(const TString& tableName)
                    : TableName(tableName)
                {

                }
            };

            class TNullChecker: public INode {
            private:
                CSA_DEFAULT(TNullChecker, TNode, FieldName);
                CSA_INIT(TNullChecker, FieldName);
                CSA_FLAG(TNullChecker, Null, true);
                CSA_DEFAULT(TNullChecker, TNode, EmptyValue);
                CSA_INIT(TNullChecker, EmptyValue);
            public:
                virtual bool DoToString(const IExternalMethods& external, IOutputStream& os) const override;
                TNullChecker(const TString& fieldName) {
                    InitFieldName<TFieldName>(fieldName);
                }
                template <class TEmptyValue>
                TNullChecker(const TString& fieldName, const TEmptyValue& eValue) {
                    InitFieldName<TFieldName>(fieldName);
                    InitEmptyValue<TValue>(eValue);
                }
            };

            class TNullInit: public INode {
            private:
                CSA_DEFAULT(TNullInit, TNode, FieldName);
                CSA_INIT(TNullInit, FieldName);
            public:
                virtual bool DoToString(const IExternalMethods& external, IOutputStream& os) const override;
                TNullInit(const TString& fieldName) {
                    InitFieldName<TFieldName>(fieldName);
                }
            };

            enum class EMultiOperation {
                And,
                Or,
                ObjectsSet
            };

            enum class EBinaryOperation {
                Equal,
                NotEqual,
                Greater,
                Less,
                GreaterOrEqual,
                LessOrEqual,
                In,
                NotIn
            };

            class TMultiOperator: public INode, public TNonCopyable {
            private:
                CS_ACCESS(TMultiOperator, EMultiOperation, Operation, EMultiOperation::And);
                CSA_FLAG(TMultiOperator, Negative, false);
                CSA_DEFAULT(TMultiOperator, TVector<TNode>, Nodes);
            protected:
                virtual bool DoToString(const IExternalMethods& external, IOutputStream& os) const override;
            public:
                TMultiOperator(TMultiOperator&& source) = default;

                TMultiOperator(const EMultiOperation op = EMultiOperation::And)
                    : Operation(op)
                {

                }

                bool IsEmpty() const {
                    return Nodes.empty();
                }

                template <class T, typename... TArgs>
                T& RetNode(TArgs... args) {
                    Nodes.emplace_back(new T(args...));
                    return Nodes.back().VGet<T>();
                }

                template <class T, typename... TArgs>
                TMultiOperator& InitNode(TArgs... args) {
                    Nodes.emplace_back(new T(args...));
                    return *this;
                }

                TMultiOperator& FillBinary(const TTableRecord& rwt);
                template <class TRecord>
                TMultiOperator& FillRecords(const TVector<TRecord>& rwt, const EMultiOperation mOperator = EMultiOperation::Or) {
                    SetOperation(mOperator);
                    if (!rwt.size()) {
                        return *this;
                    }
                    for (auto&& i : rwt) {
                        RetNode<TMultiOperator>().FillBinary(i);
                    }
                    return *this;
                }
                TMultiOperator& FillBinary(const TTableRecordWT& rwt);
                TMultiOperator& FillFilterSpecial(const TTableRecordWT& rwt);

                template <class TContainer>
                TMultiOperator& FillValues(const TContainer& values) {
                    Operation = EMultiOperation::ObjectsSet;
                    for (auto&& i : values) {
                        InitNode<TValue>(i);
                    }
                    return *this;
                }

                template <class T>
                TMultiOperator& FillValues(const std::initializer_list<T>& values) {
                    Operation = EMultiOperation::ObjectsSet;
                    for (auto&& i : values) {
                        InitNode<TValue>(i);
                    }
                    return *this;
                }
            };

            class TBinaryOperator: public INode {
            private:
                CS_ACCESS(TBinaryOperator, EBinaryOperation, Operation, EBinaryOperation::Equal);
                CSA_DEFAULT(TBinaryOperator, TNode, Left);
                CSA_DEFAULT(TBinaryOperator, TNode, Right);
                CSA_INIT(TBinaryOperator, Left);
                CSA_INIT(TBinaryOperator, Right);
            protected:
                virtual bool DoToString(const IExternalMethods& external, IOutputStream& os) const override;
            public:
                template <class T>
                TBinaryOperator& SetRight(const T& value) {
                    Right = new T(value);
                    return *this;
                }

                TBinaryOperator(const EBinaryOperation op = EBinaryOperation::Equal)
                    : Operation(op) {

                }

                template <class T>
                TBinaryOperator(const TString& fieldName, const T& fieldValue, const EBinaryOperation op = EBinaryOperation::Equal)
                    : Operation(op) {
                    InitLeft<TFieldName>(fieldName).InitRight<TValue>(fieldValue);
                }

                template <class T>
                TBinaryOperator(const TString& fieldName, const TSet<T>& fieldValues, const EBinaryOperation op = EBinaryOperation::In)
                    : Operation(op) {
                    InitLeft<TFieldName>(fieldName).RetRight<TMultiOperator>().FillValues(fieldValues);
                }

                template <class T>
                TBinaryOperator(const TString& fieldName, const TVector<T>& fieldValues, const EBinaryOperation op = EBinaryOperation::In)
                    : Operation(op) {
                    InitLeft<TFieldName>(fieldName).RetRight<TMultiOperator>().FillValues(fieldValues);
                }

            };

            class TFieldOrder {
            private:
                CSA_DEFAULT(TFieldOrder, TString, FieldId);
                CSA_FLAG(TFieldOrder, Ascending, true);
            public:

                TFieldOrder() = default;
                TFieldOrder(const TFieldOrder& base) = default;
                TFieldOrder(const TString& fieldId)
                    : FieldId(fieldId) {

                }

                NCS::NScheme::TScheme GetScheme(const bool isTemplate = true) const {
                    NCS::NScheme::TScheme result;
                    if (isTemplate) {
                        result.Add<TFSString>("field_id").SetDefault(FieldId).ReadOnly(true);
                    } else {
                        result.Add<TFSString>("field_id").ReadOnly(false);
                    }
                    result.Add<TFSBoolean>("asc").SetDefault(AscendingFlag);
                    return result;
                }

                bool DeserializeFromJson(const NJson::TJsonValue& jsonInfo) {
                    if (!TJsonProcessor::Read(jsonInfo, "field_id", FieldId)) {
                        return false;
                    }
                    if (!TJsonProcessor::Read(jsonInfo, "asc", AscendingFlag)) {
                        return false;
                    }
                    return true;
                }

                NJson::TJsonValue SerializeToJson() const {
                    NJson::TJsonValue result = NJson::JSON_MAP;
                    TJsonProcessor::Write(result, "field_id", FieldId);
                    TJsonProcessor::Write(result, "asc", AscendingFlag);
                    return result;
                }
            };

            class TOrder: public INode {
            private:
                CSA_READONLY_DEF(TVector<TFieldOrder>, Fields);
            protected:
                virtual bool DoToString(const IExternalMethods& external, IOutputStream& os) const override;
            public:
                TOrder() = default;
                TOrder(const TString& fieldName) {
                    Fields.emplace_back(TFieldOrder().SetFieldId(fieldName));
                }

                template <class TObject>
                TOrder& SetFields(const TVector<TObject>& objects) {
                    TVector<TFieldOrder> fields;
                    for (auto&& i : objects) {
                        fields.emplace_back(i);
                    }
                    std::swap(Fields, fields);
                    return *this;
                }

                TOrder& SetFieldNames(const TVector<TString>& fieldNames) {
                    TVector<TFieldOrder> fields;
                    for (auto&& i : fieldNames) {
                        fields.emplace_back(TFieldOrder().SetFieldId(i));
                    }
                    std::swap(Fields, fields);
                    return *this;
                }

                TOrder& RegisterField(const TString& fieldId, const bool ascending = true) {
                    for (auto&& i : Fields) {
                        Y_ASSERT(i.GetFieldId() != fieldId);
                    }
                    Fields.emplace_back(TFieldOrder(fieldId).Ascending(ascending));
                    return *this;
                }
            };

            class IResponseContainer: public INode {
            private:
                mutable IBaseRecordsSet* RecordsSet = nullptr;
            public:
                using TPtr = TAtomicSharedPtr<IResponseContainer>;
                IBaseRecordsSet* GetRecordsSet() const {
                    return RecordsSet;
                }

                IBaseRecordsSet* MutableRecordsSet() const {
                    return RecordsSet;
                }

                const IResponseContainer& SetRecordsSet(IBaseRecordsSet* records) const {
                    RecordsSet = records;
                    return *this;
                }
            };

            class TSelect: public IResponseContainer {
            private:
                CSA_DEFAULT(TSelect, TNode, Source);
                CSA_DEFAULT(TSelect, TNode, Fields);
                CSA_DEFAULT(TSelect, TNode, Condition);
                CSA_DEFAULT(TSelect, TNode, OrderBy);
                CS_ACCESS(TSelect, ui32, CountLimit, 0);
                CS_ACCESS(TSelect, ui32, Offset, 0);
                CSA_FLAG(TSelect, Distinct, false);
                CSA_INIT(TSelect, Source);
                CSA_INIT(TSelect, Fields);
                CSA_INIT(TSelect, Condition);
                CSA_INIT(TSelect, OrderBy);
            protected:
                virtual bool DoToString(const IExternalMethods& external, IOutputStream& os) const override;
            public:
                TSelect() = default;
                TSelect(const TString& tableName, IBaseRecordsSet* records = nullptr) {
                    InitSource<TTableName>(tableName);
                    SetRecordsSet(records);
                }
            };

            class TDelete: public IResponseContainer, public TMoveOnly {
            private:
                CSA_DEFAULT(TDelete, TTableName, TableName);
                CSA_DEFAULT(TDelete, TNode, Condition);
                CSA_INIT(TDelete, Condition);
                CSA_DEFAULT(TDelete, TNode, Fields);
                CSA_INIT(TDelete, Fields);
            protected:
                virtual bool DoToString(const IExternalMethods& external, IOutputStream& os) const override;
            public:
                TDelete(const TString& tableName, IBaseRecordsSet* records = nullptr)
                    : TableName(tableName) {
                    SetRecordsSet(records);
                }
            };

            class TCommit: public IResponseContainer, public TMoveOnly {
            protected:
                virtual bool DoToString(const IExternalMethods& /*external*/, IOutputStream& os) const override;
            };

            template <class TChildren>
            class TRequestWithRecords: public IResponseContainer, public TMoveOnly {
            private:
                CSA_PROTECTED_DEF(TRequestWithRecords, TVector<TTableRecord>, Records);
            public:
                template <class T>
                TTableRecord& AddRecord(const T& object) {
                    Records.emplace_back(object.SerializeToTableRecord());
                    return Records.back();
                }

                template <class TObject>
                TChildren& FillRecords(const TVector<TObject>& objects) {
                    for (auto&& i : objects) {
                        Records.emplace_back(i.SerializeToTableRecord());
                    }
                    return *VerifyDynamicCast<TChildren*>(this);
                }
                template <class TObject>
                TChildren& FillRecords(const TVector<const TObject*>& objects) {
                    for (auto&& i : objects) {
                        if (!i) {
                            continue;
                        }
                        Records.emplace_back(i->SerializeToTableRecord());
                    }
                    return *VerifyDynamicCast<TChildren*>(this);
                }
                template <class T>
                TChildren& FillRecords(const std::initializer_list<T>& objects) {
                    for (auto&& i : objects) {
                        Records.emplace_back(i.SerializeToTableRecord());
                    }
                    return *VerifyDynamicCast<TChildren*>(this);
                }
            };

            class TInsert: public TRequestWithRecords<TInsert> {
            public:
                enum class EConflictPolicy {
                    Update,
                    Nothing
                };
            private:
                CSA_DEFAULT(TInsert, TTableName, TableName);
                CSA_DEFAULT(TInsert, TFieldName, UniqueFieldIds);
                CS_ACCESS(TInsert, EConflictPolicy, ConflictPolicy, EConflictPolicy::Nothing);
            protected:
                virtual bool DoToString(const IExternalMethods& external, IOutputStream& os) const override;
            public:
                TInsert& SetUniqueFieldId(const TString& fieldId) {
                    UniqueFieldIds = fieldId;
                    return *this;
                }

                TInsert(const TString& tableName, IBaseRecordsSet* records = nullptr)
                    : TableName(tableName) {
                    SetRecordsSet(records);
                }

            };

            class TUpdate: public IResponseContainer, public TMoveOnly {
            private:
                CSA_DEFAULT(TUpdate, TTableName, TableName);
                CSA_DEFAULT(TUpdate, TNode, Condition);
                CSA_DEFAULT(TUpdate, TNode, Update);
                CSA_INIT(TUpdate, Condition);
                CSA_INIT(TUpdate, Update);
            protected:
                virtual bool DoToString(const IExternalMethods& external, IOutputStream& os) const override;
            public:
                TUpdate(const TString& tableName, IBaseRecordsSet* records = nullptr)
                    : TableName(tableName) {
                    SetRecordsSet(records);
                }
            };

            class TMultiUpdate: public TRequestWithRecords<TMultiUpdate> {
            private:
                CSA_DEFAULT(TMultiUpdate, TTableName, TableName);
                CSA_DEFAULT(TMultiUpdate, TSet<TString>, ConditionFieldIds);
            protected:
                virtual bool DoToString(const IExternalMethods& external, IOutputStream& os) const override;
            public:
                TMultiUpdate(const TString& tableName, IBaseRecordsSet* records = nullptr)
                    : TableName(tableName) {
                    SetRecordsSet(records);
                }
            };

            class TQuery: public TBaseInterfaceContainer<IResponseContainer> {
            private:
                using TBase = TBaseInterfaceContainer<IResponseContainer>;
            public:
                using TBase::TBase;

                TMaybe<TStatement> BuildStatement(const IExternalMethods& external) const;
                TQuery& SetRecordsSet(IBaseRecordsSet* value);
                TString DebugString() const;

                TQuery(const TSelect& req)
                    : TBase(new TSelect(req)) {

                }

                TQuery(const TSelect&& req)
                    : TBase(new TSelect(std::move(req))) {

                }
            };

            class TRequests {
            private:
                CSA_READONLY_DEF(TVector<TQuery>, Queries);
            public:
                TRequests() = default;
                TRequests(const TVector<TQuery>& queries)
                    : Queries(queries)
                {

                }
                TString DebugString() const;
                TQuery& AddQuery(const TQuery& query) {
                    Queries.emplace_back(query);
                    return Queries.back();
                }
                template <class T, typename... Args>
                T& AddRequest(Args... args) {
                    Queries.emplace_back(new T(args...));
                    return Queries.back().VGet<T>();
                }
                bool BuildStatements(const IExternalMethods& external, TVector<TStatement>& result) const;
                bool BuildStatementsPtr(const IExternalMethods& external, TVector<TStatement::TPtr>& result) const;
            };
        }
    }
}

using TSRRequests = NCS::NStorage::NRequest::TRequests;
using TSRCommit = NCS::NStorage::NRequest::TCommit;
using TSRQuery = NCS::NStorage::NRequest::TQuery;
using TSRSelect = NCS::NStorage::NRequest::TSelect;
using TSRInsert = NCS::NStorage::NRequest::TInsert;
using TSRDelete = NCS::NStorage::NRequest::TDelete;
using TSRUpdate = NCS::NStorage::NRequest::TUpdate;
using TSRMultiUpdate = NCS::NStorage::NRequest::TMultiUpdate;
using TSRValue = NCS::NStorage::NRequest::TValue;
using TSRCondition = NCS::NStorage::NRequest::TNode;
using TSRNullChecker = NCS::NStorage::NRequest::TNullChecker;
using TSRNullInit = NCS::NStorage::NRequest::TNullInit;

using TSRTableName = NCS::NStorage::NRequest::TTableName;
using TSRFieldName = NCS::NStorage::NRequest::TFieldName;
using TSRExistsChecker = NCS::NStorage::NRequest::TExistsChecker;
using TSROrder = NCS::NStorage::NRequest::TOrder;
using TSRMultiOperator = NCS::NStorage::NRequest::TMultiOperator;
using TSRBinaryOperator = NCS::NStorage::NRequest::TBinaryOperator;
using ESRBinaryOperation = NCS::NStorage::NRequest::EBinaryOperation;
using ESRMultiOperation = NCS::NStorage::NRequest::EMultiOperation;

using TSRMulti = NCS::NStorage::NRequest::TMultiOperator;
using TSRBinary = NCS::NStorage::NRequest::TBinaryOperator;
using ESRBinary = NCS::NStorage::NRequest::EBinaryOperation;
using ESRMulti = NCS::NStorage::NRequest::EMultiOperation;
