package grpcserver

import (
	"context"

	grpchealth "google.golang.org/grpc/health/grpc_health_v1"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/cmsdb"
	"a.yandex-team.ru/cloud/mdb/internal/fs"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/library/go/core/log"
)

// HealthService provides grpc-ping handles
type HealthService struct {
	Cmsdb  cmsdb.Client
	Closer fs.FileWatcher
	Logger log.Logger
}

var _ grpchealth.HealthServer = &HealthService{}

// Check is a standard grpc health check handle
func (health *HealthService) Check(ctx context.Context, _ *grpchealth.HealthCheckRequest) (*grpchealth.HealthCheckResponse, error) {
	if err := health.Cmsdb.IsReady(ctx); err != nil {
		health.Logger.Errorf("Not serving because cmsdb is down: %v", err)
		return &grpchealth.HealthCheckResponse{Status: grpchealth.HealthCheckResponse_NOT_SERVING}, nil
	}

	if health.Closer.Exists() {
		health.Logger.Warn("Not serving because close file exists")
		return &grpchealth.HealthCheckResponse{Status: grpchealth.HealthCheckResponse_NOT_SERVING}, nil
	}

	return &grpchealth.HealthCheckResponse{Status: grpchealth.HealthCheckResponse_SERVING}, nil
}

// Watch is a streaming handle and it is not implemented here
func (health *HealthService) Watch(*grpchealth.HealthCheckRequest, grpchealth.Health_WatchServer) error {
	return semerr.NotImplemented("watch is not implemented")
}
