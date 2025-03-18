#include "worker_variables.h"

#include "messages.h"

#include <util/charset/unidata.h>
#include <util/stream/file.h>

bool TWorkerVariablesLight::Update(const TVariablesMessage& message) {
    bool changed = false;

    for (TVariablesMessage::TRepeatedVariable::const_iterator i = message.GetVariable().begin(); i != message.GetVariable().end(); ++i) {
        if (i->HasValue())
            changed = Set(TVar(i->GetName(), i->GetValue())) || changed;
        else
            changed = Del(i->GetName()) || changed;
    }

    return changed;
}

bool TWorkerVariables::Update(const TVariablesMessage& message) {
    bool changed = false;

    for (TVariablesMessage::TRepeatedVariable::const_iterator i = message.GetVariable().begin(); i != message.GetVariable().end(); ++i) {
        if (i->HasValue())
            changed = Set(TVar(i->GetName(), i->GetValue(), GetLastRevision())) || changed;
        else
            changed = Del(i->GetName()) || changed;
    }

    return changed;
}

void TWorkerVariables::ApplyDefaultVariable(const TString& name, const TString& value) {
    Map.insert_unique(TVar(name, value, GetLastRevision()));
}

void TWorkerVariables::ApplyStrongVariable(const TString& name, const TString& value) {
    const TVar variable(name, value, GetLastRevision());
    Map.find_or_insert(variable) = variable;
}

void TWorkerVariables::SerializeToArcadiaStream(IOutputStream* out) const {
    TVariablesMessage(*this).SerializeToArcadiaStream(out);
}

void TWorkerVariables::ParseFromArcadiaStream(IInputStream* in) {
    TVariablesMessage message;
    message.ParseFromArcadiaStream(in);
    Clear();
    Update(message);
}

TWorkerVariables::TVar::TRevision TWorkerVariables::Substitute(TString& s) const {
    TVar::TRevision ret = 0;

    size_t dollar = TString::npos;

    for (size_t i = 0;; ++i) {
        if (dollar != TString::npos && (i == s.length() || (s[i] != '_' && !IsAlnum(s[i])))) {
            if (i > dollar + 1) {
                TVar variable;
                TString(s, dollar + 1, i - dollar - 1).swap(variable.Name);

                if (!Get(variable))
                    throw TNoVariable() << variable.Name;

                s.replace(dollar, i - dollar, variable.Value);
                ret = Max(ret, variable.Revision);

                i = dollar + variable.Value.length() - 1;
            }

            dollar = TString::npos;
        }

        if (i == s.length())
            break;

        if (i <= s.size() && s[i] == '$')
            dollar = i;
    }

    return ret;
}

size_t SubstituteNumericArg(TString& s, size_t n, const TString& to) {
    TString from('\\');
    from += ToString<size_t>(n);

    size_t ret = 0;

    for (size_t off = 0; (off = s.find(from, off)) != TString::npos;) {
        if (off + from.length() == s.length() || !IsDigit(s[off + from.length()])) {
            s.replace(off, from.length(), to);
            off += to.length();
            ++ret;
        } else
            off += from.length();
    }

    return ret;
}


template <>
void Out<NPrivate::TVariableWithRevision>(
        IOutputStream& os,
        TTypeTraits<NPrivate::TVariableWithRevision>::TFuncParam var)
{
    os << var.Name << " -> " << var.Value << " (" << var.Revision << ")";
}
