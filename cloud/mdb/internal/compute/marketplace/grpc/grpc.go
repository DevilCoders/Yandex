package grpc

import (
	"context"

	license "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/marketplace/v1/license"
	marketplace "a.yandex-team.ru/cloud/mdb/internal/compute/marketplace"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil"
	"a.yandex-team.ru/library/go/core/log"
)

type LicenseServiceClient struct {
	client license.ProductLicenseServiceClient
}

var _ marketplace.LicenseService = &LicenseServiceClient{}

// NewClient constructs new thread safe client
func NewClient(ctx context.Context, target, userAgent string, cfg grpcutil.ClientConfig, l log.Logger) (marketplace.LicenseService, error) {
	conn, err := grpcutil.NewConn(
		ctx,
		target,
		userAgent,
		cfg,
		l,
	)
	if err != nil {
		return nil, err
	}

	return &LicenseServiceClient{client: license.NewProductLicenseServiceClient(conn)}, nil
}

func (c *LicenseServiceClient) CheckLicenseResult(ctx context.Context, cloudID string, productIDs []string) error {
	req := license.CheckProductLicenseRequest{
		CloudId:    cloudID,
		ProductIds: productIDs,
	}

	_, err := c.client.Check(ctx, &req)

	return err
}
