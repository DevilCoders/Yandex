package blackbox

import (
	"fmt"
)

type UserAttribute string

type UserDBField string

// Attributes description: https://doc.yandex-team.ru/Passport/AuthDevGuide/concepts/DB_About.html#DB_About__db-attributes
const (
	UserAttributeAccountRegistrationDatetime             UserAttribute = "1"
	UserAttributeAccountHavePayPassword                  UserAttribute = "5"
	UserAttributeAccountIsPddAgreementAccepted           UserAttribute = "6"
	UserAttributeAccountIsPddAdmin                       UserAttribute = "7"
	UserAttributeAccountIsBetatester                     UserAttribute = "8"
	UserAttributeAccountIsCorporate                      UserAttribute = "9"
	UserAttributeAccountIsVip                            UserAttribute = "10"
	UserAttributeAccountIsEmployee                       UserAttribute = "12"
	UserAttributeAccountIsMaillist                       UserAttribute = "13"
	UserAttributePersonContactPhoneNumber                UserAttribute = "15"
	UserAttributePasswordUpdateDatetime                  UserAttribute = "20"
	UserAttributePasswordQuality                         UserAttribute = "21"
	UserAttributePasswordForcedChangingReason            UserAttribute = "22"
	UserAttributePasswordIsCreatingRequired              UserAttribute = "23"
	UserAttributeAccountIsStrongPasswordRequired         UserAttribute = "24"
	UserAttributePersonFirstname                         UserAttribute = "27"
	UserAttributePersonLastname                          UserAttribute = "28"
	UserAttributePersonGender                            UserAttribute = "29"
	UserAttributePersonBirthday                          UserAttribute = "30"
	UserAttributePersonCountry                           UserAttribute = "31"
	UserAttributePersonCity                              UserAttribute = "32"
	UserAttributePersonTimezone                          UserAttribute = "33"
	UserAttributePersonLanguage                          UserAttribute = "34"
	UserAttributePhoneNumber                             UserAttribute = "35"
	UserAttributePhoneConfirmationDatetime               UserAttribute = "36"
	UserAttributeSubscriptionFotki                       UserAttribute = "44"
	UserAttributeSubscriptionCards                       UserAttribute = "45"
	UserAttributeSubscriptionNews                        UserAttribute = "46"
	UserAttributeSubscriptionMoikrug                     UserAttribute = "47"
	UserAttributeSubscriptionDirect                      UserAttribute = "48"
	UserAttributeSubscriptionSpamooborona                UserAttribute = "49"
	UserAttributeSubscriptionBalance                     UserAttribute = "50"
	UserAttributeSubscriptionMoneyLoginRule              UserAttribute = "51"
	UserAttributeSubscription23                          UserAttribute = "52"
	UserAttributeSubscription24                          UserAttribute = "53"
	UserAttributeSubscription25                          UserAttribute = "54"
	UserAttributeSubscription26                          UserAttribute = "55"
	UserAttributeSubscriptionJabberLoginRule             UserAttribute = "56"
	UserAttributeSubscription29                          UserAttribute = "57"
	UserAttributeSubscription30                          UserAttribute = "58"
	UserAttributeSubscription31                          UserAttribute = "59"
	UserAttributeSubscription39                          UserAttribute = "62"
	UserAttributeSubscription40                          UserAttribute = "63"
	UserAttributeSubscription41                          UserAttribute = "64"
	UserAttributeSubscriptionWwwdgtMode                  UserAttribute = "65"
	UserAttributeSubscriptionDiskLoginRule               UserAttribute = "66"
	UserAttributeSubscriptionNarod2LoginRule             UserAttribute = "67"
	UserAttributeSubscription47                          UserAttribute = "68"
	UserAttributeSubscription48                          UserAttribute = "69"
	UserAttributeSubscription49                          UserAttribute = "70"
	UserAttributeSubscription50                          UserAttribute = "71"
	UserAttributeSubscription51                          UserAttribute = "72"
	UserAttributeSubscription52                          UserAttribute = "73"
	UserAttributeSubscription53                          UserAttribute = "74"
	UserAttributeSubscription54                          UserAttribute = "75"
	UserAttributeSubscription55                          UserAttribute = "76"
	UserAttributeSubscription57                          UserAttribute = "77"
	UserAttributeSubscriptionCloudIsActive               UserAttribute = "78"
	UserAttributeSubscription60                          UserAttribute = "79"
	UserAttributeSubscriptionCloudIsPaid                 UserAttribute = "80"
	UserAttributeSubscription76                          UserAttribute = "81"
	UserAttributeSubscriptionCloudUseMobile              UserAttribute = "82"
	UserAttributeSubscription78                          UserAttribute = "83"
	UserAttributeSubscriptionCloudUseDesktop             UserAttribute = "84"
	UserAttributeSubscriptionCloudUseWeb                 UserAttribute = "85"
	UserAttributeSubscription83                          UserAttribute = "89"
	UserAttributeSubscription84                          UserAttribute = "90"
	UserAttributeSubscription85                          UserAttribute = "91"
	UserAttributeSubscription86                          UserAttribute = "92"
	UserAttributeSubscription88                          UserAttribute = "95"
	UserAttributeSubscription90                          UserAttribute = "96"
	UserAttributeSubscription91                          UserAttribute = "97"
	UserAttributeAvatarDefault                           UserAttribute = "98"
	UserAttributeSubscription92                          UserAttribute = "99"
	UserAttributeAccountBrowserKey                       UserAttribute = "101"
	UserAttributeSubscriptionCloudIsMigrant              UserAttribute = "103"
	UserAttributeSubscriptionMailapp2014                 UserAttribute = "105"
	UserAttributePasswordWebOnlyaccountEnableAppPassword UserAttribute = "107"
	UserAttributeSubscriptionVideoSearch                 UserAttribute = "108"
	UserAttributeSubscriptionBugbounty                   UserAttribute = "109"
	UserAttributeSubscriptionAvia                        UserAttribute = "112"
	UserAttributeSubscriptionMailapp2014Androidphone     UserAttribute = "113"
	UserAttributeSubscriptionMailapp2014Androidtablet    UserAttribute = "114"
	UserAttributeSubscriptionMailapp2014Iphone           UserAttribute = "115"
	UserAttributeSubscriptionMailapp2014Ipad             UserAttribute = "116"
	UserAttributeSubscriptionMailapp2014Ipadbirdseye     UserAttribute = "117"
	UserAttributeSubscriptionKinopoisk                   UserAttribute = "119"
	UserAttributeSubscriptionRealty                      UserAttribute = "120"
	UserAttributeSubscriptionRealtyAgent                 UserAttribute = "121"
	UserAttributeSubscriptionMoneyYacardHolder           UserAttribute = "122"
	UserAttributeSubscriptionRabota                      UserAttribute = "124"
	UserAttributeSubscriptionRabotaEmployee              UserAttribute = "125"
	UserAttributeSubscriptionRabotaBlueCollar            UserAttribute = "126"
	UserAttributeSubscriptionRabotaEmployer              UserAttribute = "127"
	UserAttributeSubscriptionRabotaMobileApp             UserAttribute = "128"
	UserAttributeSubscriptionRabotaMobileSite            UserAttribute = "129"
	UserAttributeSubscriptionRealtyModerator             UserAttribute = "130"
	UserAttributeSubscriptionTaxi                        UserAttribute = "131"
	UserAttributeAccountIsShared                         UserAttribute = "132"
	UserAttributeAccountShow2FaPromo                     UserAttribute = "133"
	UserAttributeSubscriptionRealtyAppIos                UserAttribute = "137"
	UserAttributeSubscriptionRealtyAppAndroid            UserAttribute = "138"
	UserAttributeSubscriptionSiteSearch                  UserAttribute = "139"
	UserAttributeSubscriptionToloka                      UserAttribute = "142"
	UserAttributeAccountAudienceOn                       UserAttribute = "146"
	UserAttributeSubscriptionTelephony                   UserAttribute = "148"
	UserAttributeSubscriptionMusicpremium                UserAttribute = "152"
	UserAttributeAccountIsConnectAdmin                   UserAttribute = "154"
	UserAttributeAccountIsMoneyAgreementAccepted         UserAttribute = "157"
	UserAttributeAccountCreatorUID                       UserAttribute = "166"
	UserAttributeAccountExternalOrganizationIds          UserAttribute = "168"
	UserAttributePersonBirthday1001                      UserAttribute = "1001"
	UserAttributePersonBirthyear                         UserAttribute = "1002"
	UserAttributeAccount2FaOn                            UserAttribute = "1003"
	UserAttributeAccountHavePassword                     UserAttribute = "1005"
	UserAttributeAccountHaveHint                         UserAttribute = "1006"
	UserAttributePersonFio                               UserAttribute = "1007"
	UserAttributeAccountNormalizedLogin                  UserAttribute = "1008"
	UserAttributeAccountIsAvailable                      UserAttribute = "1009"
	UserAttributeAccountHaveOrganizationName             UserAttribute = "1011"
	UserAttributeAccountSecurityLevel                    UserAttribute = "1013"
	UserAttributeAccountRfc2FaOn                         UserAttribute = "1014"
	UserAttributeAccountHavePlus                         UserAttribute = "1015"
	UserAttributeAccountKinopoiskOttSubscriptionName     UserAttribute = "1016"
	UserAttributeAccountConnectOrganizationIds           UserAttribute = "1017"
	UserAttributeAccountPlusTrialUsedTS                  UserAttribute = "1018"
	UserAttributeAccountPlusSubscriptionStoppedTS        UserAttribute = "1019"
	UserAttributeAccountPlusSubscriptionExpireTS         UserAttribute = "1020"
	UserAttributeAccountPlusNextChargeTS                 UserAttribute = "1021"
)

