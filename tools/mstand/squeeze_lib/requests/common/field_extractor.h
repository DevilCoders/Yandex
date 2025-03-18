#pragma once

#include <quality/user_sessions/request_aggregate_lib/all.h>
#include <library/cpp/cgiparam/cgiparam.h>


bool GetHasMisspell(const NRA::TRequest* request);
bool GetIsPermissionRequested(const NRA::TRequest* request);
NYT::TNode GetRearrValues(const NRA::TNameValueMap& rearrValues);
NYT::TNode GetRelevValues(const NRA::TNameValueMap& relevValues);
NYT::TNode GetUserHistory(const NRA::TNameValueMap& searchProps);
NYT::TNode PrepareSuggestData(const NRA::TRequest* request);
TMaybe<TString> GetDomRegion(const NRA::TRequest* request);
TString GetRefererName(const TString& referer, const TCgiParameters& cgi);
NYT::TNode GetSearchProps(const NRA::TNameValueMap& searchProps);