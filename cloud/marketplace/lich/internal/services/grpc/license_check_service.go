package grpc

import (
	"context"

	"google.golang.org/grpc"
	"google.golang.org/protobuf/types/known/emptypb"

	"a.yandex-team.ru/cloud/marketplace/lich/internal/core/actions"
	"a.yandex-team.ru/cloud/marketplace/lich/internal/services/env"
	"a.yandex-team.ru/cloud/marketplace/pkg/ctxtools"

	mkt "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/marketplace/v1/license"
)

type licenseCheckService struct {
	baseService
}

func newLicenseCheckService(env *env.Env) *licenseCheckService {
	return &licenseCheckService{
		baseService: baseService{
			Env: env,
		},
	}
}

func (s *licenseCheckService) Check(ctx context.Context, request *mkt.CheckProductLicenseRequest) (*emptypb.Empty, error) {
	ctxtools.Logger(ctx).Debug("running license check method")

	_, err := actions.NewLicenseCheckAction(s.Env).Do(ctx, actions.LicenseCheckParams{
		CloudID:     request.CloudId,
		ProductsIDs: request.ProductIds,
	})

	if err := s.mapActionError(ctx, err); err != nil {
		return nil, err
	}

	return EmptyPB, nil
}

func (s *licenseCheckService) bind(server *grpc.Server) {
	mkt.RegisterProductLicenseServiceServer(server, s)
}
