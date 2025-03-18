package blackbox

import "time"

const (
	SessionTTLShort         SessionTTL = "0"
	SessionTTLMdaShort      SessionTTL = "2"
	SessionTTLIntranetShort SessionTTL = "4"
	SessionTTLLong          SessionTTL = "5"
)

type (
	User struct {
		ID          ID
		UID         UID
		DisplayName DisplayName
		PublicID    string
		Attributes  map[UserAttribute]string
		Aliases     map[UserAlias]string
		Auth        AuthInfo
		Login       string
		AddressList []Address
		PhoneList   []Phone
		HasPassword bool
		HasHint     bool
		DBFields    map[UserDBField]string
		FamilyInfo  FamilyInfo
		KarmaStatus KarmaStatus
	}

	SessionIDResponse struct {
		User         User
		NeedResign   bool
		UserTicket   string
		Age          int64
		ExpiresIn    int64
		TTL          SessionTTL
		ConnectionID string
	}

	MultiSessionIDResponse struct {
		DefaultUID   ID
		NeedResign   bool
		Users        []User
		UserTicket   string
		Age          int64
		ExpiresIn    int64
		TTL          SessionTTL
		ConnectionID string
	}

	OAuthResponse struct {
		User         User
		Scopes       []string
		ClientID     string
		ClientName   string
		ClientIcon   string
		DeviceID     string
		DeviceName   string
		TokenID      string
		IssueTime    time.Time
		CreateTime   time.Time
		ExpireTime   time.Time
		UserTicket   string
		ConnectionID string
		LoginID      string
	}

	UserInfoResponse struct {
		Users []User
	}

	UserTicketResponse struct {
		Users []User
	}

	CheckIPResponse struct {
		YandexIP bool
	}

	ID          = uint64
	KarmaStatus = uint64

	SessionTTL string

	DisplayName struct {
		Name       string
		PublicName string
		AvatarID   string
		Empty      bool
	}

	Address struct {
		Address   string
		Validated bool
		Default   bool
		RPOP      bool
		Unsafe    bool
		Silent    bool
		Native    bool
		BornDate  time.Time
	}

	Phone struct {
		ID                    string
		FormattedNumber       string
		E164Number            string
		MaskedFormattedNumber string
		MaskedE164Number      string
		IsConfirmed           bool
		IsBound               bool
		IsDefault             bool
		IsSecured             bool
		IsBankPhoneNumber     bool
	}

	UID struct {
		ID     ID
		Hosted bool
		Lite   bool
	}

	AuthInfo struct {
		// Excluded Secure and PartnerPddToken fields.
		HavePassword            bool
		PasswordVerificationAge int64
	}

	FamilyInfo struct {
		AdminUID ID
		FamilyID string
	}
)

func (u *MultiSessionIDResponse) DefaultUser() (*User, error) {
	if u.DefaultUID == 0 {
		return nil, ErrNoDefaultUID
	}

	for i, user := range u.Users {
		if user.ID == u.DefaultUID {
			return &u.Users[i], nil
		}
	}

	return nil, ErrNoDefaultUser
}