func (a UserAttribute) String() string {
	switch a {
	case UserAttributeAccountRegistrationDatetime:
		return "account.registration_datetime"
	case UserAttributeAccountHavePayPassword:
		return "account.have_pay_password"
	case UserAttributeAccountIsPddAgreementAccepted:
		return "account.is_pdd_agreement_accepted"
	case UserAttributeAccountIsPddAdmin:
		return "account.is_pdd_admin"
	case UserAttributeAccountIsBetatester:
		return "account.is_betatester"
	case UserAttributeAccountIsCorporate:
		return "account.is_corporate"
	case UserAttributeAccountIsVip:
		return "account.is_vip"
	case UserAttributeAccountIsEmployee:
		return "account.is_employee"
	case UserAttributeAccountIsMaillist:
		return "account.is_maillist"
	case UserAttributePersonContactPhoneNumber:
		return "person.contact_phone_number"
	case UserAttributePasswordUpdateDatetime:
		return "password.update_datetime"
	case UserAttributePasswordQuality:
		return "password.quality"
	case UserAttributePasswordForcedChangingReason:
		return "password.forced_changing_reason"
	case UserAttributePasswordIsCreatingRequired:
		return "password.is_creating_required"
	case UserAttributeAccountIsStrongPasswordRequired:
		return "account.is_strong_password_required"
	case UserAttributePersonFirstname:
		return "person.firstname"
	case UserAttributePersonLastname:
		return "person.lastname"
	case UserAttributePersonGender:
		return "person.gender"
	case UserAttributePersonBirthday1001:
		return "person.birthday1001"
	case UserAttributePersonCountry:
		return "person.country"
	case UserAttributePersonCity:
		return "person.city"
	case UserAttributePersonTimezone:
		return "person.timezone"
	case UserAttributePersonLanguage:
		return "person.language"
	case UserAttributePhoneNumber:
		return "phone.number"
	case UserAttributePhoneConfirmationDatetime:
		return "phone.confirmation_datetime"
	case UserAttributeSubscriptionFotki:
		return "subscription.fotki"
	case UserAttributeSubscriptionCards:
		return "subscription.cards"
	case UserAttributeSubscriptionNews:
		return "subscription.news"
	case UserAttributeSubscriptionMoikrug:
		return "subscription.moikrug"
	case UserAttributeSubscriptionDirect:
		return "subscription.direct"
	case UserAttributeSubscriptionSpamooborona:
		return "subscription.spamooborona"
	case UserAttributeSubscriptionBalance:
		return "subscription.balance"
	case UserAttributeSubscriptionMoneyLoginRule:
		return "subscription.money.login_rule"
	case UserAttributeSubscription23:
		return "subscription.23"
	case UserAttributeSubscription24:
		return "subscription.24"
	case UserAttributeSubscription25:
		return "subscription.25"
	case UserAttributeSubscription26:
		return "subscription.26"
	case UserAttributeSubscriptionJabberLoginRule:
		return "subscription.jabber.login_rule"
	case UserAttributeSubscription29:
		return "subscription.29"
	case UserAttributeSubscription30:
		return "subscription.30"
	case UserAttributeSubscription31:
		return "subscription.31"
	case UserAttributeSubscription39:
		return "subscription.39"
	case UserAttributeSubscription40:
		return "subscription.40"
	case UserAttributeSubscription41:
		return "subscription.41"
	case UserAttributeSubscriptionWwwdgtMode:
		return "subscription.wwwdgt.mode"
	case UserAttributeSubscriptionDiskLoginRule:
		return "subscription.disk.login_rule"
	case UserAttributeSubscriptionNarod2LoginRule:
		return "subscription.narod2.login_rule"
	case UserAttributeSubscription47:
		return "subscription.47"
	case UserAttributeSubscription48:
		return "subscription.48"
	case UserAttributeSubscription49:
		return "subscription.49"
	case UserAttributeSubscription50:
		return "subscription.50"
	case UserAttributeSubscription51:
		return "subscription.51"
	case UserAttributeSubscription52:
		return "subscription.52"
	case UserAttributeSubscription53:
		return "subscription.53"
	case UserAttributeSubscription54:
		return "subscription.54"
	case UserAttributeSubscription55:
		return "subscription.55"
	case UserAttributeSubscription57:
		return "subscription.57"
	case UserAttributeSubscriptionCloudIsActive:
		return "subscription.cloud.is_active"
	case UserAttributeSubscription60:
		return "subscription.60"
	case UserAttributeSubscriptionCloudIsPaid:
		return "subscription.cloud.is_paid"
	case UserAttributeSubscription76:
		return "subscription.76"
	case UserAttributeSubscriptionCloudUseMobile:
		return "subscription.cloud.use_mobile"
	case UserAttributeSubscription78:
		return "subscription.78"
	case UserAttributeSubscriptionCloudUseDesktop:
		return "subscription.cloud.use_desktop"
	case UserAttributeSubscriptionCloudUseWeb:
		return "subscription.cloud.use_web"
	case UserAttributeSubscription83:
		return "subscription.83"
	case UserAttributeSubscription84:
		return "subscription.84"
	case UserAttributeSubscription85:
		return "subscription.85"
	case UserAttributeSubscription86:
		return "subscription.86"
	case UserAttributeSubscription88:
		return "subscription.88"
	case UserAttributeSubscription90:
		return "subscription.90"
	case UserAttributeSubscription91:
		return "subscription.91"
	case UserAttributeAvatarDefault:
		return "avatar.default"
	case UserAttributeSubscription92:
		return "subscription.92"
	case UserAttributeAccountBrowserKey:
		return "account.browser_key"
	case UserAttributeSubscriptionCloudIsMigrant:
		return "subscription.cloud.is_migrant"
	case UserAttributeSubscriptionMailapp2014:
		return "subscription.mailapp.2014"
	case UserAttributePasswordWebOnlyaccountEnableAppPassword:
		return "password.web_onlyaccount.enable_app_password"
	case UserAttributeSubscriptionVideoSearch:
		return "subscription.video.search"
	case UserAttributeSubscriptionBugbounty:
		return "subscription.bugbounty"
	case UserAttributeSubscriptionAvia:
		return "subscription.avia"
	case UserAttributeSubscriptionMailapp2014Androidphone:
		return "subscription.mailapp2014.androidphone"
	case UserAttributeSubscriptionMailapp2014Androidtablet:
		return "subscription.mailapp2014.androidtablet"
	case UserAttributeSubscriptionMailapp2014Iphone:
		return "subscription.mailapp2014.iphone"
	case UserAttributeSubscriptionMailapp2014Ipad:
		return "subscription.mailapp2014.ipad"
	case UserAttributeSubscriptionMailapp2014Ipadbirdseye:
		return "subscription.mailapp2014.ipadbirdseye"
	case UserAttributeSubscriptionKinopoisk:
		return "subscription.kinopoisk"
	case UserAttributeSubscriptionRealty:
		return "subscription.realty"
	case UserAttributeSubscriptionRealtyAgent:
		return "subscription.realty_agent"
	case UserAttributeSubscriptionMoneyYacardHolder:
		return "subscription.money_yacard_holder"
	case UserAttributeSubscriptionRabota:
		return "subscription.rabota"
	case UserAttributeSubscriptionRabotaEmployee:
		return "subscription.rabota.employee"
	case UserAttributeSubscriptionRabotaBlueCollar:
		return "subscription.rabota.blue_collar"
	case UserAttributeSubscriptionRabotaEmployer:
		return "subscription.rabota.employer"
	case UserAttributeSubscriptionRabotaMobileApp:
		return "subscription.rabota.mobile_app"
	case UserAttributeSubscriptionRabotaMobileSite:
		return "subscription.rabota.mobile_site"
	case UserAttributeSubscriptionRealtyModerator:
		return "subscription.realty_moderator"
	case UserAttributeSubscriptionTaxi:
		return "subscription.taxi"
	case UserAttributeAccountIsShared:
		return "account.is_shared"
	case UserAttributeAccountShow2FaPromo:
		return "account.show_2fa_promo"
	case UserAttributeSubscriptionRealtyAppIos:
		return "subscription.realty.app.ios"
	case UserAttributeSubscriptionRealtyAppAndroid:
		return "subscription.realty.app.android"
	case UserAttributeSubscriptionSiteSearch:
		return "subscription.site_search"
	case UserAttributeSubscriptionToloka:
		return "subscription.toloka"
	case UserAttributeAccountAudienceOn:
		return "account.audience_on"
	case UserAttributeSubscriptionTelephony:
		return "subscription.telephony"
	case UserAttributeSubscriptionMusicpremium:
		return "subscription.musicpremium"
	case UserAttributeAccountIsConnectAdmin:
		return "account.is_connect_admin"
	case UserAttributeAccountIsMoneyAgreementAccepted:
		return "account.is_money_agreement_accepted"
	case UserAttributeAccountCreatorUID:
		return "account.creator_uid"
	case UserAttributeAccountExternalOrganizationIds:
		return "account.external_organization_ids"
	case UserAttributePersonBirthday:
		return "person.birthday"
	case UserAttributePersonBirthyear:
		return "person.birthyear"
	case UserAttributeAccount2FaOn:
		return "account.2fa_on"
	case UserAttributeAccountHavePassword:
		return "account.have_password"
	case UserAttributeAccountHaveHint:
		return "account.have_hint"
	case UserAttributePersonFio:
		return "person.fio"
	case UserAttributeAccountNormalizedLogin:
		return "account.normalized_login"
	case UserAttributeAccountIsAvailable:
		return "account.is_available"
	case UserAttributeAccountHaveOrganizationName:
		return "account.have_organization_name"
	case UserAttributeAccountSecurityLevel:
		return "account.security_level"
	case UserAttributeAccountRfc2FaOn:
		return "account.rfc_2fa_on"
	case UserAttributeAccountHavePlus:
		return "account.have_plus"
	case UserAttributeAccountKinopoiskOttSubscriptionName:
		return "account.kinopoisk_ott_subscription_name"
	case UserAttributeAccountConnectOrganizationIds:
		return "account.connect_organization_ids"
	case UserAttributeAccountPlusTrialUsedTS:
		return "account.plus_trial_used_ts"
	case UserAttributeAccountPlusSubscriptionStoppedTS:
		return "account.plus_subscription_stopped_ts"
	case UserAttributeAccountPlusSubscriptionExpireTS:
		return "account.plus_subscription_expire_ts"
	case UserAttributeAccountPlusNextChargeTS:
		return "account.plus_next_charge_ts"
	default:
		return fmt.Sprintf("unknown_%s", string(a))
	}
}

