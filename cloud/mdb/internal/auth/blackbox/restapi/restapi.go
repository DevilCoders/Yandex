package restapi

import (
	"context"
	"encoding/json"
	"errors"
	"fmt"
	"net"
	"net/http"
	"net/url"
	"strings"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/auth/blackbox"
	"a.yandex-team.ru/cloud/mdb/internal/auth/tvm"
	xurl "a.yandex-team.ru/cloud/mdb/internal/x/net/url"
	"a.yandex-team.ru/library/go/core/log"
)

// Known errors
var (
	ErrInvalidSessionID = errors.New("invalid Session_id")
	ErrInvalidToken     = errors.New("invalid OAuth token")
	ErrNeedReset        = errors.New("need reset")
)

const (
	statusValid     = "VALID"
	statusNeedReset = "NEED_RESET"
	statusExpired   = "EXPIRED"
	statusNoAuth    = "NOAUTH"
	statusInvalid   = "INVALID"
)

type status struct {
	ID    int    `json:"id"`
	Value string `json:"value"`
}

type uid struct {
	Value string `json:"value"`
}

// Client ...
type Client struct {
	addr *url.URL
	l    log.Logger
}

var _ blackbox.Client = &Client{}

const (
	// IntranetURL is URL of internal blackbox
	IntranetURL = "https://blackbox.yandex-team.ru"
)

// New constructs blackbox client
func New(uri string, l log.Logger) (*Client, error) {
	addr, err := url.Parse(uri)
	if err != nil {
		return nil, err
	}

	return &Client{
		addr: addr,
		l:    l,
	}, nil
}

func (c *Client) do(req *http.Request) (*http.Response, time.Duration, error) {
	startTS := time.Now()
	resp, err := http.DefaultClient.Do(req)
	return resp, time.Since(startTS), err
}

// Ping checks if blackbox is alive via opening pure tcp connection.
// Blackbox' /ping handle errors too often to be reliable.
func (c *Client) Ping(ctx context.Context) error {
	var dialer net.Dialer
	conn, err := dialer.DialContext(ctx, "tcp", xurl.AddrFromURL(c.addr))
	if err != nil {
		return err
	}
	defer conn.Close()
	return nil
}

// SessionID https://doc.yandex-team.ru/blackbox/reference/MethodSessionID.xml
func (c *Client) SessionID(ctx context.Context, tvmToken, sessionID, userip, host string, dbfields []string) (blackbox.UserInfo, error) {
	if sessionID == "" {
		return blackbox.UserInfo{}, ErrInvalidSessionID
	}

	params := url.Values{
		"method":    {"sessionid"},
		"sessionid": {sessionID},
		"userip":    {userip},
		"host":      {host},
		"format":    {"json"},
	}

	if len(dbfields) > 0 {
		params["dbfields"] = []string{strings.Join(dbfields, ",")}
	}

	req := &http.Request{
		Method: http.MethodGet,
		URL: &url.URL{
			Scheme:   c.addr.Scheme,
			Host:     c.addr.Host,
			Path:     "/blackbox",
			RawQuery: params.Encode(),
		},
		Header: http.Header{tvm.HeaderXYaServiceTicket: []string{tvmToken}},
		Host:   c.addr.Host,
	}
	req = req.WithContext(ctx)

	resp, _, err := c.do(req)
	if err != nil {
		if urlerr, ok := err.(*url.Error); ok {
			// remove url with sessionid from error message
			err = urlerr.Err
		}

		return blackbox.UserInfo{}, err
	}
	defer func() { _ = resp.Body.Close() }()

	var data struct {
		Age         int                  `json:"age"`
		DBFields    map[string]string    `json:"dbfields"`
		DisplayName blackbox.DisplayName `json:"display_name"`
		Error       string               `json:"error"`
		Login       string               `json:"login"`
		Status      status               `json:"status"`
		TTL         string               `json:"ttl"`
		UID         uid                  `json:"uid"`
	}
	err = json.NewDecoder(resp.Body).Decode(&data)
	if err != nil {
		return blackbox.UserInfo{}, fmt.Errorf("invalid json: %s", err)
	}

	// https://wiki.yandex-team.ru/passport/mda#opisanieprotokola
	switch data.Status.Value {
	case statusValid:
		return blackbox.UserInfo{
			DBFields:    data.DBFields,
			DisplayName: data.DisplayName,
			Login:       data.Login,
			UID:         data.UID.Value,
		}, nil
	case statusNeedReset:
		return blackbox.UserInfo{}, ErrNeedReset
	case statusExpired:
	case statusNoAuth:
	case statusInvalid:
		return blackbox.UserInfo{}, fmt.Errorf("bad status %s: %s", data.Status.Value, data.Error)
	}

	return blackbox.UserInfo{}, fmt.Errorf("%s: %s", data.Status.Value, data.Error)
}

// OAuth https://doc.yandex-team.ru/blackbox/reference/method-oauth.xml
func (c *Client) OAuth(ctx context.Context, tvmToken, token, userip string, dbfields []string) (blackbox.UserInfo, error) {
	if token == "" {
		return blackbox.UserInfo{}, ErrInvalidToken
	}

	params := url.Values{
		"method":      {"oauth"},
		"oauth_token": {token},
		"userip":      {userip},
		"format":      {"json"},
	}

	if len(dbfields) > 0 {
		params["dbfields"] = []string{strings.Join(dbfields, ",")}
	}

	req := &http.Request{
		Method: http.MethodGet,
		URL: &url.URL{
			Scheme:   c.addr.Scheme,
			Host:     c.addr.Host,
			Path:     "/blackbox",
			RawQuery: params.Encode(),
		},
		Header: http.Header{tvm.HeaderXYaServiceTicket: []string{tvmToken}},
		Host:   c.addr.Host,
	}
	req = req.WithContext(ctx)

	resp, _, err := c.do(req)
	if err != nil {
		if urlerr, ok := err.(*url.Error); ok {
			// remove url with token from error message
			err = urlerr.Err
		}

		return blackbox.UserInfo{}, err
	}
	defer func() { _ = resp.Body.Close() }()

	var data struct {
		OAuth struct {
			UID        string `json:"uid"`
			ClientName string `json:"client_name"`
			ClientID   string `json:"client_id"`
			Scope      string `json:"scope"`
		} `json:"oauth"`
		DBFields    map[string]string    `json:"dbfields"`
		DisplayName blackbox.DisplayName `json:"display_name"`
		Error       string               `json:"error"`
		Login       string               `json:"login"`
		Status      status               `json:"status"`
		UID         uid                  `json:"uid"`
	}
	err = json.NewDecoder(resp.Body).Decode(&data)
	if err != nil {
		return blackbox.UserInfo{}, fmt.Errorf("invalid json: %s", err.Error())
	}

	switch data.Status.Value {
	case statusValid:
		return blackbox.UserInfo{
			DBFields:    data.DBFields,
			DisplayName: data.DisplayName,
			Login:       data.Login,
			UID:         data.UID.Value,
			Scope:       data.OAuth.Scope,
		}, nil
	case statusInvalid:
	default:
		return blackbox.UserInfo{}, fmt.Errorf("bad status %s: %s", data.Status.Value, data.Error)
	}

	return blackbox.UserInfo{}, fmt.Errorf("%s: %s", data.Status.Value, data.Error)
}
