#include "request.h"

namespace NCS {
    namespace NStorage {
        namespace NRequest {

            bool TSelect::DoToString(const IExternalMethods& external, IOutputStream& os) const {
                os << "SELECT ";
                if (DistinctFlag) {
                    if (!Fields) {
                        TFLEventLog::Error("cannot made distinct request for * fields");
                        return false;
                    }
                    os << "distinct ";
                }
                if (!Fields) {
                    os << "*";
                } else if (!Fields.ToString(external, os)) {
                    return false;
                }
                os << " FROM ";
                if (!Source.ToString(external, os)) {
                    return false;
                }
                if (!!Condition) {
                    os << " WHERE ";
                    if (!Condition.ToString(external, os)) {
                        return false;
                    }
                }
                if (!!OrderBy) {
                    os << " ORDER BY ";
                    if (!OrderBy.ToString(external, os)) {
                        return false;
                    }
                }
                if (CountLimit) {
                    os << " LIMIT " << CountLimit << Endl;
                }
                if (Offset) {
                    os << " OFFSET " << Offset << Endl;
                }
                return true;
            }

            bool TOrder::DoToString(const IExternalMethods& external, IOutputStream& os) const {
                if (!Fields.size()) {
                    return true;
                }
                os << Endl;
                bool isFirst = true;
                for (auto&& f : Fields) {
                    if (!external.IsValidFieldName(f.GetFieldId())) {
                        return false;
                    }
                    if (!isFirst) {
                        os << ", ";
                    }
                    os << f.GetFieldId() << " " << (f.IsAscending() ? "ASC" : "DESC");
                    isFirst = false;
                }
                os << Endl;
                return true;
            }

            bool TMultiOperator::DoToString(const IExternalMethods& external, IOutputStream& os) const {
                if (Nodes.empty()) {
                    TFLEventLog::Error("incorrect request features for multi conditions node");
                    return false;
                }
                TVector<TString> nodeStr;
                for (auto&& i : Nodes) {
                    TStringStream sb;
                    if (!i.ToString(external, sb)) {
                        return false;
                    }
                    if (Operation == EMultiOperation::ObjectsSet) {
                        nodeStr.emplace_back(sb.Str());
                    } else {
                        nodeStr.emplace_back("(" + sb.Str() + ")");
                    }
                }
                TString opStr;
                switch (Operation) {
                    case EMultiOperation::And:
                        opStr = " AND ";
                        break;
                    case EMultiOperation::Or:
                        opStr = " OR ";
                        break;
                    case EMultiOperation::ObjectsSet:
                        opStr = ", ";
                        break;
                };
                if (IsNegative()) {
                    os << "( NOT (" << JoinSeq(opStr, nodeStr) << "))";
                } else {
                    os << JoinSeq(opStr, nodeStr);
                }
                return true;
            }

            NCS::NStorage::NRequest::TMultiOperator& TMultiOperator::FillBinary(const TTableRecord& rwt) {
                for (auto&& i : rwt) {
                    TMaybe<TDBValue> dbValue = TDBValueInputOperator::ToDBValue(i.second);
                    if (!dbValue) {
                        InitNode<TNullChecker>(i.first);
                    } else {
                        TBinaryOperator& bOp = RetNode<TBinaryOperator>();
                        bOp.InitLeft<TFieldName>(i.first).InitRight<TValue>(*dbValue);
                    }
                }
                return *this;
            }

            NCS::NStorage::NRequest::TMultiOperator& TMultiOperator::FillBinary(const TTableRecordWT& rwt) {
                for (auto&& i : rwt) {
                    InitNode<TBinaryOperator>(i.first, i.second);
                }
                return *this;
            }

