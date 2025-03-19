package http

import (
	"bytes"
	"context"
	"encoding/json"
	"io/ioutil"
	"net/http"

	"a.yandex-team.ru/cloud/mdb/internal/compute/iam"
	"a.yandex-team.ru/cloud/mdb/internal/compute/marketplace"
	"a.yandex-team.ru/cloud/mdb/internal/httputil"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

const (
	// API error codes from marketplace license check method
	licenseCheckCode         = "LICENSE_CHECK"
	licenseCheckExternalCode = "LICENSE_CHECK_EXTERNAL"
)

type ClientConfig struct {
	CaFile      string `json:"ca_file" yaml:"ca_file"`
	LogHTTPBody bool   `json:"log_http_body" yaml:"log_http_body"`
}

func DefaultClientConfig() ClientConfig {
	cfg := ClientConfig{}
	cfg.CaFile = "/opt/yandex/allCAs.pem"
	cfg.LogHTTPBody = false

	return cfg
}

// Client implements API to compute billing
type Client struct {
	l            log.Logger
	api          *http.Client
	tokenService iam.TokenService
	account      iam.ServiceAccount
	host         string
	config       ClientConfig
}

type licenseRequest struct {
	CloudID    string   `json:"cloud_id"`
	ProductIDs []string `json:"product_ids"`
}

type licenseResponse struct {
	Code    string `json:"code"`
	Message string `json:"message"`
}

// New constructs new http.client to compute billing
func NewClient(ctx context.Context, host string, l log.Logger,
	tokenService iam.TokenService, account iam.ServiceAccount, cfg ClientConfig) (marketplace.LicenseService, error) {

	client := &Client{
		l:            l,
		host:         host,
		tokenService: tokenService,
		account:      account,
		config:       cfg,
	}

	logTransport, err := getTransport(client)
	if err != nil {
		return nil, err
	}

	client.api = &http.Client{Transport: logTransport}

	return client, nil
}

func (c *Client) CheckLicenseResult(ctx context.Context, cloudID string, productIDs []string) error {
	req := makeRequest(cloudID, productIDs)
	jsonReq, err := json.Marshal(req)
	if err != nil {
		return xerrors.Errorf("failed to marshal json request: %w", err)
	}

	code, body, err := c.doPost(ctx, "marketplace/v1/license/check", jsonReq)
	if err != nil {
		return err
	}

	if code == http.StatusOK {
		return nil
	}

	c.l.Errorf("error checking license. Code: %d, body: %s", code, body)
	se := httputil.SemanticErrorFromHTTP(code, string(body))
	if semerr.IsUnavailable(se) {
		return semerr.Unavailable("license service unavailable")
	}

	// something went wrong
	var respErr licenseResponse
	if err := json.Unmarshal(body, &respErr); err != nil {
		return semerr.Internal("error checking license")
	}

	if respErr.Code == licenseCheckCode ||
		respErr.Code == licenseCheckExternalCode {
		return semerr.Authorization(respErr.Message)
	}

	return semerr.Internalf("error checking license: %s", respErr.Message)
}

func (c *Client) headers(req *http.Request, token string) {
	ct := "application/json"
	req.Header.Set("Accept", ct)
	req.Header.Set("Content-type", ct)
	req.Header.Set("X-YaCloud-SubjectToken", token)
}

func (c *Client) getToken(ctx context.Context) (string, error) {
	token, err := c.tokenService.ServiceAccountToken(ctx, c.account)
	if err != nil {
		return "", err
	}
	return token.Value, err
}

func (c *Client) doPost(ctx context.Context, url string, data []byte) (int, []byte, error) {
	r := bytes.NewReader(data)
	fullURL := c.host + "/" + url
	req, err := http.NewRequest(http.MethodPost, fullURL, r)
	if err != nil {
		return -1, nil, xerrors.Errorf("failed to build HTTP POST request %s: %w", fullURL, err)
	}
	token, err := c.getToken(ctx)
	if err != nil {
		return -1, nil, xerrors.Errorf("failed to obtain token: %w", err)
	}
	c.headers(req, token)
	resp, err := c.api.Do(req.WithContext(ctx))
	if err != nil {
		return -1, nil, xerrors.Errorf("failed to make HTTP POST request %s: %w", fullURL, err)
	}
	defer func() { _ = resp.Body.Close() }()

	body, err := ioutil.ReadAll(resp.Body)
	if err != nil {
		return -1, nil, xerrors.Errorf("failed to read body (error code %d) from POST %s: %w", resp.StatusCode, fullURL, err)
	}

	return resp.StatusCode, body, nil
}

func getTransport(cl *Client) (http.RoundTripper, error) {
	cfg := httputil.DefaultTransportConfig()
	cfg.TLS = httputil.TLSConfig{CAFile: cl.config.CaFile}
	cfg.Logging = httputil.LoggingConfig{LogRequestBody: cl.config.LogHTTPBody, LogResponseBody: cl.config.LogHTTPBody}

	return httputil.NewTransport(cfg, cl.l)
}

func makeRequest(cloudID string, productIDs []string) licenseRequest {
	return licenseRequest{
		CloudID:    cloudID,
		ProductIDs: productIDs,
	}
}
