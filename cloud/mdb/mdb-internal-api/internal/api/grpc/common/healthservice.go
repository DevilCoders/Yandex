package common

import (
	"context"

	grpchealth "google.golang.org/grpc/health/grpc_health_v1"

	"a.yandex-team.ru/cloud/mdb/internal/grpcutil/interceptors"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/common"
)

type HealthService struct {
	interceptors.AuthNeedChecker
	health common.Health
}

var _ grpchealth.HealthServer = &HealthService{}

func NewHealthService(health common.Health) *HealthService {
	return &HealthService{
		AuthNeedChecker: &interceptors.NoAuth{},
		health:          health,
	}
}

func (hs *HealthService) Check(ctx context.Context, _ *grpchealth.HealthCheckRequest) (*grpchealth.HealthCheckResponse, error) {
	if err := hs.health.IsReady(ctx); err != nil {
		return &grpchealth.HealthCheckResponse{Status: grpchealth.HealthCheckResponse_NOT_SERVING}, nil
	}

	return &grpchealth.HealthCheckResponse{Status: grpchealth.HealthCheckResponse_SERVING}, nil
}

func (hs *HealthService) Watch(*grpchealth.HealthCheckRequest, grpchealth.Health_WatchServer) error {
	return semerr.NotImplemented("watch is not implemented")
}