            NCS::NStorage::NRequest::TMultiOperator& TMultiOperator::FillFilterSpecial(const TTableRecordWT& rwt) {
                for (auto&& i : rwt) {
                    const TString value = TDBValueOperator::SerializeToString(i.second);
                    if (value.StartsWith("??")) {
                        TStringBuf sb(value.data(), value.size());
                        sb.Skip(2);
                        TVector<TStringBuf> conditions = StringSplitter(sb).SplitBySet("&").SkipEmpty().ToList<TStringBuf>();
                        for (auto&& l : conditions) {
                            while (l.size() && l.StartsWith(' ')) {
                                l.Skip(1);
                            }
                            while (l.size() && l.EndsWith(' ')) {
                                l.Chop(1);
                            }
                            ESRBinary op = ESRBinary::Equal;
                            if (l.StartsWith(">=")) {
                                l.Skip(2);
                                op = ESRBinary::GreaterOrEqual;
                            } else if (l.StartsWith("<=")) {
                                l.Skip(2);
                                op = ESRBinary::LessOrEqual;
                            } else if (l.StartsWith("=")) {
                                l.Skip(1);
                                op = ESRBinary::Equal;
                            } else if (l.StartsWith(">")) {
                                l.Skip(1);
                                op = ESRBinary::Greater;
                            } else if (l.StartsWith("<")) {
                                l.Skip(1);
                                op = ESRBinary::Less;
                            } else if (l.StartsWith("!=")) {
                                l.Skip(2);
                                op = ESRBinary::NotEqual;
                            } else if (l.StartsWith("(") && l.EndsWith(")")) {
                                op = ESRBinary::In;
                                l.Chop(1);
                                l.Skip(1);
                            } else {
                                InitNode<TSRMulti>();
                                TFLEventLog::Error("cannot detect operation type")("raw_data", value)("local", l);
                                return *this;
                            }
                            while (l.size() && l.StartsWith(' ')) {
                                l.Skip(1);
                            }
                            while (l.size() && l.EndsWith(' ')) {
                                l.Chop(1);
                            }
                            if (!l.size()) {
                                InitNode<TSRMulti>();
                                TFLEventLog::Error("cannot detect operation value (empty)")("raw_data", value);
                                return *this;
                            }
                            if (op == ESRBinary::In) {
                                const TVector<TString> values = StringSplitter(l).SplitBySet(",").SkipEmpty().ToList<TString>();
                                InitNode<TSRBinary>(i.first, values);
                            } else {
                                InitNode<TSRBinary>(i.first, TString(l), op);
                            }
                        }
                    } else {
                        InitNode<TSRBinary>(i.first, i.second);
                    }
                }
                return *this;
            }

            bool TBinaryOperator::DoToString(const IExternalMethods& external, IOutputStream& os) const {
                TString opStr;
                switch (Operation) {
                    case EBinaryOperation::Equal:
                        opStr = "=";
                        break;
                    case EBinaryOperation::NotEqual:
                        opStr = "!=";
                        break;
                    case EBinaryOperation::Greater:
                        opStr = ">";
                        break;
                    case EBinaryOperation::Less:
                        opStr = "<";
                        break;
                    case EBinaryOperation::GreaterOrEqual:
                        opStr = ">=";
                        break;
                    case EBinaryOperation::LessOrEqual:
                        opStr = "<=";
                        break;
                    case EBinaryOperation::In:
                        opStr = "IN";
                        break;
                    case EBinaryOperation::NotIn:
                        opStr = "NOT IN";
                        break;
                };
                TStringStream sbLeft;
                if (!Left.ToString(external, sbLeft)) {
                    return false;
                }
                TStringStream sbRight;
                if (!Right.ToString(external, sbRight)) {
                    return false;
                }
                if (!sbLeft || !sbRight) {
                    TFLEventLog::Error("no left/right parts for binary operator");
                    return false;
                }
                if (Operation == EBinaryOperation::In || Operation == EBinaryOperation::NotIn) {
                    os << sbLeft.Str() << " " << opStr << " (" << sbRight.Str() << ")";
                } else {
                    os << sbLeft.Str() << " " << opStr << " " << sbRight.Str();
                }
                return true;
            }

            bool TValue::DoToString(const IExternalMethods& external, IOutputStream& os) const {
                auto pred = [&external](const auto& v) {
                    return external.Quote(v);
                };
                os << std::visit(pred, Value);
                return true;
            }

