package http

import (
	"bytes"
	"context"
	"encoding/json"
	"io/ioutil"
	"net/http"
	"time"

	"github.com/karlseguin/ccache/v2"

	"a.yandex-team.ru/cloud/mdb/internal/compute/billing"
	"a.yandex-team.ru/cloud/mdb/internal/httputil"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type metricsResp struct {
	Metrics []metric `json:"metrics"`
}

type metric struct {
	FolderID string                     `json:"folder_id"`
	Schema   string                     `json:"schema"`
	Tags     map[string]json.RawMessage `json:"tags"`
}

type computeInstanceSpec struct {
	FolderID              string                 `json:"folder_id"`
	PlatformID            string                 `json:"platform_id"`
	ResourcesSpec         resourcesSpec          `json:"resources_spec"`
	BootDiskSpec          bootDiskSpec           `json:"boot_disk_spec"`
	ZoneID                *string                `json:"zone_id,omitempty"`
	NetworkInterfaceSpecs []networkInterfaceSpec `json:"network_interface_specs,omitempty"`
}

type resourcesSpec struct {
	Memory       uint64 `json:"memory"`
	Cores        int    `json:"cores"`
	CoreFraction int    `json:"core_fraction"`
	GPUs         int    `json:"gpus"`
}

type bootDiskSpec struct {
	DiskSpec diskSpec `json:"disk_spec"`
}

type diskSpec struct {
	TypeID  *string `json:"type_id,omitempty"`
	Size    *uint64 `json:"size,omitempty"`
	ImageID string  `json:"image_id"`
}

type networkInterfaceSpec struct {
	SubnetID *string `json:"subnet_id,omitempty"`
}

// Client implements API to compute billing
type Client struct {
	l   log.Logger
	api *http.Client

	host        string
	caPath      string
	token       string
	logHTTPBody bool

	cacheTTL  time.Duration
	cacheSize int64
	cache     *ccache.Cache
}

type ClientOption func(*Client) error

// CAPath sets client CA path option
func CAPath(capath string) ClientOption {
	return func(cl *Client) error {
		cl.caPath = capath
		return nil
	}
}

// Token sets client token option
func Token(token string) ClientOption {
	return func(cl *Client) error {
		cl.token = token
		return nil
	}
}

// LogHTTPBody enables http body logging
func LogHTTPBody(log bool) ClientOption {
	return func(cl *Client) error {
		cl.logHTTPBody = log
		return nil
	}
}

// CacheTTL sets cache entry ttl in ns
func CacheTTL(ttl time.Duration) ClientOption {
	return func(cl *Client) error {
		cl.cacheTTL = ttl
		return nil
	}
}

// CacheSize sets max object size in cache
func CacheSize(size int64) ClientOption {
	return func(cl *Client) error {
		cl.cacheSize = size
		return nil
	}
}

func getLogTransport(cl *Client) (http.RoundTripper, error) {
	return httputil.DEPRECATEDNewTransport(
		httputil.TLSConfig{CAFile: cl.caPath},
		httputil.LoggingConfig{LogRequestBody: cl.logHTTPBody, LogResponseBody: cl.logHTTPBody},
		cl.l,
	)
}

// New constructs new http.client to compute billing
func New(host string, l log.Logger, options ...ClientOption) (billing.Client, error) {
	client := &Client{
		l:         l,
		host:      host,
		cacheTTL:  5 * time.Minute,
		cacheSize: 1024,
	}
	for _, optFunc := range options {
		if err := optFunc(client); err != nil {
			return nil, err
		}
	}

	logTransport, err := getLogTransport(client)
	if err != nil {
		return nil, err
	}

	client.api = &http.Client{Transport: logTransport}
	client.cache = ccache.New(ccache.Configure().MaxSize(client.cacheSize))

	return client, nil
}

func (c *Client) Metrics(ctx context.Context, instance billing.ComputeInstance) ([]billing.Metric, error) {
	spec := computeInstanceSpecFromReq(instance)
	jsonReq, err := json.Marshal(spec)
	if err != nil {
		return nil, xerrors.Errorf("failed to marshal json request: %w", err)
	}

	cacheKey := string(jsonReq)
	item := c.cache.Get(cacheKey)
	if item != nil && !item.Expired() {
		return item.Value().([]billing.Metric), nil
	}

	resp, err := c.doPost(ctx, "console/simulateInstanceBillingMetrics", jsonReq)
	if err != nil {
		return nil, xerrors.Errorf("failed post-request: %w", err)
	}
	var jsonResp metricsResp
	if err := json.Unmarshal(resp, &jsonResp); err != nil {
		return nil, xerrors.Errorf("failed to unmarshal json response: %w", err)
	}
	metrics := metricsFromResp(jsonResp)
	c.cache.Set(cacheKey, metrics, c.cacheTTL)
	return metrics, nil
}

func (c *Client) headers(req *http.Request) {
	ct := "application/json"
	req.Header.Set("Accept", ct)
	req.Header.Set("Content-type", ct)
	req.Header.Set("X-YaCloud-SubjectToken", c.token)
}

func (c *Client) doPost(ctx context.Context, url string, data []byte) ([]byte, error) {
	r := bytes.NewReader(data)
	fullURL := c.host + "/" + url
	req, err := http.NewRequest(http.MethodPost, fullURL, r)
	if err != nil {
		return nil, xerrors.Errorf("failed to build HTTP POST request %s: %w", fullURL, err)
	}
	c.headers(req)
	resp, err := c.api.Do(req.WithContext(ctx))
	if err != nil {
		return nil, xerrors.Errorf("failed to make HTTP POST request %s: %w", fullURL, err)
	}
	defer func() { _ = resp.Body.Close() }()

	body, err := ioutil.ReadAll(resp.Body)
	if err != nil {
		return nil, xerrors.Errorf("failed to read body (error code %d) from POST %s: %w", resp.StatusCode, fullURL, err)
	}

	if resp.StatusCode != http.StatusOK {
		return nil, xerrors.Errorf("unexpected status code for POST %s: %d", fullURL, resp.StatusCode)
	}

	return body, nil
}

func metricsFromResp(resp metricsResp) []billing.Metric {
	metrics := make([]billing.Metric, 0, len(resp.Metrics))
	for _, m := range resp.Metrics {
		metrics = append(metrics, billing.Metric(m))
	}
	return metrics
}

func computeInstanceSpecFromReq(in billing.ComputeInstance) computeInstanceSpec {
	return computeInstanceSpec{
		FolderID:   in.FolderID,
		PlatformID: in.PlatformID,
		ResourcesSpec: resourcesSpec{
			Memory:       in.Resources.Memory,
			Cores:        in.Resources.Cores,
			CoreFraction: in.Resources.CoreFraction,
			GPUs:         in.Resources.GPUs,
		},
		BootDiskSpec: bootDiskSpec{
			DiskSpec: diskSpec{
				TypeID:  in.BootDisk.TypeID,
				Size:    in.BootDisk.Size,
				ImageID: in.BootDisk.ImageID,
			},
		},
		ZoneID:                in.ZoneID,
		NetworkInterfaceSpecs: []networkInterfaceSpec{{SubnetID: in.SubnetID}},
	}
}
