#pragma once

#include <util/datetime/base.h>
#include <util/generic/fwd.h>
#include <util/generic/maybe.h>
#include <util/generic/ptr.h>
#include <util/generic/yexception.h>

// Occurs when server's answer code is 400
class TGetOauthTokenApiException
   : public yexception {
};

/**
 * @brief Interface to provide custom retry policy
 */
class IOauthTokenRetryPolicy : public TThrRefBase {
public:
    virtual ~IOauthTokenRetryPolicy() = default;

    /**
     * @brief Returns the delay time before the next retry. Method itself should control the number of retries.
     *
     * @return Duration of delay before the next retry or Nothing() if no more retries are expected.
     */
    virtual TMaybe<TDuration> GetNextDelay() = 0;

    /**
     * @brief Returns an instance of the class to its initial state.
     */
    virtual void Reset() = 0;
};

/**
 * @brief Default retry policy
 */
class TOauthTokenSquareRP : public IOauthTokenRetryPolicy {
public:
    TOauthTokenSquareRP(ui32 retriesNum);

    /**
     * @brief Each delay depends on square of retry number such as
     *        delay = 1 + random(0, (retryNumber + 2)^2)
     */
    TMaybe<TDuration> GetNextDelay() override;
    void Reset() override;

private:
    ui32 RetriesNum_;
    ui32 Current_ = 1;
};

struct TOauthTokenConnectOpts {
    TDuration SocketTimeout = TDuration::Seconds(5);
    TDuration ConnectTimeout = TDuration::Seconds(30);
    TIntrusivePtr<IOauthTokenRetryPolicy> RetryPolicy = MakeIntrusive<TOauthTokenSquareRP>(3U);
};

/**
 * @brief Params builder for getting defined user's oauth token from app defined by clientId and clientSecret
 *
 * Information about OAuth in yandex can be found here
 * https://doc.yandex-team.ru/oauth/dg-internal/concepts/about.html
 *
 * To create your own app use https://oauth.yandex-team.ru
 *
 * @param clientId     ID of app from https://oauth.yandex-team.ru
 * @param clientSecret Password of app from https://oauth.yandex-team.ru
*/
struct TOauthTokenParams {
    const TString ClientId;
    const TString ClientSecret;
    TMaybe<TString> Login = Nothing();
    TMaybe<TString> KeyPath = Nothing();
    TMaybe<TString> Password = Nothing();

    TOauthTokenParams(TStringBuf clientId, TStringBuf clientSecret);
    /**
    * @param keyPath    Path to a private key of user. If not set, it will try to get key from: ssh forwarding agent, standard rsa key (in that order)
    */
    TOauthTokenParams& SetKeyPath(TStringBuf keyPath);
    /**
    * @param login      Username to ask oauth token for. If not set, system username will be used.
    */
    TOauthTokenParams& SetLogin(TStringBuf login);
    /**
    * Force login-password authentication
    *
    * @atteintion         It really is not a good idea to use login and password pair in production
    *
    * @param login        Username to ask oauth token for.
    * @param password     Password for defined user.
    */
    TOauthTokenParams& SetPasswordAuth(TStringBuf login, TStringBuf password);
};

TString GetOauthToken(
    const TOauthTokenParams& params,
    TOauthTokenConnectOpts connectionOpt = TOauthTokenConnectOpts()
);