            bool TFieldName::DoToString(const IExternalMethods& external, IOutputStream& os) const {
                for (auto&& i : FieldNames) {
                    if (!external.IsValidFieldName(i)) {
                        return false;
                    }
                }
                if (!TableName) {
                    os << JoinSeq(", ", FieldNames);
                } else {
                    if (!external.IsValidTableName(TableName)) {
                        return false;
                    }
                    TVector<TString> fieldNamesLocal;
                    for (auto&& i : FieldNames) {
                        fieldNamesLocal.emplace_back(TableName + "." + i);
                    }
                    os << JoinSeq(", ", fieldNamesLocal);
                }
                return true;
            }

            bool TInsert::DoToString(const IExternalMethods& external, IOutputStream& ss) const {
                auto gLogging = TFLRecords::StartContext().Method("TInsert::DoToString")("table_name", TableName.GetTableName())("unique", UniqueFieldIds.DebugString())/*("condition", Condition.DebugString())*/;
                if (!Records.size()) {
                    TFLEventLog::Error("no records");
                    return false;
                }
                TSet<TString> fieldsAll;
                for (auto&& i : Records) {
                    for (auto&& f : i) {
                        fieldsAll.emplace(f.first);
                    }
                }
                ss << "INSERT INTO ";
                if (!TableName.ToString(external, ss)) {
                    return false;
                }
                ss << " (" << JoinSeq(", ", fieldsAll) << ")" << Endl;
                ss << "VALUES";
                for (ui32 i = 0; i < Records.size(); ++i) {
                    if (i) {
                        ss << "," << Endl;
                    }
                    const TString values = Records[i].GetValues(external, &fieldsAll);
                    ss << "(" << values << ")";
                }
                ss << Endl;
                if (!!UniqueFieldIds) {
                    ss << "ON CONFLICT(";
                    if (!UniqueFieldIds.ToString(external, ss)) {
                        return false;
                    }
                    ss << ") DO" << Endl;
                    if (ConflictPolicy == EConflictPolicy::Update) {
                        ss << "UPDATE SET" << Endl;
                        bool isFirst = true;
                        for (auto&& i : fieldsAll) {
                            if (isFirst) {
                                isFirst = false;
                            } else {
                                ss << ",";
                            }
                            ss << i << " = EXCLUDED." << i << Endl;
                        }
                    } else if (ConflictPolicy == EConflictPolicy::Nothing) {
                        ss << "NOTHING" << Endl;
                    } else {
                        TFLEventLog::Error("incorrect conflict policy")("policy", ConflictPolicy);
                        return false;
                    }
                }
                if (IResponseContainer::GetRecordsSet()) {
                    ss << " RETURNING *";
                }
                return true;
            }

            bool TDelete::DoToString(const IExternalMethods& external, IOutputStream& os) const {
                os << "DELETE FROM ";
                if (!TableName.ToString(external, os)) {
                    return false;
                }
                if (!!Condition) {
                    os << " WHERE" << Endl;
                    if (!Condition.ToString(external, os)) {
                        return false;
                    }
                }
                if (IResponseContainer::GetRecordsSet()) {
                    os << " RETURNING ";
                    if (!Fields) {
                        os << "*";
                    } else {
                        if (!Fields.ToString(external, os)) {
                            return false;
                        }
                    }
                }
                return true;
            }

            bool TNullChecker::DoToString(const IExternalMethods& external, IOutputStream& os) const {
                os << "(";
                os << "(";
                os << "(";
                if (!FieldName.ToString(external, os)) {
                    return false;
                }
                os << ") ";
                if (NullFlag) {
                    os << "IS NULL";
                } else {
                    os << "IS NOT NULL";
                }
                os << ")";
                if (!!EmptyValue) {
                    os << "OR (";
                    os << "(";
                    if (!FieldName.ToString(external, os)) {
                        return false;
                    }
                    os << ") = ";
                    os << "(";
                    if (!EmptyValue.ToString(external, os)) {
                        return false;
                    }
                    os << ")";
                    os << ")";
                }
                os << ")";
                return true;
            }

