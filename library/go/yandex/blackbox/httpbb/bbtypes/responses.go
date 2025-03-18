package bbtypes

import (
	"time"

	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/yandex/blackbox"
)

type Response interface {
	CheckError() *BlackboxError
}

type BaseResponse struct {
	Exception *struct {
		ID BlackboxErrorCode `json:"id"`
	} `json:"exception"`
	Error string `json:"error"`
}

func (r *BaseResponse) CheckError() *BlackboxError {
	if r.Exception == nil {
		return nil
	}

	return &BlackboxError{
		Code:    r.Exception.ID,
		Message: r.Error,
	}
}

type SessionIDResponse struct {
	/*
		Not supported fields so far:
		  - special
		  - new-session
		  - emails
		  - karma
		  - karma_status
	*/
	BaseResponse
	Status       Status              `json:"status"`
	Age          int64               `json:"age"`
	ExpiresIn    int64               `json:"expires_in"`
	Attributes   Attributes          `json:"attributes"`
	Aliases      Aliases             `json:"aliases"`
	DisplayName  *DisplayName        `json:"display_name"`
	AddressList  *AddressList        `json:"address-list"`
	PhoneList    *PhoneList          `json:"phones"`
	AuthInfo     AuthInfo            `json:"auth"`
	Login        string              `json:"login"`
	PublicID     string              `json:"public_id"`
	TTL          blackbox.SessionTTL `json:"ttl"`
	UID          UID                 `json:"uid"`
	UserTicket   string              `json:"user_ticket"`
	ConnectionID string              `json:"connection_id"`
	DBFields     DBFields            `json:"dbfields"`
	FamilyInfo   FamilyInfo          `json:"family_info"`
}

type MultiSessionIDResponse struct {
	/*
		Not supported fields so far:
		  - special
		  - new-session
		  - allow_more_users
		  - users/phones
		  - users/emails
		  - users/karma
		  - users/karma_status
	*/

	BaseResponse
	Status       Status              `json:"status"`
	Age          int64               `json:"age"`
	ExpiresIn    int64               `json:"expires_in"`
	DefaultUID   blackbox.ID         `json:"default_uid,string"`
	TTL          blackbox.SessionTTL `json:"ttl"`
	UserTicket   string              `json:"user_ticket"`
	ConnectionID string              `json:"connection_id"`
	Users        []struct {
		Status      Status       `json:"status"`
		UID         UID          `json:"uid"`
		Login       string       `json:"login"`
		PublicID    string       `json:"public_id"`
		AuthInfo    AuthInfo     `json:"auth"`
		DisplayName *DisplayName `json:"display_name"`
		AddressList *AddressList `json:"address-list"`
		Attributes  Attributes   `json:"attributes"`
		Aliases     Aliases      `json:"aliases"`
		DBFields    DBFields     `json:"dbfields"`
		FamilyInfo  FamilyInfo   `json:"family_info"`
	} `json:"users"`
}

type OAuthResponse struct {
	/*
		Not supported fields so far:
		  - karma
		  - karma_status
		  - phones
		  - emails
		  - oauth/is_ttl_refreshable
		  - oauth/client_homepage
	*/

	BaseResponse
	OAuth struct {
		ClientName string `json:"client_name"`
		ClientID   string `json:"client_id"`
		ClientIcon string `json:"client_icon"`
		Scope      string `json:"scope"`
		DeviceID   string `json:"device_id"`
		DeviceName string `json:"device_name"`
		TokenID    string `json:"token_id"`
		IssueTime  bbTime `json:"issue_time"`
		CreateTime bbTime `json:"ctime"`
		ExpireTime bbTime `json:"expire_time"`
	} `json:"oauth"`
	Status       Status       `json:"status"`
	Attributes   Attributes   `json:"attributes"`
	DisplayName  *DisplayName `json:"display_name"`
	AddressList  *AddressList `json:"address-list"`
	Aliases      Aliases      `json:"aliases"`
	Login        string       `json:"login"`
	PublicID     string       `json:"public_id"`
	UID          UID          `json:"uid"`
	UserTicket   string       `json:"user_ticket"`
	ConnectionID string       `json:"connection_id"`
	LoginID      string       `json:"login_id"`
	DBFields     DBFields     `json:"dbfields"`
	FamilyInfo   FamilyInfo   `json:"family_info"`
}

type UserInfoResponse struct {
	BaseResponse
	Users []struct {
		Status       Status       `json:"status"`
		UID          UID          `json:"uid"`
		Login        string       `json:"login"`
		PublicID     string       `json:"public_id"`
		DisplayName  *DisplayName `json:"display_name"`
		AddressList  *AddressList `json:"address-list"`
		PhoneList    *PhoneList   `json:"phones"`
		Attributes   Attributes   `json:"attributes"`
		Aliases      Aliases      `json:"aliases"`
		HavePassword bool         `json:"have_password"`
		HaveHint     bool         `json:"have_hint"`
		DBFields     DBFields     `json:"dbfields"`
		FamilyInfo   FamilyInfo   `json:"family_info"`
		KarmaStatus  KarmaStatus  `json:"karma_status"`
	} `json:"users"`
}

type UserTicketResponse struct {
	BaseResponse
	Users []struct {
		Status      Status       `json:"status"`
		UID         UID          `json:"uid"`
		Login       string       `json:"login"`
		PublicID    string       `json:"public_id"`
		DisplayName *DisplayName `json:"display_name"`
		AddressList *AddressList `json:"address-list"`
		Attributes  Attributes   `json:"attributes"`
		Aliases     Aliases      `json:"aliases"`
		DBFields    DBFields     `json:"dbfields"`
	} `json:"users"`
}

