package httpbb

import (
	"context"
	"strconv"
	"strings"
	"time"

	"github.com/go-resty/resty/v2"

	"a.yandex-team.ru/library/go/yandex/blackbox"
	"a.yandex-team.ru/library/go/yandex/blackbox/httpbb/bbtypes"
	"a.yandex-team.ru/library/go/yandex/tvm"
)

var _ blackbox.Client = (*Client)(nil)

type (
	Client struct {
		http *HTTPClient
	}

	Environment struct {
		BlackboxHost string
		TvmID        tvm.ClientID
	}
)

var (
	IntranetEnvironment = Environment{
		BlackboxHost: "https://blackbox.yandex-team.ru",
		TvmID:        223,
	}

	TestEnvironment = Environment{
		BlackboxHost: "https://blackbox-test.yandex.net",
		TvmID:        224,
	}

	MiminoEnvironment = Environment{
		BlackboxHost: "https://blackbox-mimino.yandex.net",
		TvmID:        239,
	}

	ProdEnvironment = Environment{
		BlackboxHost: "https://blackbox.yandex.net",
		TvmID:        222,
	}
)

const (
	defaultTimeout = 200 * time.Millisecond
	defaultRetries = 3

	backoffInitInternal   = 100 * time.Millisecond
	backoffMaxInterval    = 200 * time.Millisecond
	backoffMaxElapsedTime = 1 * time.Second
)

// NewClient creates new client with custom blackbox environment.
func NewClient(env Environment, opts ...Option) (*Client, error) {
	return NewClientWithResty(env, resty.New(), opts...)
}

// NewClientWithResty creates new client with custom resty client and custom environment
// BE CAREFUL: options affect resty client, side-effects could be unexpected if you use it somewhere else
func NewClientWithResty(env Environment, r *resty.Client, opts ...Option) (*Client, error) {
	c := &Client{
		http: newHTTPClient(env, r),
	}

	for _, opt := range opts {
		opt(c)
	}

	c.http.setupTLS()
	return c, nil
}

// NewIntranet creates new blackbox client for intranet environment (aka yandex-team.ru).
func NewIntranet(opts ...Option) (*Client, error) {
	return NewClient(IntranetEnvironment, opts...)
}

// NewTest creates new blackbox client for test environment (aka passport-test.yandex.ru).
func NewTest(opts ...Option) (*Client, error) {
	return NewClient(TestEnvironment, opts...)
}

// NewMimino creates new blackbox client for mimino environment.
func NewMimino(opts ...Option) (*Client, error) {
	return NewClient(MiminoEnvironment, opts...)
}

// NewProd creates new blackbox client for production environment (aka yandex.ru).
func NewProd(opts ...Option) (*Client, error) {
	return NewClient(ProdEnvironment, opts...)
}