            bool INode::ToString(const IExternalMethods& external, IOutputStream& os) const {
                auto gLogging = TFLRecords::StartContext()("method", "request_to_string");
                if (!DoToString(external, os)) {
                    TFLEventLog::Alert("cannot build db request");
                    return false;
                }
                return true;
            }

            TString INode::GetStringDef(const IExternalMethods& external, const TString& def /*= ""*/) const {
                TStringStream ss;
                if (!ToString(external, ss)) {
                    return def;
                } else {
                    return ss.Str();
                }
            }

            TString INode::DebugString() const {
                TDebugExternalMethods extMethods;
                return GetStringDef(extMethods, "CANNOT_SERIALIZE");
            }

            bool TTableName::DoToString(const IExternalMethods& external, IOutputStream& os) const {
                if (!external.IsValidTableName(TableName)) {
                    return false;
                }
                os << TableName;
                return true;
            }

            bool TUpdate::DoToString(const IExternalMethods& external, IOutputStream& os) const {
                os << "UPDATE" << Endl;
                if (!TableName.ToString(external, os)) {
                    return false;
                }
                os << " SET" << Endl;
                if (!Update.ToString(external, os)) {
                    return false;
                }
                if (!!Condition) {
                    os << " WHERE" << Endl;
                    if (!Condition.ToString(external, os)) {
                        return false;
                    }
                }
                if (GetRecordsSet()) {
                    os << " RETURNING *";
                }
                return true;
            }

            bool TNullInit::DoToString(const IExternalMethods& external, IOutputStream& os) const {
                if (!FieldName.ToString(external, os)) {
                    return false;
                }
                os << " = NULL";
                return true;
            }

            TMaybe<NCS::NStorage::TStatement> TQuery::BuildStatement(const IExternalMethods& external) const {
                if (!Object) {
                    TFLEventLog::Error("not initialized query for build statement");
                    return Nothing();
                }
                TStringStream ss;
                if (!Object->ToString(external, ss)) {
                    TFLEventLog::Error("cannot build string query for statement construction");
                    return Nothing();
                }
                return TStatement(ss.Str(), Object->GetRecordsSet());
            }

            NCS::NStorage::NRequest::TQuery& TQuery::SetRecordsSet(IBaseRecordsSet* value) {
                if (!!Object) {
                    Object->SetRecordsSet(value);
                }
                return *this;
            }

            TString TQuery::DebugString() const {
                if (!Object) {
                    return "not_initialized_object";
                }
                return Object->DebugString();
            }

            bool TExistsChecker::DoToString(const IExternalMethods& external, IOutputStream& os) const {
                if (NegativeCheckerFlag) {
                    os << "NOT EXISTS (";
                } else {
                    os << "EXISTS (";
                }
                if (!Selection.ToString(external, os)) {
                    return false;
                }
                os << ")";
                return true;
            }

            bool TNode::ToString(const IExternalMethods& external, IOutputStream& os) const {
                if (!Object) {
                    TFLEventLog::Error("not initialized request node object");
                    return false;
                }
                if (!Object->ToString(external, os)) {
                    return false;
                }
                return true;
            }

            bool TCommit::DoToString(const IExternalMethods& /*external*/, IOutputStream& os) const {
                os << "COMMIT;";
                return true;
            }

            TString TRequests::DebugString() const {
                TVector<TString> strings;
                for (auto&& i : Queries) {
                    strings.emplace_back(i.DebugString());
                }
                return JoinSeq(";\n", strings);
            }

            bool TRequests::BuildStatements(const IExternalMethods& external, TVector<TStatement>& result) const {
                TVector<NCS::NStorage::TStatement> statements;
                for (auto&& i : Queries) {
                    TMaybe<NCS::NStorage::TStatement> st = i.BuildStatement(external);
                    if (!st) {
                        TFLEventLog::Error("cannot build request in queries set")("debug_query", i.DebugString());
                        return false;
                    }
                    statements.emplace_back(std::move(*st));
                }
                std::swap(result, statements);
                return true;
            }

