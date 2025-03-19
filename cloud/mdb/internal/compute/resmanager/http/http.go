package http

import (
	"context"
	"encoding/json"
	"fmt"
	"io/ioutil"
	"net/http"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/compute/resmanager"
	"a.yandex-team.ru/cloud/mdb/internal/httputil"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/nop"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type accessSubject struct {
	ID   string `json:"id"`
	Type string `json:"type"`
}

type accessBinding struct {
	Subject accessSubject `json:"subject"`
	RoleID  string        `json:"roleId"`
}

type accessBindingsResp struct {
	AccessBindings []accessBinding `json:"accessBindings"`
	NextPageToken  string          `json:"nextPageToken"`
}

type client struct {
	host        string // base endpoint including API version
	caPath      string
	l           log.Logger
	timeout     time.Duration
	api         *http.Client
	logHTTPBody bool
}

type clientOption func(*client)

// Logger sets the logger for the client
func Logger(l log.Logger) func(*client) {
	return func(c *client) {
		c.l = l
	}
}

// Timeout sets the timeout for the client
func Timeout(t time.Duration) func(*client) {
	return func(c *client) {
		c.timeout = t
	}
}

// CAPath sets the CA file's path for the client
func CAPath(caPath string) func(*client) {
	return func(c *client) {
		c.caPath = caPath
	}
}

// LogHTTPBody enables http body logging
func LogHTTPBody(log bool) func(*client) {
	return func(c *client) {
		c.logHTTPBody = log
	}
}

// NewClient creates provider for YC Network API
func NewClient(host string, opts ...clientOption) (resmanager.Client, error) {
	client := &client{
		host: host,
		l:    &nop.Logger{},
	}
	for _, opt := range opts {
		opt(client)
	}

	rt, err := httputil.DEPRECATEDNewTransport(
		httputil.TLSConfig{CAFile: client.caPath},
		httputil.LoggingConfig{LogRequestBody: client.logHTTPBody, LogResponseBody: client.logHTTPBody},
		client.l,
	)
	if err != nil {
		return nil, err
	}

	client.api = &http.Client{Transport: rt, Timeout: client.timeout}
	return client, nil
}

func (c *client) doGet(ctx context.Context, url, iamToken string, params map[string]string) ([]byte, error) {
	req, err := http.NewRequest(http.MethodGet, url, nil)
	if err != nil {
		return nil, err
	}
	req.Header.Set("Accept", "application/json")
	req.Header.Set("Content-Type", "application/json")
	req.Header.Set("Authorization", fmt.Sprintf("Bearer %v", iamToken))

	query := req.URL.Query()
	for name, value := range params {
		query.Add(name, value)
	}
	req.URL.RawQuery = query.Encode()
	resp, err := c.api.Do(req.WithContext(ctx))
	if err != nil {
		return nil, err
	}
	defer func() { _ = resp.Body.Close() }()

	body, err := ioutil.ReadAll(resp.Body)
	if err != nil {
		return nil, xerrors.Errorf("failed to read body (error code %d): %w", resp.StatusCode, err)
	}

	if resp.StatusCode != http.StatusOK {
		return nil, xerrors.Errorf("error code: %d, body: %s", resp.StatusCode, body)
	}
	return body, nil
}

type accessBindingsPager struct {
	HTTPGet       func(params map[string]string) ([]byte, error)
	NextPageToken string
}

func (p *accessBindingsPager) nextPage() ([]accessBinding, error) {
	params := map[string]string{
		"pageSize": "1000",
	}
	if p.NextPageToken != "" {
		params["pageToken"] = p.NextPageToken
	}
	responseBytes, err := p.HTTPGet(params)
	if err != nil {
		return []accessBinding{}, err
	}

	var resp accessBindingsResp
	if err = json.Unmarshal(responseBytes, &resp); err != nil {
		return []accessBinding{}, xerrors.Errorf("malformed responses: %w", err)
	}
	if len(resp.AccessBindings) > 0 {
		p.NextPageToken = resp.NextPageToken
	}
	return resp.AccessBindings, nil
}

func (c *client) getAccessBindingsPager(ctx context.Context, iamToken, folderID string) *accessBindingsPager {
	url := fmt.Sprintf("%s/folders/%s:listAccessBindings", c.host, folderID)
	return &accessBindingsPager{HTTPGet: func(params map[string]string) (bytes []byte, e error) {
		return c.doGet(ctx, url, iamToken, params)
	}}
}

// CheckServiceAccountRole verifies if service account has role within folder
func (c *client) CheckServiceAccountRole(ctx context.Context, iamToken, folderID, serviceAccountID, roleID string) (bool, error) {
	pager := c.getAccessBindingsPager(ctx, iamToken, folderID)
	neededBinding := accessBinding{Subject: accessSubject{ID: serviceAccountID, Type: "serviceAccount"}, RoleID: roleID}
	for {
		bindings, err := pager.nextPage()
		if err != nil {
			return false, err
		}
		if len(bindings) == 0 {
			break
		}
		for _, binding := range bindings {
			if binding == neededBinding {
				return true, nil
			}
		}
	}
	return false, nil
}

func (c *client) ListAccessBindings(ctx context.Context, resourceID string, private bool) ([]resmanager.AccessBinding, error) {
	return nil, semerr.NotImplemented("not implemented")
}

func (c *client) ResolveFolders(ctx context.Context, folderExtIDs []string) ([]resmanager.ResolvedFolder, error) {
	return nil, semerr.NotImplemented("not implemented")
}

func (c *client) PermissionStages(ctx context.Context, cloudExtID string) ([]string, error) {
	return nil, semerr.NotImplemented("not implemented")
}

func (c *client) Cloud(ctx context.Context, cloudExtID string) (resmanager.Cloud, error) {
	return resmanager.Cloud{}, semerr.NotImplemented("not implemented")
}