// SessionID calls sessionid blackbox method.
// Documentation: https://docs.yandex-team.ru/blackbox/methods/sessionid
func (c *Client) SessionID(
	ctx context.Context,
	req blackbox.SessionIDRequest,
) (*blackbox.SessionIDResponse, error) {

	if err := req.Validate(); err != nil {
		return nil, err
	}

	if req.GetUserTicket && !c.http.HasTVM() {
		return nil, blackbox.ErrRequestTvmNotAvailable
	}

	queryParams := map[string]string{
		"method": "sessionid",
		"format": "json",
		"userip": req.UserIP,
		"host":   req.Host,
	}

	formData := map[string]string{
		"sessionid": req.SessionID,
	}

	if req.DefaultUID > 0 {
		queryParams["default_uid"] = strconv.FormatUint(req.DefaultUID, 10)
	}

	if req.FullInfo {
		queryParams["full_info"] = "yes"
	}

	if len(req.Attributes) > 0 {
		queryParams["attributes"] = attributesToQuery(req.Attributes)
	}

	if len(req.DBFields) > 0 {
		queryParams["dbfields"] = dbfieldsToQuery(req.DBFields)
	}

	if req.RegName {
		queryParams["regname"] = "yes"
	}

	if req.GetPublicName {
		queryParams["get_public_name"] = "yes"
	}

	if req.GetPublicID {
		queryParams["get_public_id"] = "yes"
	}

	if req.GetFamilyInfo {
		queryParams["get_family_info"] = "yes"
	}

	if req.Emails != blackbox.EmailsUnspecified {
		queryParams["emails"] = req.Emails.String()
	}

	if req.GetPhones != blackbox.GetPhonesUnspecified {
		queryParams["getphones"] = req.GetPhones.String()
	}

	if len(req.PhoneAttributes) > 0 {
		queryParams["phone_attributes"] = phoneAttributesToQuery(req.PhoneAttributes)
	}

	if req.AddrToTest != "" {
		queryParams["addrtotest"] = req.AddrToTest
	}

	if req.GetUserTicket {
		queryParams["get_user_ticket"] = "yes"
	}

	if len(req.Aliases) > 0 {
		queryParams["aliases"] = aliasesToQuery(req.Aliases)
	}

	if req.StatboxID != "" {
		queryParams["statbox_id"] = req.StatboxID
	}

	if req.YandexUID != "" {
		queryParams["yandexuid"] = req.YandexUID
	}

	httpReq := c.http.R(ctx).
		SetQueryParams(queryParams).
		SetFormData(formData)
	var rsp bbtypes.SessionIDResponse
	if err := c.http.Post(httpReq, &rsp); err != nil {
		return nil, err
	}

	/*
		Session is valid in two statuses: VALID and NEED_RESET
		Documentation: https://docs.yandex-team.ru/blackbox/methods/sessionid#response_format
	*/

	switch rsp.Status.ID {
	case blackbox.StatusValid, blackbox.StatusNeedReset:
		user := &blackbox.SessionIDResponse{
			User: blackbox.User{
				ID:          rsp.UID.Value,
				UID:         rsp.UID.BlackBoxUID(),
				DisplayName: rsp.DisplayName.BlackBoxDisplayName(),
				PublicID:    rsp.PublicID,
				Attributes:  rsp.Attributes,
				Aliases:     rsp.Aliases,
				Login:       rsp.Login,
				Auth:        rsp.AuthInfo.BlackBoxAuthInfo(),
				AddressList: rsp.AddressList.BlackBoxAddressList(),
				PhoneList:   rsp.PhoneList.BlackBoxPhoneList(),
				DBFields:    rsp.DBFields,
				FamilyInfo:  rsp.FamilyInfo.BlackboxFamilyInfo(),
			},
			NeedResign:   rsp.Status.ID == blackbox.StatusNeedReset,
			UserTicket:   rsp.UserTicket,
			Age:          rsp.Age,
			ExpiresIn:    rsp.ExpiresIn,
			TTL:          rsp.TTL,
			ConnectionID: rsp.ConnectionID,
		}
		return user, nil
	case blackbox.StatusExpired, blackbox.StatusNoAuth, blackbox.StatusDisabled:
		return nil, &blackbox.UnauthorizedError{
			StatusError: blackbox.StatusError{
				Status:  rsp.Status.ID,
				Message: rsp.Error,
			},
		}
	default:
		return nil, &blackbox.StatusError{
			Status:  rsp.Status.ID,
			Message: rsp.Error,
		}
	}
}

