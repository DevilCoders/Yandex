#pragma once

#include <util/generic/string.h>

class TInstant;

/**
 * Generate random reqid with given class using hostname. Generic reqid format is the following:
 * <timestamp>-<hash:some-digits>-<short-hostname>[-<reqid-class>][-p<page>][-REASK]
 *
 * Examples:
 * 1372335902187211-1468110155885825366135491-mmeta18-00
 * 1372693715306836-2175494974-margarita-RXML
 *
 * Timestamp is a integral number (in microseconds).
 * Reqid class is an [A-Z]+ sequence, e.g. "RXML".
 *
 * @param[in] reqIdClass optional reqId class name
 * @return Generated reqid
 **/
TString ReqIdGenerate(const char* reqIdClass = nullptr);

/**
 * Parse timestamp and reqid class from reqid
 * @param[in] reqId reqid to parse
 * @param[out] reqIdTimestamp parsed timestamp (or 0 on parse error)
 * @param[out] reqIdClass parsed reqid class (or empty string on parse error or abscence)
 **/
void ReqIdParse(const TString& reqId, TInstant& reqIdTimestamp, TString& reqIdClass);

/**
 * Returns short hostname used in reqid
 *
 * @return Shortened hostname
 **/
const TString& ReqIdHostSuffix();

/**
 * Checks whether ReqId is in valid format
 * @param[in] reqId to validate
 * @return true if reqId matches reqid format, false otherwise
 **/
bool ValidateReqId(TStringBuf reqId);