            bool TRequests::BuildStatementsPtr(const IExternalMethods& external, TVector<TStatement::TPtr>& result) const {
                TVector<NCS::NStorage::TStatement::TPtr> statements;
                for (auto&& i : Queries) {
                    TMaybe<NCS::NStorage::TStatement> st = i.BuildStatement(external);
                    if (!st) {
                        TFLEventLog::Error("cannot build request in queries set")("debug_query", i.DebugString());
                        return false;
                    }
                    statements.emplace_back(new NCS::NStorage::TStatement(std::move(*st)));
                }
                std::swap(result, statements);
                return true;
            }

            bool TMultiUpdate::DoToString(const IExternalMethods& external, IOutputStream& os) const {
                if (Records.empty()) {
                    TFLEventLog::Error("no update records");
                    return false;
                }
                if (ConditionFieldIds.empty()) {
                    TFLEventLog::Error("no update records");
                    return false;
                }

                TSet<TString> valuableFieldIds;
                for (auto&& i : Records.front()) {
                    if (!ConditionFieldIds.contains(i.first)) {
                        valuableFieldIds.emplace(i.first);
                    }
                }
                TSet<TString> notNullIds;
                for (auto&& i : Records) {
                    for (auto&& [f, v] : i) {
                        if (!ConditionFieldIds.contains(f) && !valuableFieldIds.contains(f)) {
                            TFLEventLog::Error("incorrect field in record")("field_id", f);
                            return false;
                        }
                        if (!std::get_if<TNull>(&v)) {
                            notNullIds.emplace(f);
                        }
                    }
                    if (valuableFieldIds.size() + ConditionFieldIds.size() != i.size()) {
                        TFLEventLog::Error("incorrect fields in record")("expected_count", valuableFieldIds.size() + ConditionFieldIds.size())("real_count", i.size());
                        return false;
                    }
                }
                TSet<TString> allFieldIds;
                TSet<TString> allFieldNewIds;
                for (auto&& i : Records.front()) {
                    if (!notNullIds.contains(i.first)) {
                        continue;
                    }
                    if (!ConditionFieldIds.contains(i.first)) {
                        allFieldNewIds.emplace(i.first + "_update");
                    } else {
                        allFieldNewIds.emplace(i.first + "_condition");
                    }
                    allFieldIds.emplace(i.first);
                }
                for (auto&& f : allFieldIds) {
                    if (!external.IsValidFieldName(f)) {
                        TFLEventLog::Error("incorrect field name")("field_name", f);
                        return false;
                    }
                }
                os << "UPDATE ";
                if (!TableName.ToString(external, os)) {
                    return false;
                }
                os << " as t SET ";
                {
                    TVector<TString> updatesInfo;
                    for (auto&& key : valuableFieldIds) {
                        if (notNullIds.contains(key)) {
                            updatesInfo.emplace_back(key + " = c." + key + "_update");
                        } else {
                            updatesInfo.emplace_back(key + " = NULL");
                        }
                    }
                    os << JoinSeq(", ", updatesInfo);
                }
                os << " FROM (VALUES ";
                {
                    TVector<TString> inputData;
                    for (auto&& i : Records) {
                        inputData.emplace_back("(" + i.GetValues(external, &allFieldIds) + ")");
                    }
                    os << JoinSeq(", ", inputData);
                    os << ") AS c(" << JoinSeq(", ", allFieldNewIds) << ")";
                }
                os << "WHERE ";
                {
                    TVector<TString> localConditions;
                    for (auto&& k : ConditionFieldIds) {
                        if (notNullIds.contains(k)) {
                            localConditions.emplace_back("t." + k + " = c." + k + "_condition");
                        } else {
                            localConditions.emplace_back("t." + k + " IS NULL ");
                        }
                    }
                    os << JoinSeq(" AND ", localConditions);
                }
                if (GetRecordsSet()) {
                    os << " RETURNING *";
                }
                return true;
            }

        }
    }
}
