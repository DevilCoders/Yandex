package http

import (
	"context"
	"encoding/json"
	"io/ioutil"
	"net/http"

	"gopkg.in/yaml.v2"

	"a.yandex-team.ru/cloud/mdb/dataproc-agent/pkg/metaparser"
	"a.yandex-team.ru/library/go/core/xerrors"
)

// Config describes configuration for http meta parser
type Config struct {
	URL string `json:"url" yaml:"url"`
}

// DefaultConfig describes configuration for http meta parser
func DefaultConfig() Config {
	return Config{
		URL: "http://169.254.169.254/computeMetadata/v1",
	}
}

type client struct {
	cfg Config
}

// New constructor for meta info getter
func New(cfg Config) metaparser.Metaparser {
	return &client{cfg: cfg}
}

var _ metaparser.Metaparser = &client{}

func doGet(ctx context.Context, url string) ([]byte, error) {
	client := &http.Client{}
	request, err := http.NewRequest("GET", url, nil)
	if err != nil {
		return nil, xerrors.Errorf("failed to build request: %w", err)
	}
	request.Header.Set("Metadata-Flavor", "Google")
	request = request.WithContext(ctx)

	resp, err := client.Do(request)
	if err != nil {
		return nil, xerrors.Errorf("failed to get status from server %q: %w", url, err)
	}
	defer func() { _ = resp.Body.Close() }()
	if resp.StatusCode != http.StatusOK {
		return nil, xerrors.Errorf("server responded with an error: %s", resp.Status)
	}
	body, err := ioutil.ReadAll(resp.Body)
	if err != nil {
		return nil, xerrors.Errorf("failed to read response body: %w", err)
	}
	return body, nil
}

type RawAttributes struct {
	UserDataYaml string `json:"user-data"`
	FolderID     string `json:"folder-id"`
}

type UserDataRoot struct {
	Data metaparser.UserData `yaml:"data"`
}

func (c *client) GetComputeMetadata(ctx context.Context) (metaparser.ComputeMetadata, error) {
	instanceAttributes, err := c.getInstanceAttributes(ctx)
	if err != nil {
		return metaparser.ComputeMetadata{}, xerrors.Errorf("failed to get instance attributes: %w", err)
	}

	token, err := c.getServiceAccountToken(ctx)
	if err != nil {
		return metaparser.ComputeMetadata{}, xerrors.Errorf("failed to get iam token: %w", err)
	}

	computeMetadata := metaparser.ComputeMetadata{
		InstanceAttributes: instanceAttributes,
		IAMToken:           token,
	}
	return computeMetadata, nil
}

func (c *client) getInstanceAttributes(ctx context.Context) (metaparser.InstanceAttributes, error) {
	url := c.cfg.URL + "/instance/attributes/?recursive=true&alt=json"
	attributesJSON, err := doGet(ctx, url)
	if err != nil {
		return metaparser.InstanceAttributes{}, err
	}

	rawAttributes := RawAttributes{}
	err = json.Unmarshal(attributesJSON, &rawAttributes)
	if err != nil {
		return metaparser.InstanceAttributes{}, xerrors.Errorf("failed to parse json: %w", err)
	}

	userDataRoot := UserDataRoot{}
	err = yaml.Unmarshal([]byte(rawAttributes.UserDataYaml), &userDataRoot)
	if err != nil {
		return metaparser.InstanceAttributes{}, xerrors.Errorf("failed to parse user data yaml: %w", err)
	}

	instanceAttributes := metaparser.InstanceAttributes{
		UserData: userDataRoot.Data,
		FolderID: rawAttributes.FolderID,
	}
	return instanceAttributes, nil
}

func (c *client) getServiceAccountToken(ctx context.Context) (string, error) {
	url := c.cfg.URL + "/instance/service-accounts/default/token"
	body, err := doGet(ctx, url)
	if err != nil {
		return "", err
	}

	type tokenInfo struct {
		Token string `json:"access_token"`
	}
	token := tokenInfo{}
	err = json.Unmarshal(body, &token)
	if err != nil {
		return "", xerrors.Errorf("failed to parse body: %w", err)
	}

	return token.Token, nil
}
