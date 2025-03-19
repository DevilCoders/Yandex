package grpc

import (
	"context"

	"github.com/opentracing/opentracing-go"
	"google.golang.org/grpc/credentials"

	cloudVPC "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/vpc/v1/inner"
	"a.yandex-team.ru/cloud/mdb/internal/compute/dns"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil/grpcerr"
	"a.yandex-team.ru/cloud/mdb/internal/tracing"
	"a.yandex-team.ru/cloud/mdb/internal/tracing/tags"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

var _ dns.Client = &client{}

type client struct {
	l   log.Logger
	ttl int64
	api cloudVPC.NetworkDnsServiceClient
}

func checkGrpcError(err error, span opentracing.Span) error {
	if err == nil {
		return nil
	}

	tracing.SetErrorOnSpan(span, err)

	return dns.ErrRequestFailed.Wrap(grpcerr.SemanticErrorFromGRPC(err))
}

// New create new instance of DNS Compute GRPC API
func New(ctx context.Context, l log.Logger, target, userAgent string, cfg grpcutil.ClientConfig, creds credentials.PerRPCCredentials, ttl uint) (dns.Client, error) {
	conn, err := grpcutil.NewConn(ctx, target, userAgent, cfg, l, grpcutil.WithClientCredentials(creds))
	if err != nil {
		return nil, xerrors.Errorf("connecting to VPC DNS API at %q: %w", target, err)
	}

	return &client{
		l:   l,
		ttl: int64(ttl),
		api: cloudVPC.NewNetworkDnsServiceClient(conn),
	}, nil
}

// SetCNAMERecord set CNAME record
func (c *client) SetCNAMERecord(ctx context.Context, netid, name, value string) error {
	span, ctx := opentracing.StartSpanFromContext(
		ctx,
		"Compute gRPC SetCNAMERecord",
		tags.StringTagName("fqdn.cname").Tag(name),
		tags.StringTagName("fqdn.host").Tag(value),
		tags.StringTagName("network.id").Tag(netid))
	defer span.Finish()
	_, err := c.api.SetDnsRecord(ctx, &cloudVPC.NetworkSetDnsRecordRequest{
		NetworkId: netid,
		Type:      "CNAME",
		Name:      name,
		Value:     value,
		Ttl:       c.ttl,
	})
	return checkGrpcError(err, span)
}

// DeleteCNAMERecord set CNAME record
func (c *client) DeleteCNAMERecord(ctx context.Context, netid, name string) error {
	span, ctx := opentracing.StartSpanFromContext(
		ctx,
		"Compute gRPC DeleteCNAMERecord",
		tags.StringTagName("fqdn.cname").Tag(name),
		tags.StringTagName("network.id").Tag(netid))
	defer span.Finish()
	_, err := c.api.DeleteDnsRecord(ctx, &cloudVPC.NetworkDeleteDnsRecordRequest{
		NetworkId: netid,
		Type:      "CNAME",
		Name:      name,
	})
	return checkGrpcError(err, span)
}

// ListRecords return map of records CNAME
func (c *client) ListRecords(ctx context.Context, netid string) (dns.MapRec, error) {
	span, ctx := opentracing.StartSpanFromContext(
		ctx,
		"Compute gRPC ListRecords",
		tags.StringTagName("network.id").Tag(netid))
	defer span.Finish()
	resp, err := c.api.ListDnsRecords(ctx, &cloudVPC.NetworkListDnsRecordsRequest{NetworkId: netid})
	if err = checkGrpcError(err, span); err != nil {
		return nil, err
	}

	cnameMap := make(map[string]dns.Rec)
	for _, dr := range resp.ExtraDnsRecords {
		cnameMap[dr.Name] = dns.Rec{
			Value: dr.Value,
			TTL:   uint(dr.Ttl),
		}
	}
	return cnameMap, nil
}
