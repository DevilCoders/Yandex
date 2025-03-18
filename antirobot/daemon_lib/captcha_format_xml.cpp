#include "captcha_format_xml.h"

#include "captcha_page_params.h"

#include <antirobot/lib/spravka.h>

#include <library/cpp/archive/yarchive.h>
#include <library/cpp/mime/types/mime.h>

namespace NAntiRobot {

TXmlCaptchaFormat::TXmlCaptchaFormat(const TArchiveReader& reader)
    : Page(reader.ObjectByKey("/captcha_partner.xml")->ReadAll(), "ru")
{
}

TResponse TXmlCaptchaFormat::GenCaptchaResponse(TCaptchaPageParams& params) const
{
    return TResponse::ToUser(params.HttpCode).SetContent(Page.Gen(params), strByMime(MIME_XML));
}

TResponse TXmlCaptchaFormat::CaptchaSuccessReply(const TRequest&,
                                                 const TSpravka& spravka) const
{
    return TResponse::ToUser(HTTP_OK).AddHeader("Set-Cookie", spravka.AsCookie());
}

} // namespace NAntiRobot
