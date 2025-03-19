package mem

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/mdb-dns/pkg/dnsq"
	"a.yandex-team.ru/library/go/core/log"
)

var _ dnsq.Client = &Client{}

// Client base struct of DNS Alternative Library
type Client struct {
	logger log.Logger
	mem    map[string]string
}

// New create new instance of DNS Query in mem implementation
func New(logger log.Logger) *Client {
	return &Client{
		logger: logger,
		mem:    make(map[string]string),
	}
}

// UpdateCNAME set new cname for fqdn
func (c *Client) UpdateCNAME(ctx context.Context, fqdn string, cname string) {
	if cname == "" {
		delete(c.mem, fqdn)
	} else {
		c.mem[fqdn] = cname
	}
}

// LookupCNAME implement lookup cname
func (c *Client) LookupCNAME(ctx context.Context, fqdn, groupid string) (cname string, ok bool) {
	return c.mem[fqdn], true
}
