package blackbox

const (
	// EmailsUnspecified was ignored.
	EmailsUnspecified EmailsRequest = ""
	// EmailsGetAll requests all known user e-mails.
	EmailsGetAll EmailsRequest = "getall"
	// EmailsGetAll requests internal user e-mails.
	EmailsGetYandex EmailsRequest = "getyandex"
	// EmailsGetDefault requests default user e-mail.
	EmailsGetDefault EmailsRequest = "getdefault"
	// EmailTestOne instructs BlackBox to return email from AddrToTest if this address belongs to the account
	// and empty list otherwise.
	EmailTestOne EmailsRequest = "testone"

	// GetPhonesUnspecified will not request phones
	GetPhonesUnspecified GetPhonesRequest = ""
	// GetPhonesAll requests all known user phones
	GetPhonesAll GetPhonesRequest = "all"
	// GetPhonesBound requests only bound user phones
	GetPhonesBound GetPhonesRequest = "bound"
)

type (
	EmailsRequest string

	GetPhonesRequest string

	SessionIDRequest struct {
		/*
			Not supported fields so far:
			  - getemails
			  - email_attributes
		*/

		// Session_id Cookie.
		// Documentation: https://docs.yandex-team.ru/blackbox/methods/sessionid#request_format
		//
		// Required to process request
		SessionID string

		// User IP Address.
		// Documentation: https://docs.yandex-team.ru/blackbox/methods/sessionid#request_format
		//
		// Required to process request
		UserIP string

		// Host address (e.g. “yandex.ru” or “volozh.ya.ru”).
		// Documentation: https://docs.yandex-team.ru/blackbox/methods/sessionid#request_format
		//
		// Required to process request
		Host string

		// Target user UID (otherwise will be used default uid from session cookie).
		// Documentation: https://docs.yandex-team.ru/blackbox/methods/sessionid#request_format
		DefaultUID ID

		// Requests additional user attributes.
		// Documentation: https://docs.yandex-team.ru/blackbox/methods/sessionid#request_format
		Attributes []UserAttribute

		// Requests user dbfields.
		// Documentation: https://docs.yandex-team.ru/blackbox/methods/sessionid#request_format
		DBFields []UserDBField

		// Requests user login aliases. Supported only list of needed aliases.
		// Documentation: https://docs.yandex-team.ru/blackbox/methods/sessionid#request_format
		Aliases []UserAlias

		// Requests user emails.
		// Documentation: https://docs.yandex-team.ru/blackbox/methods/sessionid#request_format
		Emails EmailsRequest

		// Requests user phones
		// Documentation: https://docs.yandex-team.ru/blackbox/methods/sessionid#request_format
		GetPhones GetPhonesRequest

		// Requests phone attributes
		// Documentation: https://docs.yandex-team.ru/blackbox/methods/sessionid#request_format
		PhoneAttributes []PhoneAttribute

		// Email that is checked for belonging to the user.
		// Documentation: https://docs.yandex-team.ru/blackbox/methods/sessionid#request_format
		//
		// Required if .Emails is EmailTestOne
		AddrToTest string

		// Requests information about cookie accounts (including blocked and logouted).
		// Documentation: https://docs.yandex-team.ru/blackbox/methods/sessionid#request_format
		FullInfo bool

		// Requests user DisplayName.
		// Documentation: https://docs.yandex-team.ru/blackbox/methods/sessionid#request_format
		RegName bool

		// Requests public user name.
		GetPublicName bool

		// Requests public user id.
		// Documentation: https://wiki.yandex-team.ru/passport/blackbox/#25.11.2019publicid
		GetPublicID bool

		// Requests user ticket.
		// Documentation: https://docs.yandex-team.ru/blackbox/methods/sessionid#request_format
		//
		// Warning: works only with TVM
		GetUserTicket bool

		// Request family id
		// Documentation: https://wiki.yandex-team.ru/passport/blackbox/#14.02.2020getfamilyinfoyes
		GetFamilyInfo bool

		// Optional parameter for logging, will be saved in the statbox log in the field idnt.
		// Documentation: https://docs.yandex-team.ru/blackbox/methods/sessionid#request_format
		StatboxID string

		// Optional parameter for logging, will be saved in the statbox log in the field yuid.
		// Documentation: https://docs.yandex-team.ru/blackbox/methods/sessionid#request_format
		YandexUID string
	}

	MultiSessionIDRequest struct {
		/*
			Not supported fields so far:
			  - getphones
			  - phone_attributes
			  - getemails
			  - email_attributes
		*/

		// Session_id Cookie.
		// Documentation: https://docs.yandex-team.ru/blackbox/methods/sessionid#request_format
		//
		// Required to process request
		SessionID string

		// User IP Address.
		// Documentation: https://docs.yandex-team.ru/blackbox/methods/sessionid#request_format
		//
		// Required to process request
		UserIP string

		// Host address (e.g. “yandex.ru” or “volozh.ya.ru”).
		// Documentation: https://docs.yandex-team.ru/blackbox/methods/sessionid#request_format
		//
		// Required to process request
		Host string

		// Target user UID (otherwise will be used default uid from session cookie).
		// Documentation: https://docs.yandex-team.ru/blackbox/methods/sessionid#request_format
		DefaultUID ID

		// Requests additional user attributes.
		// Documentation: https://docs.yandex-team.ru/blackbox/methods/sessionid#request_format
		Attributes []UserAttribute

		// Requests user dbfields.
		// Documentation: https://docs.yandex-team.ru/blackbox/methods/sessionid#request_format
		DBFields []UserDBField

		// Requests user login aliases. Supported only list of needed aliases.
		// Documentation: https://docs.yandex-team.ru/blackbox/methods/sessionid#request_format
		Aliases []UserAlias

		// Requests user emails.
		// Documentation: https://docs.yandex-team.ru/blackbox/methods/sessionid#request_format
		Emails EmailsRequest

		// Email that is checked for belonging to the user.
		// Documentation: https://docs.yandex-team.ru/blackbox/methods/sessionid#request_format
		//
		// Required if .Emails is EmailTestOne
		AddrToTest string

		// Requests information about cookie accounts (including blocked and logouted).
		// Documentation: https://docs.yandex-team.ru/blackbox/methods/sessionid#request_format
		FullInfo bool

		// Requests user DisplayName.
		// Documentation: https://docs.yandex-team.ru/blackbox/methods/sessionid#request_format
		RegName bool

		// Requests public user name.
		GetPublicName bool

		// Requests public user id.
		// Documentation: https://wiki.yandex-team.ru/passport/blackbox/#25.11.2019publicid
		GetPublicID bool

		// Requests user ticket.
		// Documentation: https://docs.yandex-team.ru/blackbox/methods/sessionid#request_format
		//
		// Warning: works only with TVM
		GetUserTicket bool

		// Request family id
		// Documentation: https://wiki.yandex-team.ru/passport/blackbox/#14.02.2020getfamilyinfoyes
		GetFamilyInfo bool

		// Optional parameter for logging, will be saved in the statbox log in the field idnt.
		// Documentation: https://docs.yandex-team.ru/blackbox/methods/sessionid#request_format
		StatboxID string

		// Optional parameter for logging, will be saved in the statbox log in the field yuid.
		// Documentation: https://docs.yandex-team.ru/blackbox/methods/sessionid#request_format
		YandexUID string
	}

	OAuthRequest struct {
		/*
			Not supported fields so far:
			  - getphones
			  - phone_attributes
			  - getemails
			  - email_attributes
		*/

		// OAuth-token to check.
		// Documentation: https://docs.yandex-team.ru/blackbox/methods/oauth#request_format
		//
		// Required to process request
		OAuthToken string

		// User IP Address.
		// Documentation: https://docs.yandex-team.ru/blackbox/methods/oauth#request_format
		//
		// Required to process request
		UserIP string

		// Requests additional user attributes.
		// Documentation: https://docs.yandex-team.ru/blackbox/methods/oauth#request_format
		Attributes []UserAttribute

		// Requests user dbfields.
		// Documentation: https://docs.yandex-team.ru/blackbox/methods/oauth#request_format
		DBFields []UserDBField

		// Requests user login aliases. Supported only list of needed aliases.
		// Documentation: https://docs.yandex-team.ru/blackbox/methods/oauth#request_format
		Aliases []UserAlias

		// List of required scopes. E.g. []string{"login:avatar", "login:birthday"}
		// Documentation: https://docs.yandex-team.ru/blackbox/methods/oauth#request_format
		Scopes []string

		// Requests user emails.
		// Documentation: https://docs.yandex-team.ru/blackbox/methods/oauth#request_format
		Emails EmailsRequest

		// Email that is checked for belonging to the user.
		// Documentation: https://docs.yandex-team.ru/blackbox/methods/oauth#request_format
		//
		// Required if .Emails is EmailTestOne
		AddrToTest string

		// Requests user ticket.
		// Documentation: https://docs.yandex-team.ru/blackbox/methods/oauth#request_format
		//
		// Warning: works only with TVM
		GetUserTicket bool

		// Requests user DisplayName.
		// Documentation: https://docs.yandex-team.ru/blackbox/methods/oauth#request_format
		RegName bool

		// Requests public user name.
		GetPublicName bool

		// Requests public user id.
		// Documentation: https://wiki.yandex-team.ru/passport/blackbox/#25.11.2019publicid
		GetPublicID bool

		// Request family id
		// Documentation: https://wiki.yandex-team.ru/passport/blackbox/#14.02.2020getfamilyinfoyes
		GetFamilyInfo bool

		// Request login id
		// Documentation: https://wiki.yandex-team.ru/passport/blackbox/#20.12.2019getloginid
		GetLoginID bool
	}

	UserInfoRequest struct {
		/*
			Not supported fields so far:
			  - suid
			  - sid
			  - pintotest
			  - getemails
			  - email_attributes
		*/

		// UIDs of the users you want to get info about.
		// Documentation: https://docs.yandex-team.ru/blackbox/methods/userinfo#request_format
		UIDs []ID

		// User login you want to get info about.
		// Documentation: https://docs.yandex-team.ru/blackbox/methods/userinfo#request_format
		Login string

		// User public id you want to get info about.
		// Documentation: TBD
		PublicID string

		// User IP Address.
		// Documentation: https://docs.yandex-team.ru/blackbox/methods/userinfo#request_format
		//
		// Required to process request
		UserIP string

		// Requests additional user attributes.
		// Documentation: https://docs.yandex-team.ru/blackbox/methods/userinfo#request_format
		Attributes []UserAttribute

		// Requests user dbfields.
		// Documentation: https://docs.yandex-team.ru/blackbox/methods/userinfo#request_format
		DBFields []UserDBField

		// Requests DisplayName.
		// Documentation: https://docs.yandex-team.ru/blackbox/methods/userinfo#request_format
		RegName bool

		// Requests user phones
		// Documentation: https://docs.yandex-team.ru/blackbox/methods/sessionid#request_format
		GetPhones GetPhonesRequest

		// Requests phone attributes
		// Documentation: https://docs.yandex-team.ru/blackbox/methods/sessionid#request_format
		PhoneAttributes []PhoneAttribute

		// Requests public user name.
		GetPublicName bool

		// Requests public user id.
		// Documentation: https://wiki.yandex-team.ru/passport/blackbox/#25.11.2019publicid
		GetPublicID bool

		// Request family id
		// Documentation: https://wiki.yandex-team.ru/passport/blackbox/#14.02.2020getfamilyinfoyes
		GetFamilyInfo bool

		// Requests user login aliases. Supported only list of needed aliases.
		// Documentation: https://docs.yandex-team.ru/blackbox/methods/userinfo#request_format
		Aliases []UserAlias

		// Requests user emails.
		// Documentation: https://docs.yandex-team.ru/blackbox/methods/userinfo#request_format
		Emails EmailsRequest

		// Email that is checked for belonging to the user.
		// Documentation: https://docs.yandex-team.ru/blackbox/methods/userinfo#request_format
		//
		// Required if .Emails is EmailTestOne
		AddrToTest string
	}

	UserTicketRequest struct {
		/*
			Not supported fields so far:
			  - dbfields
			  - pintotest
			  - getphones
			  - phone_attributes
			  - getemails
			  - email_attributes
		*/

		// UserTicket received when using the methods login, sessionid or oauth with a parameter get_user_ticket.
		// Documentation: https://docs.yandex-team.ru/blackbox/methods/user_ticket#request_format
		UserTicket string

		// UIDs of the users you want to get info about.
		// Documentation: https://docs.yandex-team.ru/blackbox/methods/user_ticket#request_format
		UIDs []ID

		// Multi indicates whether to request information about multiple users.
		// Documentation: https://docs.yandex-team.ru/blackbox/methods/user_ticket#request_format
		Multi bool

		// Requests DisplayName.
		// Documentation: https://docs.yandex-team.ru/blackbox/methods/user_ticket#request_format
		RegName bool

		// Requests public user name.
		GetPublicName bool

		// Requests public user id.
		// Documentation: https://wiki.yandex-team.ru/passport/blackbox/#25.11.2019publicid
		GetPublicID bool

		// Requests additional user attributes.
		// Documentation: https://docs.yandex-team.ru/blackbox/methods/user_ticket#request_format
		Attributes []UserAttribute

		// Requests user dbfields.
		// Documentation: https://docs.yandex-team.ru/blackbox/methods/user_ticket#request_format
		DBFields []UserDBField

		// Requests user login aliases. Supported only list of needed aliases.
		// Documentation: https://docs.yandex-team.ru/blackbox/methods/user_ticket#request_format
		Aliases []UserAlias

		// Requests user emails.
		// Documentation: https://docs.yandex-team.ru/blackbox/methods/user_ticket#request_format
		Emails EmailsRequest

		// Email that is checked for belonging to the user.
		// Documentation: https://docs.yandex-team.ru/blackbox/methods/user_ticket#request_format
		//
		// Required if .Emails is EmailTestOne
		AddrToTest string
	}

	CheckIPRequest struct {
		// IP to check.
		// Documentation: https://docs.yandex-team.ru/blackbox/methods/checkip
		IP string
	}
)