type PhoneAttribute string

// Phone Attributes description: https://doc.yandex-team.ru/Passport/AuthDevGuide/concepts/DB_About.html#DB_About__section_kbk_b5f_2hb
const (
	PhoneAttributePhoneFormattedNumber       PhoneAttribute = "101"
	PhoneAttributePhoneE164Number            PhoneAttribute = "102"
	PhoneAttributePhoneMaskedFormattedNumber PhoneAttribute = "103"
	PhoneAttributePhoneMaskedE164Number      PhoneAttribute = "104"
	PhoneAttributePhoneIsConfirmed           PhoneAttribute = "105"
	PhoneAttributePhoneIsBound               PhoneAttribute = "106"
	PhoneAttributePhoneIsDefault             PhoneAttribute = "107"
	PhoneAttributePhoneIsSecured             PhoneAttribute = "108"
)

func (a PhoneAttribute) String() string {
	switch a {
	case PhoneAttributePhoneFormattedNumber:
		return "phone.formatted_number"
	case PhoneAttributePhoneE164Number:
		return "phone.e164_number"
	case PhoneAttributePhoneMaskedFormattedNumber:
		return "phone.masked_formatted_number"
	case PhoneAttributePhoneMaskedE164Number:
		return "phone.masked_e164_number"
	case PhoneAttributePhoneIsConfirmed:
		return "phone.is_confirmed"
	case PhoneAttributePhoneIsBound:
		return "phone.is_bound"
	case PhoneAttributePhoneIsDefault:
		return "phone.is_default"
	case PhoneAttributePhoneIsSecured:
		return "phone.is_secured"
	default:
		return fmt.Sprintf("unknown_%s", string(a))
	}
}
