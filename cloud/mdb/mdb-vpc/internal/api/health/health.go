package health

import (
	"context"

	grpchealth "google.golang.org/grpc/health/grpc_health_v1"

	"a.yandex-team.ru/cloud/mdb/internal/fs"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb"
	"a.yandex-team.ru/library/go/core/log"
)

// Service provides grpc-ping handles
type Service struct {
	db     vpcdb.VPCDB
	closer fs.FileWatcher
	logger log.Logger
}

var _ grpchealth.HealthServer = &Service{}

// Check is a standard grpc health check handle
func (s *Service) Check(ctx context.Context, _ *grpchealth.HealthCheckRequest) (*grpchealth.HealthCheckResponse, error) {
	if err := s.db.IsReady(ctx); err != nil {
		s.logger.Errorf("Not serving because vpcdb is down: %v", err)
		return &grpchealth.HealthCheckResponse{Status: grpchealth.HealthCheckResponse_NOT_SERVING}, nil
	}

	if s.closer.Exists() {
		s.logger.Warn("Not serving because close file exists")
		return &grpchealth.HealthCheckResponse{Status: grpchealth.HealthCheckResponse_NOT_SERVING}, nil
	}

	return &grpchealth.HealthCheckResponse{Status: grpchealth.HealthCheckResponse_SERVING}, nil
}

// Watch is a streaming handle and it is not implemented here
func (s *Service) Watch(*grpchealth.HealthCheckRequest, grpchealth.Health_WatchServer) error {
	return semerr.NotImplemented("watch is not implemented")
}

func NewService(db vpcdb.VPCDB, closer fs.FileWatcher, l log.Logger) grpchealth.HealthServer {
	return &Service{
		db:     db,
		closer: closer,
		logger: l,
	}
}