func (req EmailsRequest) String() string {
	return string(req)
}

func (req GetPhonesRequest) String() string {
	return string(req)
}

func (req SessionIDRequest) Validate() error {
	if req.SessionID == "" {
		return ErrRequestNoSessionID
	}

	if req.Host == "" {
		return ErrRequestNoHost
	}

	if req.UserIP == "" {
		return ErrRequestNoUserIP
	}

	if req.Emails == EmailTestOne && req.AddrToTest == "" {
		return ErrRequestNoEmailToTest
	}

	if req.GetPhones == GetPhonesUnspecified && len(req.PhoneAttributes) > 0 {
		return ErrRequestNoGetPhones
	}

	return nil
}

func (req MultiSessionIDRequest) Validate() error {
	if req.SessionID == "" {
		return ErrRequestNoSessionID
	}

	if req.Host == "" {
		return ErrRequestNoHost
	}

	if req.UserIP == "" {
		return ErrRequestNoUserIP
	}

	if req.Emails == EmailTestOne && req.AddrToTest == "" {
		return ErrRequestNoEmailToTest
	}

	return nil
}

func (req OAuthRequest) Validate() error {
	if req.OAuthToken == "" {
		return ErrRequestNoOAuthToken
	}

	if req.UserIP == "" {
		return ErrRequestNoUserIP
	}

	if req.Emails == EmailTestOne && req.AddrToTest == "" {
		return ErrRequestNoEmailToTest
	}

	return nil
}

func (req UserInfoRequest) Validate() error {
	if len(req.UIDs) == 0 && req.Login == "" && req.PublicID == "" {
		return ErrRequestNoUIDOrLogin
	}

	if req.UserIP == "" {
		return ErrRequestNoUserIP
	}

	if req.Emails == EmailTestOne && req.AddrToTest == "" {
		return ErrRequestNoEmailToTest
	}

	return nil
}

func (req UserTicketRequest) Validate() error {
	if req.UserTicket == "" {
		return ErrRequestNoUserTicket
	}

	if len(req.UIDs) > 0 && req.Multi {
		return ErrRequestUIDsandMultiConflict
	}

	if req.Emails == EmailTestOne && req.AddrToTest == "" {
		return ErrRequestNoEmailToTest
	}

	return nil
}

func (req CheckIPRequest) Validate() error {
	if req.IP == "" {
		return ErrRequestNoUserIP
	}

	return nil
}