// MultiSessionID calls sessionid blackbox method for multisession=yes.
// Documentation: https://docs.yandex-team.ru/blackbox/methods/sessionid
func (c *Client) MultiSessionID(
	ctx context.Context,
	req blackbox.MultiSessionIDRequest,
) (*blackbox.MultiSessionIDResponse, error) {

	if err := req.Validate(); err != nil {
		return nil, err
	}

	if req.GetUserTicket && !c.http.HasTVM() {
		return nil, blackbox.ErrRequestTvmNotAvailable
	}

	queryParams := map[string]string{
		"method":       "sessionid",
		"multisession": "yes",
		"format":       "json",
		"userip":       req.UserIP,
		"host":         req.Host,
	}

	formData := map[string]string{
		"sessionid": req.SessionID,
	}

	if req.DefaultUID > 0 {
		queryParams["default_uid"] = strconv.FormatUint(req.DefaultUID, 10)
	}

	if req.FullInfo {
		queryParams["full_info"] = "yes"
	}

	if len(req.Attributes) > 0 {
		queryParams["attributes"] = attributesToQuery(req.Attributes)
	}

	if len(req.DBFields) > 0 {
		queryParams["dbfields"] = dbfieldsToQuery(req.DBFields)
	}

	if req.RegName {
		queryParams["regname"] = "yes"
	}

	if req.GetPublicName {
		queryParams["get_public_name"] = "yes"
	}

	if req.GetPublicID {
		queryParams["get_public_id"] = "yes"
	}

	if req.GetFamilyInfo {
		queryParams["get_family_info"] = "yes"
	}

	if req.Emails != blackbox.EmailsUnspecified {
		queryParams["emails"] = req.Emails.String()
	}

	if req.AddrToTest != "" {
		queryParams["addrtotest"] = req.AddrToTest
	}

	if req.GetUserTicket {
		queryParams["get_user_ticket"] = "yes"
	}

	if len(req.Aliases) > 0 {
		queryParams["aliases"] = aliasesToQuery(req.Aliases)
	}

	if req.StatboxID != "" {
		queryParams["statbox_id"] = req.StatboxID
	}

	if req.YandexUID != "" {
		queryParams["yandexuid"] = req.YandexUID
	}

	httpReq := c.http.R(ctx).
		SetQueryParams(queryParams).
		SetFormData(formData)
	var rsp bbtypes.MultiSessionIDResponse
	if err := c.http.Post(httpReq, &rsp); err != nil {
		return nil, err
	}

	/*
		Session is valid in two statuses: VALID and NEED_RESET
		Documentation: https://docs.yandex-team.ru/blackbox/methods/sessionid#response_format
	*/

	switch rsp.Status.ID {
	case blackbox.StatusValid, blackbox.StatusNeedReset:
		result := &blackbox.MultiSessionIDResponse{
			DefaultUID:   rsp.DefaultUID,
			NeedResign:   rsp.Status.ID == blackbox.StatusNeedReset,
			Users:        make([]blackbox.User, 0, len(rsp.Users)),
			UserTicket:   rsp.UserTicket,
			ConnectionID: rsp.ConnectionID,
			TTL:          rsp.TTL,
			Age:          rsp.Age,
			ExpiresIn:    rsp.ExpiresIn,
		}

		for _, user := range rsp.Users {
			if user.Status.ID != blackbox.StatusValid && user.Status.ID != blackbox.StatusNeedReset {
				// skip not valid users in session
				continue
			}

			result.Users = append(result.Users, blackbox.User{
				ID:          user.UID.Value,
				UID:         user.UID.BlackBoxUID(),
				PublicID:    user.PublicID,
				DisplayName: user.DisplayName.BlackBoxDisplayName(),
				Attributes:  user.Attributes,
				Aliases:     user.Aliases,
				Login:       user.Login,
				Auth:        user.AuthInfo.BlackBoxAuthInfo(),
				AddressList: user.AddressList.BlackBoxAddressList(),
				DBFields:    user.DBFields,
				FamilyInfo:  user.FamilyInfo.BlackboxFamilyInfo(),
			})
		}
		return result, nil
	case blackbox.StatusExpired, blackbox.StatusNoAuth:
		return nil, &blackbox.UnauthorizedError{
			StatusError: blackbox.StatusError{
				Status:  rsp.Status.ID,
				Message: rsp.Error,
			},
		}
	default:
		return nil, &blackbox.StatusError{
			Status:  rsp.Status.ID,
			Message: rsp.Error,
		}
	}
}

