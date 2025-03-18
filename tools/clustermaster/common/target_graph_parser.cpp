#include "target_graph_parser.h"

#include "config_lexer.h"
#include "depend.h"

#include <util/generic/bt_exception.h>

static void ParseRangeList(const TTokenStack &stack, int &pos, TVector<TString> &output) {
    while(1) {
        if (!stack.Is(pos, TT_IDENTIFIER))
            break;

        TString cur = stack.Get(pos, TT_IDENTIFIER);

        size_t splitter;
        if ((splitter = cur.find("..")) != TString::npos) {
            // Item range
            TString first = cur.substr(0, splitter);
            TString last = cur.substr(splitter + 2, TString::npos);

            if (!ParseRange(first, last, output)) {
                ythrow yexception() << "invalid range: " << cur;
                // or should we just treat is as a simple item?
                //output.push_back(cur);
            }
        } else {
            // Simple item
            output.push_back(cur);
        }

        if (!stack.Is(pos+1, TT_LISTDIVIDER)) {
            pos++;
            break;
        }
        pos += 2;
    }
}


static TVector<std::pair<TString, TString> > ParseOptions(const TTokenStack& stack, ui32 pos) {
    TVector<std::pair<TString, TString> > r;

    // Parse options
    while (stack.Has(pos)) {
        TString option = stack.Get(pos++, TT_OPTION);

        int eqpos = option.find('=');
        Y_VERIFY(eqpos, "must be equal sign");

        TString variable = option.substr(0, eqpos);
        TString quotedValue = option.substr(eqpos + 1, TString::npos);

        if (quotedValue[0] == '"')
            quotedValue = quotedValue.substr(1, quotedValue.length() - 2);

        TString value;
        size_t left = 0;
        size_t right;
        while(1) {
            right = quotedValue.find('\\', left);
            if (right == TString::npos || right > left)
                value += quotedValue.substr(left, right - left);
            if (right == TString::npos)
                break;
            ++right;
            Y_VERIFY(right < quotedValue.length(), "%s", (TString("Unclosed quote while parsing `") + option + "'").data());
            value += quotedValue[right];
            left = right + 1;
            if (left >= quotedValue.length())
                break;
        }

        r.push_back(std::pair<TString, TString>(variable, value));
    }

    return r;
}


TTargetTypeParsed TTargetTypeParsed::Parse(const TTokenStack& stack) {
    TTargetTypeParsed r;

    r.Name = stack.Get(0, TT_IDENTIFIER);

    Y_VERIFY(stack.Is(1, TT_TARGETTYPEDEF), "does not compute");

    TVector<TString> hosts, clusters;

    int pos = 2;

    for (;;) {
        TVector<TString> params;
        ParseRangeList(stack, pos, params);
        if (params.empty())
            break;
        r.Paramss.push_back(params);
    }

    if (r.Paramss.empty()) {
        ythrow yexception() << "must at least have hosts";
    }

    r.Options = ParseOptions(stack, pos);

    return r;
}


TString TTargetParsed::TDepend::ToString() const {
    return Name.empty() ? TString("^") : Name;
}

