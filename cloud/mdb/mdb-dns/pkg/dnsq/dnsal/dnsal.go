package dnsal

import (
	"context"

	"github.com/miekg/dns"
	"github.com/opentracing/opentracing-go"

	"a.yandex-team.ru/cloud/mdb/internal/tracing"
	"a.yandex-team.ru/cloud/mdb/internal/tracing/tags"
	"a.yandex-team.ru/cloud/mdb/mdb-dns/pkg/dnsq"
	"a.yandex-team.ru/library/go/core/log"
)

var _ dnsq.Client = &Client{}

// Client base struct of DNS Alternative Library
type Client struct {
	logger log.Logger
	ns     string
	dnsc   dns.Client
}

// New create new instance of DNS Alternative Library
func New(logger log.Logger, ns string) dnsq.Client {
	return &Client{
		logger: logger,
		ns:     ns,
	}
}

// LookupCNAME implement lookup cname
func (c *Client) LookupCNAME(ctx context.Context, fqdn, groupid string) (cname string, ok bool) {
	span, ctx := opentracing.StartSpanFromContext(
		ctx,
		"DNSAL Lookup CNAME",
		tags.DNSNameserver.Tag(c.ns),
		tags.CNAMEFqdn.Tag(fqdn),
	)
	defer span.Finish()
	m := dns.Msg{}
	m.SetQuestion(fqdn+".", dns.TypeCNAME)
	r, _, err := c.dnsc.ExchangeContext(ctx, &m, c.ns+":53")
	if err != nil {
		tracing.SetErrorOnSpan(span, err)
		c.logger.Warnf("lookup CNAME request for FQDN: %s, error: %s", fqdn, err)
		return "", false
	}
	if len(r.Answer) == 0 {
		tags.TargetFqdn.Set(span, "")
		return "", true
	}
	if len(r.Answer) != 1 {
		c.logger.Warnf("got few cnames for FQDN: %s, records count: %d", fqdn, len(r.Answer))
	}
	cname = r.Answer[0].(*dns.CNAME).Target
	cname = cname[:len(cname)-1]
	tags.TargetFqdn.Set(span, cname)
	return cname, true
}
