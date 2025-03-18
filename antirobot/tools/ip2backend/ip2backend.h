#pragma once
#include <antirobot/lib/host_addr.h>
#include <antirobot/lib/http_request.h>

#include <util/generic/string.h>

namespace NAntiRobot {

/**
 * @return the name of antirobot backend which processes request from the given IP-address
 * @param ipAddress A string representation of IPv4 of IPv6
 * @throw yexception in case of failure or if the address given in @arg ipAddress
 *        is not a valid IPv4 or IPv6
 */
TString Ip2Backend(const THostAddr& antirobotService, const THttpRequest& request,
                  int sendCount);

/**
 * @return the request string to get backend which processes request from the given IP-address
 * parameteres are the same witch Ip2Backend
 */
THttpRequest MakeRequest(const TString& ipAddress, const THostAddr& antirobotService,
                         int processorCount);

}