TTargetParsed TTargetParsed::Parse(const TTokenStack& stack) {
    TTargetParsed r;

    r.Type = stack.Get(0);
    r.Name = stack.Get(1);

    int pos = 2;
    if (stack.Is(2, TT_EXPLDEPENDS)) {
        // Explicit depends
        pos = 3;
        while (stack.Has(pos)) {
            TDepend depend;

            if (stack.Is(pos, TT_DEPENDONPREV)) {

            } else if (stack.Is(pos, TT_IDENTIFIER)) {
                depend.Name = stack.Get(pos, TT_IDENTIFIER);
            } else {
                // no more dependency specifiers
                break;
            }

            pos++;
            for (; stack.Has(pos); pos++) {
                if (stack.Is(pos, TT_DEPENDMAPPING)) {
                    depend.ParamMappings = ParseParamMappings(stack.Get(pos, TT_DEPENDMAPPING));
                } else if (stack.Is(pos, TT_GROUPDEPEND)) {
                    depend.Flags |= DF_GROUP_DEPEND;
                } else if (stack.Is(pos, TT_SEMAPHORE)) {
                    depend.Flags |= DF_SEMAPHORE;
                } else if (stack.Is(pos, TT_NONRECURSIVE)) {
                    depend.Flags |= DF_NON_RECURSIVE;
                } else if (stack.Is(pos, TT_CONDEPEND)) {
                    if (!depend.Condition.empty())
                        ythrow yexception() << "multiple conditions for target '" << depend.ToString() << "'";

                    depend.Condition = stack.Get(pos, TT_CONDEPEND).substr(1, TString::npos);
                } else {
                    break;
                }
            }

            r.Depends.push_back(depend);
        }
    } else {
        // Implicit depends

        TDepend depend;

        for (; stack.Has(pos); pos++) {
            // TODO: copypaste from 20 lines above
            if (stack.Is(pos, TT_DEPENDMAPPING)) {
                depend.ParamMappings = ParseParamMappings(stack.Get(pos, TT_DEPENDMAPPING));
            } else if (stack.Is(pos, TT_GROUPDEPEND)) {
                depend.Flags |= DF_GROUP_DEPEND;
            } else if (stack.Is(pos, TT_SEMAPHORE)) {
                depend.Flags |= DF_SEMAPHORE;
            } else if (stack.Is(pos, TT_NONRECURSIVE)) {
                depend.Flags |= DF_NON_RECURSIVE;
            } else {
                break;
            }
        }

        r.Depends.push_back(depend);
    }

    r.Options = ParseOptions(stack, pos);
    return r;
}

TVariableParsed TVariableParsed::Parse(const TTokenStack& stack) {
    TVariableParsed r;

    if (stack.Is(1, TT_DEFVAR)) {
        r.Strong = false;
    } else if (stack.Is(1, TT_STRONGVAR)) {
        r.Strong = true;
    } else {
        ythrow TWithBackTrace<yexception>() << "unexpected token";
    }
    r.Name = stack.Get(0, TT_IDENTIFIER);
    r.Value = stack.Get(2, TT_IDENTIFIER);
    return r;
}


TTargetGraphParsed TTargetGraphParsed::Parse(TStringBuf stringBuf) {
    TTargetGraphParsed r;
    TIntermediate intermediate = TIntermediate::Parse(stringBuf);
    for (TVector<TTokenStack>::const_iterator stack = intermediate.Stacks.begin();
            stack != intermediate.Stacks.end(); ++stack)
    {
        if (stack->Empty())
            continue;
        r.Nodes.push_back(ParseNode(*stack));
    }
    r.Shebang = intermediate.Shebang;
    return r;
}

TTargetGraphNodeParsed TTargetGraphParsed::ParseNode(const TTokenStack& stack) {
    if (stack.Is(1, TT_DEFVAR) || stack.Is(1, TT_STRONGVAR)) {
        return TVariableParsed::Parse(stack);
    } else if (stack.Is(1, TT_TARGETTYPEDEF)) {
        return TTargetTypeParsed::Parse(stack);
    } else {
        return TTargetParsed::Parse(stack);
    }
}

template <typename T>
TVector<T> TTargetGraphParsed::GetNodesOfType() const {
    TVector<T> r;
    for (TVector<TTargetGraphNodeParsed>::const_iterator node = Nodes.begin();
            node != Nodes.end(); ++node)
    {
        if (node->Compatible<T>()) {
            r.push_back(node->Cast<T>());
        }
    }
    return r;
}

TVector<TTargetTypeParsed> TTargetGraphParsed::GetTypes() const {
    return GetNodesOfType<TTargetTypeParsed>();
}

TVector<TTargetParsed> TTargetGraphParsed::GetTargets() const {
    return GetNodesOfType<TTargetParsed>();
}
