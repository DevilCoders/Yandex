package cloudmeta

import (
	"context"
	"encoding/json"
	"errors"
	"fmt"
	"io/ioutil"
	"net/http"
	"time"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling/logf"
	"a.yandex-team.ru/cloud/billing/go/pkg/errsentinel"
)

var ErrMetadataServiceBroken = errsentinel.New("metadata service is broken")

type Client struct {
	endpoint string

	httpClient httpClient
}

func New() *Client {
	return new(remoteMetadataEndpoint)
}

func NewLocal() *Client {
	return new(localMetadataEndpoint)
}

type httpClient interface {
	Do(req *http.Request) (*http.Response, error)
}

func new(endpoint string) *Client {
	return &Client{
		endpoint: endpoint,
		httpClient: &http.Client{
			Timeout: requestTimeout,
		},
	}
}

const (
	// Local endpoint to get IAM token inside YC VM
	// https://cloud.yandex.ru/docs/compute/operations/vm-connect/auth-inside-vm
	localMetadataEndpoint  = "http://localhost:6770/"
	remoteMetadataEndpoint = "http://169.254.169.254/"

	tokenPath      = "computeMetadata/v1/instance/service-accounts/default/token"
	attributesPath = "computeMetadata/v1/instance/attributes/?recursive=true"

	requestTimeout = time.Millisecond * 500
)

type MetadataToken struct {
	AccessToken string
	ExpiresIn   time.Duration
}

func (c *Client) GetToken(ctx context.Context) (token MetadataToken, err error) {
	tooling.ICRequestWithNameStarted(ctx, "compute-metadata", "GetToken")
	defer func() {
		if err != nil {
			err = ErrMetadataServiceBroken.Wrap(err)
		}
		tooling.ICRequestDone(ctx, err)
	}()

	logger := tooling.Logger(ctx)
	req, err := http.NewRequestWithContext(ctx, http.MethodGet, c.endpoint+tokenPath, nil)
	if err != nil {
		return
	}
	req.Header.Set("Metadata-Flavor", "Google")
	resp, err := c.httpClient.Do(req)

	if err == nil && resp.StatusCode == http.StatusOK {
		body, _ := ioutil.ReadAll(resp.Body)
		token, err = parseToken(body)
		return
	}
	if err != nil {
		err = fmt.Errorf("token response: %w", err)
		return
	}

	body, _ := ioutil.ReadAll(resp.Body)
	logger.Error("wrong token response", logf.HTTPStatus(resp.StatusCode), logf.HTTPBody(body))
	err = fmt.Errorf("response status %d", resp.StatusCode)
	return
}

func (c *Client) GetAttributes(ctx context.Context) (attributes map[string]string, err error) {
	tooling.ICRequestWithNameStarted(ctx, "compute-metadata", "GetAttributes")
	defer func() {
		if err != nil {
			err = ErrMetadataServiceBroken.Wrap(err)
		}
		tooling.ICRequestDone(ctx, err)
	}()

	logger := tooling.Logger(ctx)
	req, err := http.NewRequestWithContext(ctx, http.MethodGet, c.endpoint+attributesPath, nil)
	if err != nil {
		return
	}
	req.Header.Set("Metadata-Flavor", "Google")
	resp, err := c.httpClient.Do(req)

	if err == nil && resp.StatusCode == http.StatusOK {
		body, _ := ioutil.ReadAll(resp.Body)
		attributes, err = parseAttributes(body)
		return
	}
	if err != nil {
		err = fmt.Errorf("attributes response: %w", err)
		return
	}

	body, _ := ioutil.ReadAll(resp.Body)
	logger.Error("wrong attributes response", logf.HTTPStatus(resp.StatusCode), logf.HTTPBody(body))
	err = fmt.Errorf("response status %d", resp.StatusCode)
	return
}

func parseToken(data []byte) (MetadataToken, error) {
	var resp struct {
		AccessToken string `json:"access_token"`
		ExpiresIn   int    `json:"expires_in"`
	}

	if err := json.Unmarshal(data, &resp); err != nil {
		return MetadataToken{}, fmt.Errorf("body decode: %w", err)
	}

	if resp.ExpiresIn == 0 {
		return MetadataToken{}, errors.New("empty expiration")
	}

	if resp.AccessToken == "" {
		return MetadataToken{}, errors.New("empty token")
	}

	return MetadataToken{
		AccessToken: resp.AccessToken,
		ExpiresIn:   time.Second * time.Duration(resp.ExpiresIn),
	}, nil
}

func parseAttributes(data []byte) (map[string]string, error) {
	var resp map[string]string

	if err := json.Unmarshal(data, &resp); err != nil {
		return nil, fmt.Errorf("body decode: %w", err)
	}

	return resp, nil
}
