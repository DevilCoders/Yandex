package compute

import (
	"context"

	"github.com/opentracing/opentracing-go"

	"a.yandex-team.ru/cloud/mdb/internal/compute/dns"
	"a.yandex-team.ru/cloud/mdb/internal/tracing"
	"a.yandex-team.ru/cloud/mdb/mdb-dns/pkg/dnsapi"
)

var _ dnsapi.Client = &client{}

type client struct {
	cc dns.Client
}

// New constructs new compute client
func New(cc dns.Client) dnsapi.Client {
	return &client{
		cc: cc,
	}
}

// UpdateRecords process update info for DNS API
func (c *client) UpdateRecords(ctx context.Context, up *dnsapi.RequestUpdate) ([]*dnsapi.RequestUpdate, error) {
	span, ctx := opentracing.StartSpanFromContext(ctx, "Compute DNS UpdateRecords")
	defer span.Finish()
	netid := up.GroupID
	for cidfqdn, upInfo := range up.Records {
		var err error
		if upInfo.CNAMENew == "" {
			err = c.cc.DeleteCNAMERecord(ctx, netid, cidfqdn)
		} else {
			err = c.cc.SetCNAMERecord(ctx, netid, cidfqdn, upInfo.CNAMENew)
		}
		if err != nil {
			tracing.SetErrorOnSpan(span, err)
			return nil, err
		}
	}

	return nil, nil
}
