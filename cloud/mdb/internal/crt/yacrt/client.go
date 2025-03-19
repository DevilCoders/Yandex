package yacrt

import (
	"fmt"
	"net/http"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/crt"
	"a.yandex-team.ru/cloud/mdb/internal/httputil"
	"a.yandex-team.ru/cloud/mdb/internal/secret"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

const (
	url              = `https://crt.yandex-team.ru/api/certificate/`
	issueSuccessCode = 201
	CertStatusIssued = "issued"

	CertTypeMDB  crt.CertificateType = "mdb"
	CertTypeHost crt.CertificateType = "host"
)

type Config struct {
	httputil.ClientConfig `json:"http_client" yaml:"http_client,omitempty"`
	AbcService            string        `json:"abc" yaml:"abc"`
	URL                   string        `json:"url" yaml:"url"`
	OAuth                 secret.String `json:"oauth" yaml:"oauth"`
}

// New constructs new certificator client
func New(l log.Logger, conf Config) (*Client, error) {
	if conf.OAuth.Unmask() == "" {
		return nil, xerrors.New("OAuth should not be empty")
	}
	client, err := httputil.NewClient(conf.ClientConfig, l)
	if err != nil {
		return nil, err
	}
	cli := &Client{
		auth:       conf.OAuth.Unmask(),
		abcService: conf.AbcService,
		url:        url,
		cli:        client,
	}

	if conf.URL != "" {
		cli.url = conf.URL
	}
	return cli, nil
}

// Client for certificator rest API
type Client struct {
	cli             *httputil.Client
	abcService      string
	auth            string
	defaultCertType string
	url             string
}

type certInfo struct {
	//omit other fields
	Download2        string    `json:"download2"`
	Status           string    `json:"status"`
	EndDate          time.Time `json:"end_date"`
	Revoked          string    `json:"revoked"`
	CaName           string    `json:"ca_name"`
	PrivKeyDeletedAt string    `json:"priv_key_deleted_at"`
	Hosts            []string  `json:"hosts"`
	APIURL           string    `json:"url"`
}

func (c *Client) addAuthHeaderVal(h http.Header) {
	h.Add("Authorization", fmt.Sprintf("OAuth %s", c.auth))
}

var _ crt.Client = &Client{}
