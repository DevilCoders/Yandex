package restapi

import (
	"net/url"

	"github.com/go-openapi/runtime"
	"github.com/go-openapi/strfmt"

	"a.yandex-team.ru/cloud/mdb/internal/auth/tvm"
	"a.yandex-team.ru/cloud/mdb/internal/crt"
	"a.yandex-team.ru/cloud/mdb/internal/crt/cloudcrt/generated/swagger/client"
	"a.yandex-team.ru/cloud/mdb/internal/httputil"
	"a.yandex-team.ru/cloud/mdb/internal/httputil/openapi"
	"a.yandex-team.ru/cloud/mdb/internal/secret"
	"a.yandex-team.ru/library/go/core/log"
)

const (
	certLivesDays = 365

	CertTypeCompute         crt.CertificateType = "ca"
	CertTypeGPN             crt.CertificateType = "gpn_ca"
	CertTypeIntermediateGPN crt.CertificateType = "gpn_int_ca"
)

type CertStatus string

const CertStatusIssued CertStatus = "issued"

type Config struct {
	URL          string                   `json:"url" yaml:"url"`
	AbcServiceID int                      `json:"abc_service_id" yaml:"abc_service_id"`
	OAuth        secret.String            `json:"oauth" yaml:"oauth"`
	Transport    httputil.TransportConfig `json:"transport" yaml:"transport"`
}

func DefaultConfig() Config {
	return Config{
		Transport: httputil.DefaultTransportConfig(),
	}
}

func NewFromConfig(config Config, l log.Logger) (*Client, error) {
	rt, err := httputil.NewTransport(config.Transport, l)
	if err != nil {
		return nil, err
	}
	//
	u, err := url.Parse(config.URL)
	if err != nil {
		return nil, err
	}
	clcrtrt := openapi.NewRuntime(
		u.Host,
		client.DefaultBasePath,
		[]string{u.Scheme},
		rt,
		l,
		openapi.WithConsumer("application/x-pem-file", runtime.TextConsumer()), // we download this Content-Type
	)

	return &Client{
		cfg: config,
		api: client.New(clcrtrt, strfmt.Default),
	}, nil
}

// Client for certificator rest API
type Client struct {
	cfg Config
	api *client.Cloudcrt
}

func (c *Client) addAuthHeaderVal() string {
	return tvm.FormatOAuthToken(c.cfg.OAuth.Unmask())
}

var _ crt.Client = &Client{}
