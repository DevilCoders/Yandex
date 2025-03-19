package tvmtool

import (
	"context"
	"encoding/json"
	"io/ioutil"
	"net/http"
	"net/url"
	"strings"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/auth/tvm"
	"a.yandex-team.ru/cloud/mdb/internal/config"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

// Client is thread-safe client for interacting with TVM via tvmtool (local deamon).
type Client struct {
	alias string
	token string
	addr  *url.URL

	l log.Logger

	pingRequest *http.Request
}

// Config for constructing tmvtool client
type Config struct {
	// Alias of source service (service A)
	Alias string `json:"alias" yaml:"alias"`
	Token string `json:"token" yaml:"token"`
	URI   string `json:"uri" yaml:"uri"`
}

func (c *Config) TokenFromEnv(name string) bool {
	return config.LoadEnvToString(name, &c.Token)
}

// NewFromConfig creates new Client from config
func NewFromConfig(cfg Config, l log.Logger) (*Client, error) {
	if !cfg.TokenFromEnv("TVM_TOKEN") {
		l.Warn("env var TVM_TOKEN is empty")
	}

	return New(cfg.Alias, cfg.Token, cfg.URI, l)
}

// New creates new TVM-tool client.
func New(alias, token, uri string, l log.Logger) (*Client, error) {
	addr, err := url.Parse(uri)
	if err != nil {
		return nil, err
	}

	c := &Client{
		alias: alias,
		token: token,
		addr:  addr,
		l:     l,
	}

	c.pingRequest = &http.Request{
		Method: http.MethodGet,
		URL: &url.URL{
			Scheme: addr.Scheme,
			Host:   addr.Host,
			Path:   "/tvm/ping",
		},
		Header: http.Header{"Authorization": []string{token}},
		Host:   addr.Host,
	}

	return c, nil
}

func (c *Client) do(req *http.Request) (*http.Response, time.Duration, error) {
	startTS := time.Now()
	resp, err := http.DefaultClient.Do(req)
	return resp, time.Since(startTS), err
}

// Ping sends ping request to tvm daemon and returns non-nil error on
// no 200 OK response.
func (c *Client) Ping(ctx context.Context) error {
	resp, elapsed, err := c.do(c.pingRequest.WithContext(ctx))
	if err != nil {
		return err
	}
	defer func() { _ = resp.Body.Close() }()

	ctxlog.Debug(ctx, c.l, "Ping response",
		log.String("target", "tvmtool"),
		log.String("http_status", resp.Status),
		log.Int("http_code", resp.StatusCode),
		log.Duration("latency", elapsed),
	)

	err = c.handlePingResponse(resp)
	if err != nil {
		ctxlog.Warn(ctx, c.l, "Ping error", log.Error(err))
	}

	return err
}

func (c *Client) handlePingResponse(resp *http.Response) error {
	if resp.StatusCode != http.StatusOK {
		b, err := ioutil.ReadAll(resp.Body)
		if err != nil {
			return xerrors.Errorf("tvm: ping: response status: [%d] %s", resp.StatusCode, resp.Status)
		}

		return xerrors.Errorf("tvm: ping: [%d] %s: %s", resp.StatusCode, resp.Status, strings.TrimSpace(string(b)))
	}

	return nil
}

// GetServiceTicket fetches service ticket for tvmAlias from TVM
func (c *Client) GetServiceTicket(ctx context.Context, alias string) (string, error) {
	req := &http.Request{
		Method: http.MethodGet,
		URL: &url.URL{
			Scheme: "http",
			Host:   c.addr.Host,
			Path:   "/tvm/tickets",
			RawQuery: url.Values{
				"dsts": []string{alias},
				"src":  []string{c.alias},
			}.Encode(),
		},
		Header: http.Header{tvm.HeaderAuthorization: []string{c.token}},
		Host:   c.addr.Host,
	}
	req = req.WithContext(ctx)

	resp, elapsed, err := c.do(req)
	if err != nil {
		return "", err
	}
	defer func() { _ = resp.Body.Close() }()

	ctxlog.Debug(ctx, c.l, "GetServiceTicket response",
		log.String("http_status", resp.Status),
		log.Int("http_code", resp.StatusCode),
		log.Duration("latency", elapsed),
	)

	ticket, err := c.handleServiceTicketResponse(resp, alias)
	if err != nil {
		ctxlog.Warn(ctx, c.l, "GetServiceTicketFor error", log.Error(err))
	}

	return ticket, err
}

func (c *Client) handleServiceTicketResponse(resp *http.Response, alias string) (string, error) {
	if resp.StatusCode != http.StatusOK {
		b, err := ioutil.ReadAll(resp.Body)
		if err != nil {
			return "", xerrors.Errorf("tvm: tickets: [%d] %s", resp.StatusCode, resp.Status)
		}

		return "", xerrors.Errorf("tvm: tickets: [%d] %s: %s", resp.StatusCode, resp.Status, strings.TrimSpace(string(b)))
	}

	var tickets map[string]struct {
		Ticket string  `json:"ticket"`
		ID     uint32  `json:"tvm_id"`
		Error  *string `json:"error"`
	}
	dec := json.NewDecoder(resp.Body)
	if err := dec.Decode(&tickets); err != nil {
		return "", xerrors.Errorf("tvm: tickets: invalid json: %v", err)
	}

	ticket, ok := tickets[alias]
	if !ok {
		return "", xerrors.New("tvm: unexpected response")
	}

	if ticket.Error != nil {
		return "", xerrors.Errorf("tvm: tickets: %s", strings.TrimSpace(*ticket.Error))
	}

	return ticket.Ticket, nil
}

// CheckServiceTicket checks ticket in TVM and returns ServiceTicketInfo
func (c *Client) CheckServiceTicket(ctx context.Context, alias, ticket string) (tvm.ServiceTicket, error) {
	if alias == "" {
		alias = c.alias
	}

	req := &http.Request{
		Method: http.MethodGet,
		URL: &url.URL{
			Scheme: "http",
			Host:   c.addr.Host,
			Path:   "/tvm/checksrv",
			RawQuery: url.Values{
				"dst": []string{alias},
			}.Encode(),
		},
		Header: http.Header{
			tvm.HeaderAuthorization:    []string{c.token},
			tvm.HeaderXYaServiceTicket: []string{ticket},
		},
		Host: c.addr.Host,
	}
	req = req.WithContext(ctx)

	resp, elapsed, err := c.do(req)
	if err != nil {
		return tvm.ServiceTicket{}, err
	}

	ctxlog.Debug(ctx, c.l, "CheckServiceTicket response",
		log.String("http_status", resp.Status),
		log.Int("http_code", resp.StatusCode),
		log.Duration("latency", elapsed),
	)

	st, err := c.handleCheckServiceResponse(resp)
	if err != nil {
		ctxlog.Debug(ctx, c.l, "CheckServiceTicket error", log.Error(err))
	}
	return st, err
}

func (c *Client) handleCheckServiceResponse(resp *http.Response) (tvm.ServiceTicket, error) {
	if resp.StatusCode == http.StatusOK || resp.StatusCode == http.StatusForbidden {
		var st tvm.ServiceTicket
		dec := json.NewDecoder(resp.Body)
		if err := dec.Decode(&st); err != nil {
			return tvm.ServiceTicket{}, xerrors.Errorf("tvm: checksrv: invalid json: %v", err)
		}

		if st.Error != nil {
			return tvm.ServiceTicket{}, xerrors.Errorf("tvm: checksrv: %s", strings.TrimSpace(*st.Error))
		}

		return st, nil
	}

	b, err := ioutil.ReadAll(resp.Body)
	if err != nil {
		return tvm.ServiceTicket{}, xerrors.Errorf("tvm: checksrv: [%d] %s", resp.StatusCode, resp.Status)
	}

	return tvm.ServiceTicket{}, xerrors.Errorf("tvm: checksrv: [%d] %s: %s", resp.StatusCode, resp.Status, strings.TrimSpace(string(b)))
}
