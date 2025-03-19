package http

import (
	"bytes"
	"context"
	"encoding/json"
	"fmt"
	"net/http"
	"time"

	"github.com/opentracing/opentracing-go"

	"a.yandex-team.ru/cloud/mdb/internal/secret"
	"a.yandex-team.ru/cloud/mdb/mdb-dns/pkg/dnsapi"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

var _ dnsapi.Client = &client{}

// DefaultConfig default config
func DefaultConfig() dnsapi.Config {
	return dnsapi.Config{
		Baseurl:    "https://dns-api.yandex.net",
		Account:    "robot-dnsapi-mdb",
		FQDNSuffix: "db.yandex.net",
		ResolveNS:  "ns-cache.yandex.net",
		MaxRec:     25,
		UpdThrs:    8,
	}
}

const (
	primitivesOpsTemplate = "%s/v2.3/%s/primitives"
)

type client struct {
	logger  log.Logger
	rt      http.RoundTripper
	baseurl string
	account string
	token   secret.String
	ttl     time.Duration
}

// Primitive base format for DNS API request, description of operation
type Primitive struct {
	Operation string `json:"operation"`
	Name      string `json:"name"`
	OpType    string `json:"type"`
	Data      string `json:"data"`
	TTL       uint   `json:"ttl"`
}

// ReqPrimitives base format for DNS API request, list of operations
type ReqPrimitives struct {
	Ops []Primitive `json:"primitives"`
}

// New constructs new client
func New(
	l log.Logger,
	rt http.RoundTripper,
	config dnsapi.Config,
	ttl time.Duration,
) dnsapi.Client {
	return &client{
		logger:  l,
		rt:      rt,
		baseurl: config.Baseurl,
		account: config.Account,
		token:   config.Token,
		ttl:     ttl,
	}
}

// UpdateRecords process update info for DNS API
func (c *client) UpdateRecords(ctx context.Context, up *dnsapi.RequestUpdate) ([]*dnsapi.RequestUpdate, error) {
	span, ctx := opentracing.StartSpanFromContext(ctx, "DNS API UpdateRecords")
	defer span.Finish()

	p := ReqPrimitives{
		Ops: make([]Primitive, 0, len(up.Records)),
	}
	for cidfqdn, upInfo := range up.Records {
		if upInfo.CNAMEOld != "" {
			p.Ops = append(p.Ops, Primitive{
				Operation: "delete",
				Name:      cidfqdn,
				OpType:    "CNAME",
				Data:      upInfo.CNAMEOld,
				TTL:       uint(c.ttl.Seconds()),
			})
		}
		if upInfo.CNAMENew != "" {
			p.Ops = append(p.Ops, Primitive{
				Operation: "add",
				Name:      cidfqdn,
				OpType:    "CNAME",
				Data:      upInfo.CNAMENew,
				TTL:       uint(c.ttl.Seconds()),
			})
		}
	}

	body, err := json.Marshal(p)
	if err != nil {
		return nil, dnsapi.ErrInternal.Wrap(xerrors.Errorf("failed to prepare request body: %w", err))
	}

	rbody := bytes.NewReader(body)
	url := fmt.Sprintf(primitivesOpsTemplate, c.baseurl, c.account)
	c.logger.Debugf("send update request DNS to url: %s, with body: %s", url, body)

	req, err := http.NewRequestWithContext(ctx, "PUT", url, rbody)
	if err != nil {
		return nil, dnsapi.ErrInternal.Wrap(xerrors.Errorf("create new put request to DNS API failed: %w", err))
	}

	req.Header.Set("X-Auth-Token", c.token.Unmask())
	ct := "application/json"
	req.Header.Set("Accept", ct)
	req.Header.Set("Content-Type", ct)

	resp, err := c.rt.RoundTrip(req)
	if err != nil {
		return nil, dnsapi.ErrRequestFailed.Wrap(xerrors.Errorf("put request to DNS API failed: %w", err))
	}
	defer func() { _ = resp.Body.Close() }()

	ahost := resp.Header.Get("X-Api-Host")
	if rc := resp.StatusCode; rc != 200 {
		err = dnsapi.ErrRequestFailed.Wrap(xerrors.Errorf("put request to DNS API response code: %d, DNS API host: %s, body: %s", rc, ahost, body))
		if rc != 409 {
			return nil, err
		}
		return dnsapi.SplitOnParts(up), err
	}

	c.logger.Debugf("DNS API host: %s", ahost)

	return nil, nil
}
