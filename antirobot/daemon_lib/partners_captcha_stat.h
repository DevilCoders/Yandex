#pragma once

#include "stat.h"

#include <atomic>


namespace NAntiRobot {


enum class EPartnersCaptchaCounter {
    CorrectInputs    /* "partners.captcha_correct_inputs" */,
    ImageShows       /* "partners.captcha_image_shows" */,
    IncorrectInputs  /* "partners.captcha_incorrect_inputs" */,
    Redirects        /* "partners.captcha_redirects" */,
    Shows            /* "partners.captcha_shows" */,
    Count
};

using TPartnersCaptchaStat = TCategorizedStats<
    std::atomic<size_t>, EPartnersCaptchaCounter
>;


} // namespace NAntiRobot