// OAuth calls oauth blackbox method.
// Documentation: https://docs.yandex-team.ru/blackbox/methods/oauth
func (c *Client) OAuth(
	ctx context.Context,
	req blackbox.OAuthRequest,
) (*blackbox.OAuthResponse, error) {

	if err := req.Validate(); err != nil {
		return nil, err
	}

	if req.GetUserTicket && !c.http.HasTVM() {
		return nil, blackbox.ErrRequestTvmNotAvailable
	}

	queryParams := map[string]string{
		"method": "oauth",
		"format": "json",
		"userip": req.UserIP,
	}

	if len(req.Attributes) > 0 {
		queryParams["attributes"] = attributesToQuery(req.Attributes)
	}

	if len(req.DBFields) > 0 {
		queryParams["dbfields"] = dbfieldsToQuery(req.DBFields)
	}

	if req.RegName {
		queryParams["regname"] = "yes"
	}

	if req.GetPublicName {
		queryParams["get_public_name"] = "yes"
	}

	if req.GetPublicID {
		queryParams["get_public_id"] = "yes"
	}

	if req.GetFamilyInfo {
		queryParams["get_family_info"] = "yes"
	}

	if req.Emails != blackbox.EmailsUnspecified {
		queryParams["emails"] = req.Emails.String()
	}

	if req.AddrToTest != "" {
		queryParams["addrtotest"] = req.AddrToTest
	}

	if len(req.Scopes) > 0 {
		queryParams["scopes"] = strings.Join(req.Scopes, ",")
	}

	if len(req.Aliases) > 0 {
		queryParams["aliases"] = aliasesToQuery(req.Aliases)
	}

	if req.GetUserTicket {
		queryParams["get_user_ticket"] = "yes"
	}

	if req.GetLoginID {
		queryParams["get_login_id"] = "yes"
	}

	httpReq := c.http.R(ctx).
		SetHeader("Authorization", "OAuth "+req.OAuthToken).
		SetQueryParams(queryParams)

	var rsp bbtypes.OAuthResponse
	if err := c.http.Get(httpReq, &rsp); err != nil {
		return nil, err
	}

	/*
		OAuth token is valid only for VALID status
		Documentation: https://docs.yandex-team.ru/blackbox/methods/oauth#response_format
	*/

	switch rsp.Status.ID {
	case blackbox.StatusValid:
		user := &blackbox.OAuthResponse{
			User: blackbox.User{
				ID:          rsp.UID.Value,
				UID:         rsp.UID.BlackBoxUID(),
				PublicID:    rsp.PublicID,
				DisplayName: rsp.DisplayName.BlackBoxDisplayName(),
				Attributes:  rsp.Attributes,
				Aliases:     rsp.Aliases,
				Login:       rsp.Login,
				AddressList: rsp.AddressList.BlackBoxAddressList(),
				DBFields:    rsp.DBFields,
				FamilyInfo:  rsp.FamilyInfo.BlackboxFamilyInfo(),
			},
			Scopes:       strings.Split(rsp.OAuth.Scope, " "),
			ClientID:     rsp.OAuth.ClientID,
			ClientName:   rsp.OAuth.ClientName,
			ClientIcon:   rsp.OAuth.ClientIcon,
			DeviceID:     rsp.OAuth.DeviceID,
			DeviceName:   rsp.OAuth.DeviceName,
			TokenID:      rsp.OAuth.TokenID,
			IssueTime:    rsp.OAuth.IssueTime.Time,
			CreateTime:   rsp.OAuth.CreateTime.Time,
			ExpireTime:   rsp.OAuth.ExpireTime.Time,
			ConnectionID: rsp.ConnectionID,
			LoginID:      rsp.LoginID,
			UserTicket:   rsp.UserTicket,
		}

		return user, nil
	case blackbox.StatusDisabled:
		return nil, &blackbox.UnauthorizedError{
			StatusError: blackbox.StatusError{
				Status:  rsp.Status.ID,
				Message: rsp.Error,
			},
		}
	default:
		return nil, &blackbox.StatusError{
			Status:  rsp.Status.ID,
			Message: rsp.Error,
		}
	}
}