type CheckIPResponse struct {
	BaseResponse
	YandexIP bool `json:"yandexip"`
}

type Attributes = map[blackbox.UserAttribute]string

type Aliases = map[blackbox.UserAlias]string

type DBFields = map[blackbox.UserDBField]string

type Status struct {
	ID blackbox.Status `json:"id"`
}

type UID struct {
	Hosted bool        `json:"hosted"`
	Lite   bool        `json:"lite"`
	Value  blackbox.ID `json:"value,string"`
}

type KarmaStatus struct {
	Value uint64 `json:"value"`
}

func (ks KarmaStatus) KarmaStatusValue() blackbox.KarmaStatus {
	return ks.Value
}

type FamilyInfo struct {
	AdminUID blackbox.ID `json:"admin_uid,string"`
	FamilyID string      `json:"family_id"`
}

func (u UID) BlackBoxUID() blackbox.UID {
	return blackbox.UID{
		ID:     u.Value,
		Lite:   u.Lite,
		Hosted: u.Hosted,
	}
}

type AuthInfo struct {
	HavePassword            bool  `json:"have_password"`
	PasswordVerificationAge int64 `json:"password_verification_age"`
}

func (a AuthInfo) BlackBoxAuthInfo() blackbox.AuthInfo {
	return blackbox.AuthInfo(a)
}

type DisplayName struct {
	Name       string `json:"name"`
	PublicName string `json:"public_name"`
	Avatar     struct {
		Default string `json:"default"`
		Empty   bool   `json:"empty"`
	} `json:"avatar"`
}

func (d *DisplayName) BlackBoxDisplayName() blackbox.DisplayName {
	if d == nil {
		return blackbox.DisplayName{}
	}

	return blackbox.DisplayName{
		Name:       d.Name,
		PublicName: d.PublicName,
		AvatarID:   d.Avatar.Default,
		Empty:      d.Avatar.Empty,
	}
}

func (fi *FamilyInfo) BlackboxFamilyInfo() blackbox.FamilyInfo {
	if fi == nil {
		return blackbox.FamilyInfo{}
	}

	return blackbox.FamilyInfo{
		AdminUID: fi.AdminUID,
		FamilyID: fi.FamilyID,
	}
}

type AddressList []struct {
	Address   string `json:"address"`
	Validated bool   `json:"validated"`
	Default   bool   `json:"default"`
	RPOP      bool   `json:"rpop"`
	Unsafe    bool   `json:"unsafe"`
	Native    bool   `json:"native"`
	BornDate  bbTime `json:"born-date"`
}

func (a *AddressList) BlackBoxAddressList() []blackbox.Address {
	if a == nil {
		return nil
	}

	addressList := make([]blackbox.Address, len(*a))
	for i, addr := range *a {
		addressList[i] = blackbox.Address{
			Address:   addr.Address,
			Default:   addr.Default,
			Validated: addr.Validated,
			BornDate:  addr.BornDate.Time,
			RPOP:      addr.RPOP,
			Native:    addr.Native,
			Unsafe:    addr.Unsafe,
		}
	}
	return addressList
}

type PhoneList []struct {
	ID         string          `json:"id"`
	Attributes PhoneAttributes `json:"attributes"`
}

type PhoneAttributes struct {
	FormattedNumber       string `json:"101"`
	E164Number            string `json:"102"`
	MaskedFormattedNumber string `json:"103"`
	MaskedE164Number      string `json:"104"`
	IsConfirmed           string `json:"105"`
	IsBound               string `json:"106"`
	IsDefault             string `json:"107"`
	IsSecured             string `json:"108"`
	IsBankPhoneNumber     string `json:"109"`
}

func (a *PhoneList) BlackBoxPhoneList() []blackbox.Phone {
	if a == nil {
		return nil
	}

	phonesList := make([]blackbox.Phone, len(*a))
	for i, phone := range *a {
		phonesList[i] = blackbox.Phone{
			ID:                    phone.ID,
			FormattedNumber:       phone.Attributes.FormattedNumber,
			E164Number:            phone.Attributes.E164Number,
			MaskedFormattedNumber: phone.Attributes.MaskedFormattedNumber,
			MaskedE164Number:      phone.Attributes.MaskedE164Number,
			IsConfirmed:           ParseBlackBoxBool(phone.Attributes.IsConfirmed),
			IsBound:               ParseBlackBoxBool(phone.Attributes.IsBound),
			IsDefault:             ParseBlackBoxBool(phone.Attributes.IsDefault),
			IsSecured:             ParseBlackBoxBool(phone.Attributes.IsSecured),
			IsBankPhoneNumber:     ParseBlackBoxBool(phone.Attributes.IsBankPhoneNumber),
		}
	}
	return phonesList
}

type bbTime struct {
	time.Time
}

const (
	bbTimeFormat = "2006-01-02 15:04:05"
)

func (t bbTime) MarshalJSON() ([]byte, error) {
	if y := t.Year(); y < 0 || y >= 10000 {
		// RFC 3339 is clear that years are 4 digits exactly.
		// See golang.org/issue/4556#c15 for more discussion.
		return nil, xerrors.New("bbTime.MarshalJSON: year outside of range [0,9999]")
	}

	b := make([]byte, 0, len(bbTimeFormat)+2)
	b = append(b, '"')
	b = t.AppendFormat(b, bbTimeFormat)
	b = append(b, '"')
	return b, nil
}

func (t *bbTime) UnmarshalJSON(data []byte) error {
	// Ignore null, like in the main JSON package.
	if string(data) == "null" {
		return nil
	}

	var err error
	t.Time, err = time.Parse(`"`+bbTimeFormat+`"`, string(data))
	return err
}

func ParseBlackBoxBool(s string) bool {
	return s == "1"
}
