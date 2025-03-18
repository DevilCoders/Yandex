#include "expand_params.h"

TVector<TVector<TString> > ExpandParams(
        const TVector<TString> firsts,
        const TVector<TVector<TString> >& paramss,
        const TMasterListManager* listManager)
{
    TVector<TVector<TString> > expandedParams;
    TVector<TVector<TString> > tempExpandedParams;

    for (TVector<TString>::const_iterator first = firsts.begin(); first != firsts.end(); ++first) {
        expandedParams.emplace_back();
        expandedParams.back().push_back(*first);
    }

    for (TVector<TVector<TString> >::const_iterator params = paramss.begin() + 1; params != paramss.end(); ++ params) {
        for (TVector<TVector<TString> >::const_iterator expandedParam = expandedParams.begin();
                expandedParam != expandedParams.end(); ++expandedParam)
        {
            for (TVector<TString>::const_iterator param = params->begin(); param != params->end(); ++param) {
                TVector<TString> lastParamExpanded;
                if (param->StartsWith('!')) {
                    if (params->size() != 1) {
                        ythrow yexception() << "only one list entry is allowed in cluster list";
                    }

                    listManager->GetList(param->substr(1, TString::npos), *expandedParam, lastParamExpanded);
                } else {
                    lastParamExpanded.push_back(*param);
                }

                for (TVector<TString>::const_iterator lastParam = lastParamExpanded.begin();
                        lastParam != lastParamExpanded.end(); ++lastParam)
                {
                    tempExpandedParams.push_back(*expandedParam);
                    tempExpandedParams.back().push_back(*lastParam);
                }
            }
        }

        tempExpandedParams.swap(expandedParams);
        tempExpandedParams.clear();
    }

    return expandedParams;
}