// UserInfo calls userinfo blackbox method.
// Documentation: https://docs.yandex-team.ru/blackbox/methods/userinfo
func (c *Client) UserInfo(
	ctx context.Context,
	req blackbox.UserInfoRequest,
) (*blackbox.UserInfoResponse, error) {

	if err := req.Validate(); err != nil {
		return nil, err
	}

	queryParams := map[string]string{
		"method": "userinfo",
		"format": "json",
		"userip": req.UserIP,
	}

	if len(req.UIDs) > 0 {
		queryParams["uid"] = uidsToQuery(req.UIDs)
	}

	if req.Login != "" {
		queryParams["login"] = req.Login
	}

	if req.PublicID != "" {
		queryParams["public_id"] = req.PublicID
	}

	if len(req.Attributes) > 0 {
		queryParams["attributes"] = attributesToQuery(req.Attributes)
	}

	if len(req.DBFields) > 0 {
		queryParams["dbfields"] = dbfieldsToQuery(req.DBFields)
	}

	if req.RegName {
		queryParams["regname"] = "yes"
	}

	if req.GetPhones != blackbox.GetPhonesUnspecified {
		queryParams["getphones"] = req.GetPhones.String()
	}

	if len(req.PhoneAttributes) > 0 {
		queryParams["phone_attributes"] = phoneAttributesToQuery(req.PhoneAttributes)
	}

	if req.GetPublicName {
		queryParams["get_public_name"] = "yes"
	}

	if req.GetPublicID {
		queryParams["get_public_id"] = "yes"
	}

	if req.GetFamilyInfo {
		queryParams["get_family_info"] = "yes"
	}

	if len(req.Aliases) > 0 {
		queryParams["aliases"] = aliasesToQuery(req.Aliases)
	}

	if req.Emails != blackbox.EmailsUnspecified {
		queryParams["emails"] = req.Emails.String()
	}

	if req.AddrToTest != "" {
		queryParams["addrtotest"] = req.AddrToTest
	}

	httpReq := c.http.R(ctx).
		SetQueryParams(queryParams)

	var rsp bbtypes.UserInfoResponse
	if err := c.http.Get(httpReq, &rsp); err != nil {
		return nil, err
	}

	result := &blackbox.UserInfoResponse{
		Users: make([]blackbox.User, 0, len(rsp.Users)),
	}
	for _, u := range rsp.Users {
		if u.UID.Value == 0 {
			continue
		}

		result.Users = append(result.Users, blackbox.User{
			ID:          u.UID.Value,
			UID:         u.UID.BlackBoxUID(),
			PublicID:    u.PublicID,
			DisplayName: u.DisplayName.BlackBoxDisplayName(),
			Attributes:  u.Attributes,
			Aliases:     u.Aliases,
			Login:       u.Login,
			AddressList: u.AddressList.BlackBoxAddressList(),
			HasPassword: u.HavePassword,
			PhoneList:   u.PhoneList.BlackBoxPhoneList(),
			HasHint:     u.HaveHint,
			DBFields:    u.DBFields,
			FamilyInfo:  u.FamilyInfo.BlackboxFamilyInfo(),
			KarmaStatus: u.KarmaStatus.KarmaStatusValue(),
		})
	}

	return result, nil
}

