#pragma once
#include <antirobot/lib/antirobot_response.h>

namespace NAntiRobot {

struct TCaptchaPageParams;
class TRequest;
class TSpravka;

class TCaptchaFormat {
public:
    virtual ~TCaptchaFormat() = default;

    /**
     * Формирует HTTP-ответ, содержащий страницу капчи
     * @param[in] params параметры страницы капчи
     * @return HTTP-ответ, содержащий страницу капчи
     */
    virtual TResponse GenCaptchaResponse(TCaptchaPageParams& params) const = 0;
    virtual TResponse CaptchaSuccessReply(const TRequest& req, const TSpravka& spravka) const = 0;
};

const TCaptchaFormat& CaptchaFormat(const TRequest& req);

void EnableCrossOriginResourceSharing(const TRequest& req, TResponse& response, bool allowCredentials);

} /* namespace NAntiRobot */
