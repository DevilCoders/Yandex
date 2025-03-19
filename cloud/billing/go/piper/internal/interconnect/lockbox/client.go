package lockbox

import (
	"context"
	"encoding/json"
	"errors"
	"fmt"
	"io/ioutil"
	"net/http"
	"strings"
	"time"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling/logf"
	"a.yandex-team.ru/cloud/billing/go/pkg/errsentinel"
)

var (
	ErrLockboxServiceBroken = errsentinel.New("lockbox service is broken")
	ErrNotFound             = errsentinel.New("secret not found in lockbox")
)

type Auth interface {
	GetToken() (string, error)
}

type Client struct {
	endpoint   string
	auth       Auth
	httpClient httpClient
}

func New(endpoint string, auth Auth) *Client {
	if !strings.HasSuffix(endpoint, "/") {
		endpoint += "/"
	}
	return &Client{
		endpoint: endpoint,
		auth:     auth,
		httpClient: &http.Client{
			Timeout: requestTimeout,
		},
	}
}

type httpClient interface {
	Do(req *http.Request) (*http.Response, error)
}

const (
	requestTimeout = time.Second * 5
)

type SecretValue struct {
	Text string
	Data []byte
}

func (c *Client) GetSecret(ctx context.Context, key string) (secret map[string]SecretValue, err error) {
	tooling.ICRequestWithNameStarted(ctx, "lockbox", "GetSecret")
	defer func() {
		if err != nil {
			err = ErrLockboxServiceBroken.Wrap(err)
		}
		tooling.ICRequestDone(ctx, err)
	}()

	url := fmt.Sprintf("%slockbox/v1/secrets/%s/payload", c.endpoint, key)

	logger := tooling.Logger(ctx)
	req, err := http.NewRequestWithContext(ctx, http.MethodGet, url, nil)
	if err != nil {
		return
	}
	if c.auth != nil {
		token, err := c.auth.GetToken()
		if err != nil {
			return nil, fmt.Errorf("auth: %w", err)
		}
		req.Header.Set("Authorization", fmt.Sprintf("Bearer %s", token))
	}

	resp, err := c.httpClient.Do(req)
	var body []byte
	if err == nil {
		body, err = ioutil.ReadAll(resp.Body)
		_ = resp.Body.Close()
	}

	if err == nil && resp.StatusCode == http.StatusOK {
		var version string
		version, secret, err = parseSecret(body)
		if err != nil {
			return nil, err
		}
		logger.Info("loaded secret version", logf.Value(version))
		return
	}
	if err != nil {
		err = fmt.Errorf("lockbox response: %w", err)
		return
	}

	if resp.StatusCode == http.StatusNotFound {
		err = ErrNotFound.Wrap(errors.New(key))
		return
	}

	logger.Error("wrong lockbox response", logf.HTTPStatus(resp.StatusCode), logf.HTTPBody(body))
	err = fmt.Errorf("response status %d", resp.StatusCode)
	return
}

type secretEntry struct {
	Key         string
	BinaryValue []byte
	TextValue   string
}

type secret struct {
	VersionID string
	Entries   []secretEntry
}

func parseSecret(data []byte) (string, map[string]SecretValue, error) {
	var resp secret

	if err := json.Unmarshal(data, &resp); err != nil {
		return "", nil, fmt.Errorf("body decode: %w", err)
	}
	result := make(map[string]SecretValue, len(resp.Entries))
	for _, e := range resp.Entries {
		result[e.Key] = SecretValue{
			Text: e.TextValue,
			Data: append([]byte(nil), e.BinaryValue...),
		}
	}
	return resp.VersionID, result, nil
}
