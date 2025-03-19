package cherrypy

import (
	"bytes"
	"context"
	"crypto/x509"
	"encoding/json"
	"fmt"
	"io/ioutil"
	"math"
	"net/http"
	"sync/atomic"
	"time"

	"github.com/opentracing/opentracing-go"

	"a.yandex-team.ru/cloud/mdb/internal/httputil"
	"a.yandex-team.ru/cloud/mdb/internal/saltapi"
	"a.yandex-team.ru/cloud/mdb/internal/saltapi/tracing/tags"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type Client struct {
	host string
	api  *httputil.Client
	l    log.Logger

	saltutil saltapi.SaltUtil
	state    saltapi.State
	test     saltapi.Test
	config   saltapi.Config
}

var _ saltapi.Client = &Client{}

type Auth struct {
	client *Client

	creds saltapi.Credentials

	reauthTokenTrigger chan struct{}
	tokens             atomic.Value
}

var _ saltapi.Auth = &Auth{}

func (a *Auth) Credentials() saltapi.Credentials {
	return a.creds
}

func (a *Auth) Tokens() saltapi.Tokens {
	return *a.tokens.Load().(*saltapi.Tokens)
}

func (a *Auth) Invalidate() {
	// Store invalid token
	a.invalidateToken()

	// Trigger reauthentication.
	// Channel might not exist or reader might be absent. We must continue in both cases.
	select {
	case a.reauthTokenTrigger <- struct{}{}:
	default:
	}
}

func (a *Auth) invalidateToken() {
	a.tokens.Store(&saltapi.Tokens{})
}

func (a *Auth) AuthSessionToken(ctx context.Context) (saltapi.Token, error) {
	s, err := a.client.authSessionToken(ctx, a.creds)
	if err != nil {
		switch e := err.(type) {
		case x509.UnknownAuthorityError:
			return saltapi.Token{}, xerrors.Errorf("failed to authenticate for session token with unknown cert: %w", e)
		default:
			return saltapi.Token{}, xerrors.Errorf("failed to authenticate for session token: %w", err)
		}
	}

	tokens := a.Tokens()
	tokens.Session = s
	a.tokens.Store(&tokens)
	return s, nil
}

func (a *Auth) AuthEAuthToken(ctx context.Context, timeout time.Duration) (saltapi.Token, error) {
	ea, err := a.client.authEAuthToken(ctx, a, timeout)
	if err != nil {
		switch e := err.(type) {
		case x509.UnknownAuthorityError:
			return saltapi.Token{}, xerrors.Errorf("failed to authenticate for session eauth with unknown cert: %w", e)
		default:
			return saltapi.Token{}, xerrors.Errorf("failed to authenticate for session eauth: %w", err)
		}
	}

	tokens := a.Tokens()
	tokens.EAuth = ea
	a.tokens.Store(&tokens)
	return ea, nil
}

// AutoAuth enables automatic authentication in the background.
//
// Stopped when supplied context is closed.
func (a *Auth) AutoAuth(ctx context.Context, reAuthInterval, authAttemptTimeout time.Duration, l log.Logger) {
	ctxlog.Debug(ctx, l, "Starting Salt API automatic authentication")
	defer ctxlog.Debug(ctx, l, "Stopped Salt API automatic authentication")

	// Create reauthentication trigger channels.
	// We do not close this channel because we do not control writers.
	a.reauthTokenTrigger = make(chan struct{})

	sessionTimer := time.NewTimer(0)
	eauthTimer := time.NewTimer(0)
	lastExternalReauthTrigger := time.Now()

	for {
		select {
		case <-ctx.Done():
			if !sessionTimer.Stop() {
				<-sessionTimer.C
			}
			if !eauthTimer.Stop() {
				<-eauthTimer.C
			}

			return
		case <-a.reauthTokenTrigger:
			now := time.Now()
			allowedInterval := reAuthInterval * 2
			if !now.After(lastExternalReauthTrigger.Add(allowedInterval)) {
				ctxlog.Debugf(ctx, l, "reauthentication requested but only %s passed since last request (need %s to pass before another request is allowed)",
					now.Sub(lastExternalReauthTrigger), allowedInterval)
				continue
			}

			ctxlog.Info(ctx, l, "triggered token reauthentication")
			lastExternalReauthTrigger = time.Now()
			sessionTimer.Reset(reAuthInterval)
			eauthTimer.Reset(reAuthInterval)
		case <-sessionTimer.C:
			token, err := a.AuthSessionToken(ctx)
			if err != nil {
				ctxlog.Errorf(ctx, l, "session token authentication error: %s", err)
				sessionTimer.Reset(reAuthInterval)
				continue
			}

			timeLeft := calcTokenRenewPeriod(time.Now(), token.ExpiresAt)
			ctxlog.Infof(ctx, l, "authenticated session token, expires at %s (in %s), renewal in %s", token.ExpiresAt, time.Until(token.ExpiresAt), timeLeft)
			sessionTimer.Reset(timeLeft)
		case <-eauthTimer.C:
			token, err := a.AuthEAuthToken(ctx, authAttemptTimeout)
			if err != nil {
				ctxlog.Errorf(ctx, l, "eauth token authentication error: %s", err)
				eauthTimer.Reset(reAuthInterval)
				continue
			}

			timeLeft := calcTokenRenewPeriod(time.Now(), token.ExpiresAt)
			ctxlog.Infof(ctx, l, "authenticated eauth token, expires at %s (in %s), renewal in %s", token.ExpiresAt, time.Until(token.ExpiresAt), timeLeft)
			eauthTimer.Reset(timeLeft)
		}
	}
}

// calcTokenRenewPeriod calculates time period after which token must be renewed
// If token duration is > hour, renew when 5% of time left
// If token duration is > minute, renew when 50% of time left
// Otherwise renew one second before token expires
func calcTokenRenewPeriod(now time.Time, expires time.Time) time.Duration {
	timeLeft := expires.Sub(now)

	if timeLeft > time.Hour {
		return timeLeft - timeLeft/20
	}

	if timeLeft > time.Minute {
		return timeLeft / 2
	}

	return time.Second
}

type ClientConfig struct {
	HTTP               httputil.HTTPConfig `yaml:"http"`
	Auth               saltapi.Credentials `yaml:"auth"`
	ReAuthPeriod       time.Duration       `yaml:"reauth_period"`
	AuthAttemptTimeout time.Duration       `yaml:"auth_attempt_timeout"`
}

func DefaultClientConfig() ClientConfig {
	return ClientConfig{
		ReAuthPeriod:       15 * time.Second,
		AuthAttemptTimeout: 15 * time.Second,
	}
}

// New constructs cherrypy-based salt-api client
func New(host string, tlscfg httputil.TLSConfig, logcfg httputil.LoggingConfig, l log.Logger) (*Client, error) {
	rt, err := httputil.DEPRECATEDNewTransport(tlscfg, logcfg, l)
	if err != nil {
		return nil, err
	}

	c := &Client{
		host: host,
		api:  httputil.DEPRECATEDNewClient(&http.Client{Transport: rt}, "Salt API CherryPy Client", l),
		l:    l,
	}

	c.saltutil = &SaltUtil{c: c}
	c.state = &State{c: c}
	c.test = &Test{c: c}
	c.config = &Config{c: c}
	return c, nil
}

func (c *Client) NewAuth(creds saltapi.Credentials) saltapi.Auth {
	a := &Auth{
		client: c,
		creds:  creds,
	}

	a.invalidateToken()
	return a
}

func (c *Client) authSessionToken(ctx context.Context, creds saltapi.Credentials) (saltapi.Token, error) {
	reqbody, err := json.Marshal(loginReq{User: creds.User, Password: creds.Password.Unmask(), EAuth: creds.EAuth})
	if err != nil {
		return saltapi.Token{}, err
	}

	req, err := http.NewRequest("POST", c.host+"/login", bytes.NewReader(reqbody))
	if err != nil {
		return saltapi.Token{}, err
	}

	req.Header.Set("Accept", "application/json")
	req.Header.Set("Content-type", "application/json")

	resp, err := c.api.Do(req.WithContext(ctx), "Salt API Login")
	if err != nil {
		return saltapi.Token{}, err
	}
	defer func() { _ = resp.Body.Close() }()

	respbin, err := ioutil.ReadAll(resp.Body)
	if err != nil {
		return saltapi.Token{}, xerrors.Errorf("failed to read body (error code %d): %w", resp.StatusCode, err)
	}

	if resp.StatusCode != http.StatusOK {
		return saltapi.Token{}, xerrors.New(string(respbin))
	}

	var respbody loginResp
	if err = json.Unmarshal(respbin, &respbody); err != nil {
		return saltapi.Token{}, xerrors.Errorf("failed to read login response body %q: %w", respbin, err)
	}

	v := respbody.Return[0]
	return newToken(v.Token, v.Start, v.Expire), nil
}

func newToken(token string, start, expire float64) saltapi.Token {
	startTS := time.Unix(int64(start), 0)
	expireTS := time.Unix(int64(expire), 0)
	ttl := expireTS.Sub(startTS)
	return saltapi.Token{Value: token, ExpiresAt: time.Now().Add(ttl)}
}

type loginReq struct {
	User     string `json:"username"`
	Password string `json:"password"`
	EAuth    string `json:"eauth"`
}

type loginResp struct {
	Return []loginReturnResp `json:"return"`
}

func (c *Client) authEAuthToken(ctx context.Context, sec saltapi.Secrets, timeout time.Duration) (saltapi.Token, error) {
	ret, err := c.run(
		ctx,
		sec,
		timeout,
		runArgs{
			client:                          clientTypeRunner,
			fun:                             "auth.mk_token",
			args:                            []string{"username=" + sec.Credentials().User, "password=" + sec.Credentials().Password.Unmask(), "eauth=" + sec.Credentials().EAuth},
			useAuthCreds:                    true,
			doNotInvalidateTokenOnAuthError: true,
		},
	)
	if err != nil {
		return saltapi.Token{}, err
	}

	var resp authEAuthTokenResp
	if err = json.Unmarshal(ret, &resp); err != nil {
		return saltapi.Token{}, xerrors.Errorf("failed to read auth eauth token response body %q: %w", ret, err)
	}

	return newToken(resp.Token, resp.Start, resp.Expire), nil
}

type authEAuthTokenResp struct {
	Name   string   `json:"name"`
	Start  float64  `json:"start"`
	Token  string   `json:"token"`
	Expire float64  `json:"expire"`
	EAuth  string   `json:"eauth"`
	Groups []string `json:"groups"`
}

func (c *Client) Ping(ctx context.Context, sec saltapi.Secrets) error {
	// TODO: подумать как лучше сделать пингование. Прямо сейчас по сути пингуем только сам мастер никак не проверяя
	// что он может делать
	req, err := http.NewRequest("GET", c.host+"/", nil)
	if err != nil {
		return err
	}

	req.Header.Set("X-Auth-Token", sec.Tokens().Session.Value)
	req.Header.Set("Accept", "application/json")
	req.Header.Set("Content-type", "application/json")

	resp, err := c.api.Do(req.WithContext(ctx), "Salt API Ping")
	if err != nil {
		return err
	}
	defer func() { _ = resp.Body.Close() }()

	respbin, err := ioutil.ReadAll(resp.Body)
	if err != nil {
		return xerrors.Errorf("failed to read body (error code %d): %w", resp.StatusCode, err)
	}

	if resp.StatusCode != http.StatusOK {
		return xerrors.New(string(respbin))
	}

	return nil
}

type loginReturnResp struct {
	Token  string  `json:"token"`
	Start  float64 `json:"start"`
	Expire float64 `json:"expire"`
	User   string  `json:"user"`
	EAuth  string  `json:"eauth"`
	// TODO: does not always match type, ignore for now (not used anyway)
	// Perms []string `json:"perms"`
}

func (c *Client) Minions(ctx context.Context, sec saltapi.Secrets) ([]saltapi.Minion, error) {
	req, err := http.NewRequest("GET", c.host+"/minions", nil)
	if err != nil {
		return nil, err
	}

	req.Header.Set("X-Auth-Token", sec.Tokens().Session.Value)
	req.Header.Set("Accept", "application/json")
	req.Header.Set("Content-type", "application/json")

	resp, err := c.api.Do(req.WithContext(ctx), "Salt API Minions")
	if err != nil {
		return nil, err
	}
	defer func() { _ = resp.Body.Close() }()

	respbin, err := ioutil.ReadAll(resp.Body)
	if err != nil {
		return nil, xerrors.Errorf("failed to read body (error code %d): %w", resp.StatusCode, err)
	}

	if resp.StatusCode != http.StatusOK {
		return nil, xerrors.New(string(respbin))
	}

	var respbody minionsResp
	if err = json.Unmarshal(respbin, &respbody); err != nil {
		return nil, xerrors.Errorf("failed to read minions response body %q: %w", respbin, err)
	}

	if len(respbody.Return) == 0 {
		return nil, nil
	}

	minions := make([]saltapi.Minion, 0, len(respbody.Return[0]))
	for name, minionResp := range respbody.Return[0] {
		var m minionsReturnResp
		if err = json.Unmarshal(minionResp, &m); err != nil {
			var res bool
			if err = json.Unmarshal(minionResp, &res); err != nil {
				return nil, xerrors.Errorf("failed to read minion response body %q: %w", minionResp, err)
			}

			minions = append(minions, saltapi.Minion{FQDN: name, Accepted: false})
			continue
		}

		minions = append(minions, minionToModel(m))
	}

	return minions, nil
}

type minionsResp struct {
	Return []map[string]json.RawMessage `json:"return"`
}

type minionsReturnResp struct {
	FQDN       string `json:"fqdn"`
	MasterFQDN string `json:"master"`
}

func minionToModel(mrr minionsReturnResp) saltapi.Minion {
	return saltapi.Minion{FQDN: mrr.FQDN, MasterFQDN: mrr.MasterFQDN, Accepted: true}
}

func (c *Client) run(ctx context.Context, sec saltapi.Secrets, timeout time.Duration, args runArgs) (json.RawMessage, error) {
	runClient := args.client.String()
	if args.async {
		runClient += "_async"
	}

	// Calculate runner timeout. This timeout determines how long runner will wait for minion's response, NOT job's
	// execution timeout.
	timeoutSec := int(math.Ceil(timeout.Seconds()))
	if timeoutSec < 1 {
		return nil, xerrors.Errorf("calculated timeout %d < 1, requested timeout is %d", timeoutSec, timeout)
	}

	var cancel context.CancelFunc
	ctx, cancel = context.WithTimeout(ctx, 2*time.Duration(timeoutSec)*time.Second)
	defer cancel()

	rq := runReq{
		Client:   runClient,
		Target:   args.target,
		Fun:      args.fun,
		Args:     args.args,
		Timeout:  timeoutSec,
		Returner: args.ret,
	}

	if args.useAuthCreds {
		rq.User = sec.Credentials().User
		rq.Password = sec.Credentials().Password.Unmask()
		rq.EAuth = sec.Credentials().EAuth
	} else {
		rq.Token = sec.Tokens().EAuth.Value
	}

	reqbody, err := json.Marshal([]runReq{rq})
	if err != nil {
		return nil, err
	}

	req, err := http.NewRequest("POST", c.host+"/run", bytes.NewReader(reqbody))
	if err != nil {
		return nil, err
	}

	req.Header.Set("Accept", "application/json")
	req.Header.Set("Content-type", "application/json")

	ottags := []opentracing.Tag{
		tags.RunTarget.Tag(args.target),
		tags.JobFunc.Tag(args.fun),
		// TODO: think about reporting args, need to distinguish between safe and unsafe args
		// {Key: "args", Value: args.args}
	}

	resp, err := c.api.Do(
		req.WithContext(ctx),
		"Salt API Run",
		ottags...,
	)
	if err != nil {
		return nil, err
	}
	defer func() { _ = resp.Body.Close() }()

	respbin, err := ioutil.ReadAll(resp.Body)
	if err != nil {
		return nil, xerrors.Errorf("failed to read body (error code %d): %w", resp.StatusCode, err)
	}

	if resp.StatusCode != http.StatusOK {
		if !args.doNotInvalidateTokenOnAuthError && resp.StatusCode == http.StatusUnauthorized {
			sec.Invalidate()
		}

		return nil, xerrors.New(string(respbin))
	}

	var respbody modelResp
	if err = json.Unmarshal(respbin, &respbody); err != nil {
		return nil, xerrors.Errorf("failed to read run response body %q: %w", respbin, err)
	}

	if len(respbody.Return) == 0 {
		return nil, nil
	}

	return respbody.Return[0], nil
}

type clientType int

// Known client types
const (
	clientTypeLocal clientType = iota
	clientTypeRunner
	clientTypeWheel
)

func (ct clientType) String() string {
	switch ct {
	case clientTypeLocal:
		return "local"
	case clientTypeRunner:
		return "runner"
	case clientTypeWheel:
		return "wheel"
	default:
		panic(fmt.Sprintf("unknown client type: %d", ct))
	}
}

type runArgs struct {
	client                          clientType
	target                          string
	fun                             string
	args                            []string
	ret                             string
	async                           bool
	useAuthCreds                    bool
	doNotInvalidateTokenOnAuthError bool
}

type runReq struct {
	Client   string   `json:"client,omitempty"`
	Target   string   `json:"tgt,omitempty"`
	Fun      string   `json:"fun,omitempty"`
	Args     []string `json:"arg,omitempty"`
	Timeout  int      `json:"timeout,omitempty"`
	Returner string   `json:"ret,omitempty"`
	User     string   `json:"username,omitempty"`
	Password string   `json:"password,omitempty"`
	EAuth    string   `json:"eauth,omitempty"`
	Token    string   `json:"token,omitempty"`
}

type modelResp struct {
	Return []json.RawMessage `json:"return"`
}

type asyncRunResp struct {
	Jid     string   `json:"jid"`
	Minions []string `json:"minions"`
}

func (c *Client) SaltUtil() saltapi.SaltUtil {
	return c.saltutil
}

func (c *Client) Test() saltapi.Test {
	return c.test
}

func (c *Client) State() saltapi.State {
	return c.state
}

func (c *Client) Config() saltapi.Config {
	return c.config
}

func (c *Client) Run(ctx context.Context, sec saltapi.Secrets, timeout time.Duration, target, fun string, args ...string) error {
	_, err := c.run(ctx, sec, timeout, runArgs{target: target, fun: fun, args: args})
	return err
}

func (c *Client) AsyncRun(ctx context.Context, sec saltapi.Secrets, timeout time.Duration, target, fun string, args ...string) (string, error) {
	ret, err := c.run(ctx, sec, timeout, runArgs{async: true, target: target, fun: fun, args: args})
	if err != nil {
		return "", err
	}

	var resp asyncRunResp
	if err = json.Unmarshal(ret, &resp); err != nil {
		return "", xerrors.Errorf("read async run response body %q: %w", ret, err)
	}

	if resp.Jid == "" {
		return "", xerrors.Errorf("start async command: fun %q, args %q", fun, args)
	}

	return resp.Jid, nil
}

// parseMinionsList parses raw salt-api response that has a minion list with corresponding values.
// No key/value pair is returned for minions that did not return.
func parseMinionsList(raw json.RawMessage) (map[string]json.RawMessage, error) {
	var res map[string]json.RawMessage
	if err := json.Unmarshal(raw, &res); err != nil {
		return nil, xerrors.Errorf("failed to unmarshal minions list %q: %w", raw, err)
	}

	for k, v := range res {
		// Listings return false as value if minion did not return
		var b bool
		if err := json.Unmarshal(v, &b); err != nil {
			// Value is not boolean which means it has some useful data which means we have to return it
			continue
		}

		delete(res, k)
	}

	return res, nil
}
