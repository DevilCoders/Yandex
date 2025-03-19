package swagger

import (
	"context"
	"net/http"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/crypto"
	"a.yandex-team.ru/cloud/mdb/internal/httputil"
	"a.yandex-team.ru/cloud/mdb/internal/httputil/openapi"
	swagclient "a.yandex-team.ru/cloud/mdb/mdb-health/generated/swagger/client"
	swagclusterh "a.yandex-team.ru/cloud/mdb/mdb-health/generated/swagger/client/clusterhealth"
	swaghealth "a.yandex-team.ru/cloud/mdb/mdb-health/generated/swagger/client/health"
	"a.yandex-team.ru/cloud/mdb/mdb-health/generated/swagger/client/hostneighbours"
	swaghh "a.yandex-team.ru/cloud/mdb/mdb-health/generated/swagger/client/hostshealth"
	swaglisthh "a.yandex-team.ru/cloud/mdb/mdb-health/generated/swagger/client/listhostshealth"
	swagmodels "a.yandex-team.ru/cloud/mdb/mdb-health/generated/swagger/models"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/client"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
	"a.yandex-team.ru/library/go/core/log"
)

// Config ...
type Config struct {
	Host string             `json:"host" yaml:"host"`
	TLS  httputil.TLSConfig `json:"tls" yaml:"tls"`
}

// DefaultConfig ...
func DefaultConfig() Config {
	return Config{}
}

type ClientOptions struct {
	Key *crypto.PrivateKey
}

// EnableBodySigning enable body signing
func EnableBodySigning(key *crypto.PrivateKey) Option {
	return func(args *ClientOptions) {
		args.Key = key
	}
}

type Option func(*ClientOptions)

type Client struct {
	logger log.Logger
	client *swagclient.MdbHealth
}

// NewClient constructs new mdb-health client
func NewClient(host string, key *crypto.PrivateKey, logger log.Logger, opts ...Option) (*Client, error) {
	o := &ClientOptions{}
	for _, optSetter := range opts {
		optSetter(o)
	}

	var err error
	transport := http.DefaultTransport
	if o.Key != nil {
		transport, err = httputil.NewRoundSigningTripper(transport, key, logger)
		if err != nil {
			return nil, err
		}
	}

	rt := openapi.NewRuntime(
		host,
		swagclient.DefaultBasePath,
		[]string{"http"},
		transport,
		logger,
	)

	return &Client{
		logger: logger,
		client: swagclient.New(
			rt,
			nil,
		),
	}, nil
}

// NewClientTLSFromConfig create new client with TLS from config
func NewClientTLSFromConfig(
	config Config,
	logger log.Logger,
	opts ...Option,
) (*Client, error) {
	o := &ClientOptions{}
	for _, optSetter := range opts {
		optSetter(o)
	}

	transport, err := httputil.DEPRECATEDNewTransport(config.TLS, httputil.LoggingConfig{}, logger)
	if err != nil {
		return nil, err
	}
	if o.Key != nil {
		transport, err = httputil.NewRoundSigningTripper(transport, o.Key, logger)
		if err != nil {
			return nil, err
		}
	}

	rt := openapi.NewRuntime(
		config.Host,
		swagclient.DefaultBasePath,
		[]string{"https"},
		transport,
		logger,
	)

	return &Client{
		logger: logger,
		client: swagclient.New(rt, nil),
	}, nil
}

// NewClientTLS constructs new mdb-health client with TLS support
func NewClientTLS(
	host,
	caCert string,
	logger log.Logger,
	opts ...Option,
) (*Client, error) {
	return NewClientTLSFromConfig(
		Config{
			Host: host,
			TLS:  httputil.TLSConfig{CAFile: caCert},
		},
		logger,
		opts...,
	)
}

func (cli *Client) Ping(ctx context.Context) error {
	if _, err := cli.client.Health.Ping(swaghealth.NewPingParamsWithContext(ctx)); err != nil {
		return translateError(err)
	}

	return nil
}

func (cli *Client) Stats(ctx context.Context) error {
	// TODO handle response, return stats
	if _, err := cli.client.Health.Stats(swaghealth.NewStatsParamsWithContext(ctx)); err != nil {
		return translateError(err)
	}

	return nil
}

func (cli *Client) GetHostsHealth(ctx context.Context, fqdns []string) ([]types.HostHealth, error) {
	req := swaglisthh.NewListHostsHealthParamsWithContext(ctx).WithBody(&swagmodels.HostsList{Hosts: fqdns})
	resp, err := cli.client.Listhostshealth.ListHostsHealth(req)
	if err != nil {
		return nil, translateError(err)
	}

	hhs := make([]types.HostHealth, 0, len(resp.Payload.Hosts))
	for _, hhresp := range resp.Payload.Hosts {
		hhs = append(hhs, newHostHealthFromModel(hhresp))
	}

	return hhs, nil
}

func (cli *Client) UpdateHostHealth(ctx context.Context, hh types.HostHealth, key *crypto.PrivateKey) error {
	req := swaghh.NewUpdateHostHealthParamsWithContext(ctx).WithBody(
		&swagmodels.HostHealthUpdate{
			Hosthealth: newHostHealthToModel(hh),
		},
	)

	if _, err := cli.client.Hostshealth.UpdateHostHealth(req); err != nil {
		return translateError(err)
	}

	return nil
}

func (cli *Client) GetClusterHealth(ctx context.Context, cid string) (client.ClusterHealth, error) {
	req := swagclusterh.NewGetClusterHealthParamsWithContext(ctx).WithCid(cid)

	resp, err := cli.client.Clusterhealth.GetClusterHealth(req)
	if err != nil {
		return client.ClusterHealth{}, translateError(err)
	}

	return newClusterHealthFromModel(resp.Payload)
}

func (cli *Client) GetHostNeighboursInfo(ctx context.Context, fqdns []string) (map[string]types.HostNeighboursInfo, error) {
	req := hostneighbours.NewGetHostNeighboursParamsWithContext(ctx)
	req.Fqdns = fqdns
	resp, err := cli.client.Hostneighbours.GetHostNeighbours(req)
	if err != nil {
		return nil, translateError(err)
	}
	result := map[string]types.HostNeighboursInfo{}
	for _, h := range resp.GetPayload().Hosts {
		result[h.Fqdn] = types.HostNeighboursInfo{
			Cid:            h.Cid,
			Sid:            h.Sid,
			Env:            h.Env,
			Roles:          h.Roles,
			HACluster:      h.Hacluster,
			HAShard:        h.Hashard,
			SameRolesTotal: int(h.Samerolestotal),
			SameRolesAlive: int(h.Samerolesalive),
			SameRolesTS:    time.Unix(h.Samerolestimestamp, 0),
		}
	}
	return result, nil
}

func translateError(err error) error {
	return openapi.TranslateError(
		err,
		openapi.Sentinels{
			NotFound:      client.ErrNotFound,
			BadRequest:    client.ErrBadRequest,
			InternalError: client.ErrInternalError,
		},
	)
}
