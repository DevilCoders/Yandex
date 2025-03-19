package httpapi

import (
	"bufio"
	"context"
	"encoding/json"
	"fmt"
	"io"
	"io/ioutil"
	"net/http"
	"strings"

	"github.com/opentracing/opentracing-go"

	"a.yandex-team.ru/cloud/mdb/internal/conductor"
	"a.yandex-team.ru/cloud/mdb/internal/httputil"
	"a.yandex-team.ru/cloud/mdb/internal/tracing/tags"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

// Client ...
type Client struct {
	httpClient *httputil.Client
	l          log.Logger
	cfg        ConductorConfig
}

var _ conductor.Client = &Client{}

const (
	// URL of conductor
	URL = "https://c.yandex-team.ru"
)

func New(cfg ConductorConfig, l log.Logger) (*Client, error) {
	c, err := httputil.NewClient(cfg.Client, l)
	if err != nil {
		return nil, xerrors.Errorf("init http client: %w", err)
	}
	return &Client{httpClient: c, l: l, cfg: cfg}, nil
}

// GroupToHosts returns members of conductor group
func (c *Client) GroupToHosts(ctx context.Context, group string, attrs conductor.GroupToHostsAttrs) ([]string, error) {
	tracingTags := []opentracing.Tag{tags.ConductorGroup.Tag(group)}
	url := fmt.Sprintf("%s/api/groups2hosts/%s", URL, group)
	if attrs.DC.Valid {
		url = fmt.Sprintf("%s/?dc=%s", url, attrs.DC.String)
		tracingTags = append(tracingTags, tags.DataCenter.Tag(attrs.DC.String))
	}

	req, err := http.NewRequest("GET", url, nil)
	if err != nil {
		return nil, err
	}

	req = req.WithContext(ctx)

	resp, err := c.httpClient.Do(req, "Conductor Resolve Group to Hosts", tracingTags...)
	if err != nil {
		return nil, err
	}
	defer func() { _ = resp.Body.Close() }()

	if resp.StatusCode != http.StatusOK {
		b, _ := ioutil.ReadAll(resp.Body)
		return nil, xerrors.Errorf("request failed with code %d: %s", resp.StatusCode, b)

	}

	var hosts []string
	r := bufio.NewReader(resp.Body)
	for {
		l, err := readLine(r)
		if xerrors.Is(err, io.EOF) {
			break
		}

		hosts = append(hosts, string(l))
	}

	return hosts, nil
}

func (c *Client) HostToGroups(ctx context.Context, host string) ([]string, error) {
	url := fmt.Sprintf("%s/api/hosts2groups/%s", URL, host)
	req, err := http.NewRequest("GET", url, nil)
	if err != nil {
		return nil, err
	}

	req = req.WithContext(ctx)

	resp, err := c.httpClient.Do(req, "Conductor Resolve Host to Groups", tags.InstanceFQDN.Tag(host))
	if err != nil {
		return nil, err
	}
	defer func() { _ = resp.Body.Close() }()

	if resp.StatusCode != http.StatusOK {
		b, _ := ioutil.ReadAll(resp.Body)
		return nil, xerrors.Errorf("request failed with code %d: %s", resp.StatusCode, b)
	}

	var groups []string
	r := bufio.NewReader(resp.Body)
	for {
		l, err := readLine(r)
		if xerrors.Is(err, io.EOF) {
			break
		}

		groups = append(groups, string(l))
	}

	return groups, nil
}

func (c *Client) HostsInfo(ctx context.Context, hosts []string) ([]conductor.HostInfo, error) {
	url := fmt.Sprintf("%s/api/hosts/%s?format=json", URL, strings.Join(hosts, ","))
	req, err := http.NewRequest("GET", url, nil)
	if err != nil {
		return nil, err
	}

	req = req.WithContext(ctx)
	resp, err := c.httpClient.Do(req, "Conductor Get Hosts Info", tags.InstanceFQDNs.Tag(hosts))
	if err != nil {
		return nil, err
	}
	defer func() { _ = resp.Body.Close() }()

	if resp.StatusCode != http.StatusOK {
		if resp.StatusCode == http.StatusNotFound {
			return []conductor.HostInfo{}, nil
		}
		b, _ := ioutil.ReadAll(resp.Body)
		return nil, xerrors.Errorf("request failed with code %d: %s", resp.StatusCode, b)
	}

	var hostsInfo []conductor.HostInfo
	err = json.NewDecoder(resp.Body).Decode(&hostsInfo)
	if err != nil {
		return nil, fmt.Errorf("can not parse json response: %w", err)
	}

	return hostsInfo, nil
}

func readLine(r *bufio.Reader) ([]byte, error) {
	var line []byte
	for {
		l, p, err := r.ReadLine()
		if xerrors.Is(err, io.EOF) {
			return line, err
		}

		line = append(line, l...)
		if !p {
			return line, nil
		}
	}
}