// UserTicket calls user_ticket blackbox method.
// Documentation: https://docs.yandex-team.ru/blackbox/methods/user_ticket
func (c *Client) UserTicket(
	ctx context.Context,
	req blackbox.UserTicketRequest,
) (*blackbox.UserTicketResponse, error) {

	if err := req.Validate(); err != nil {
		return nil, err
	}

	queryParams := map[string]string{
		"method": "user_ticket",
		"format": "json",
	}

	formData := map[string]string{
		"user_ticket": req.UserTicket,
	}

	if len(req.UIDs) > 0 {
		queryParams["uid"] = uidsToQuery(req.UIDs)
	}

	if req.Multi {
		queryParams["multi"] = "yes"
	}

	if len(req.Attributes) > 0 {
		queryParams["attributes"] = attributesToQuery(req.Attributes)
	}

	if len(req.DBFields) > 0 {
		queryParams["dbfields"] = dbfieldsToQuery(req.DBFields)
	}

	if req.GetPublicName {
		queryParams["get_public_name"] = "yes"
	}

	if req.GetPublicID {
		queryParams["get_public_id"] = "yes"
	}

	if len(req.Aliases) > 0 {
		queryParams["aliases"] = aliasesToQuery(req.Aliases)
	}

	if req.Emails != blackbox.EmailsUnspecified {
		queryParams["emails"] = req.Emails.String()
	}

	if req.AddrToTest != "" {
		queryParams["addrtotest"] = req.AddrToTest
	}

	httpReq := c.http.R(ctx).
		SetQueryParams(queryParams).
		SetFormData(formData)
	var rsp bbtypes.UserTicketResponse
	if err := c.http.Post(httpReq, &rsp); err != nil {
		return nil, err
	}

	result := &blackbox.UserTicketResponse{
		Users: make([]blackbox.User, 0, len(rsp.Users)),
	}
	for _, u := range rsp.Users {
		if u.UID.Value == 0 {
			continue
		}

		result.Users = append(result.Users, blackbox.User{
			ID:          u.UID.Value,
			UID:         u.UID.BlackBoxUID(),
			PublicID:    u.PublicID,
			DisplayName: u.DisplayName.BlackBoxDisplayName(),
			Attributes:  u.Attributes,
			Aliases:     u.Aliases,
			Login:       u.Login,
			AddressList: u.AddressList.BlackBoxAddressList(),
			DBFields:    u.DBFields,
		})
	}

	return result, nil
}

// CheckIP calls checkip blackbox method.
// Documentation: https://docs.yandex-team.ru/blackbox/methods/checkip
func (c *Client) CheckIP(
	ctx context.Context,
	req blackbox.CheckIPRequest,
) (*blackbox.CheckIPResponse, error) {

	if err := req.Validate(); err != nil {
		return nil, err
	}

	queryParams := map[string]string{
		"method": "checkip",
		"format": "json",
		// Blackbox supports only this network type so far.
		"nets": "yandexusers",
		"ip":   req.IP,
	}

	httpReq := c.http.R(ctx).SetQueryParams(queryParams)
	var rsp bbtypes.CheckIPResponse
	if err := c.http.Get(httpReq, &rsp); err != nil {
		return nil, err
	}

	return &blackbox.CheckIPResponse{
		YandexIP: rsp.YandexIP,
	}, nil
}

// InternalHTTP returns internal HTTP client.
// Few reminders:
//   1. Don't use it except if you create your own BlackBox client
//   2. Don't forget to consult with pkg owner
func (c *Client) InternalHTTP() *HTTPClient {
	return c.http
}

func attributesToQuery(attrs []blackbox.UserAttribute) string {
	l := make([]string, len(attrs))
	for i, v := range attrs {
		l[i] = string(v)
	}

	return strings.Join(l, ",")
}

func dbfieldsToQuery(dbfields []blackbox.UserDBField) string {
	l := make([]string, len(dbfields))
	for i, v := range dbfields {
		l[i] = string(v)
	}

	return strings.Join(l, ",")
}

func phoneAttributesToQuery(attrs []blackbox.PhoneAttribute) string {
	l := make([]string, len(attrs))
	for i, v := range attrs {
		l[i] = string(v)
	}

	return strings.Join(l, ",")
}

func aliasesToQuery(aliases []blackbox.UserAlias) string {
	l := make([]string, len(aliases))
	for i, v := range aliases {
		l[i] = string(v)
	}

	return strings.Join(l, ",")
}

func uidsToQuery(uids []blackbox.ID) string {
	l := make([]string, len(uids))
	for i, v := range uids {
		l[i] = strconv.FormatUint(v, 10)
	}

	return strings.Join(l, ",")
}

func maskSessionID(sessionID string) string {
	pos := strings.LastIndex(sessionID, ".")
	if pos == -1 {
		return "XXXXX"
	}
	return sessionID[:pos+1]
}

func maskTvmTicket(ticket string) string {
	pos := strings.LastIndex(ticket, ":")
	if pos == -1 {
		return "XXXXX"
	}
	return ticket[:pos+1]
}
