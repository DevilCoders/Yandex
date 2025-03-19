package server

import (
	"context"

	grpchealth "google.golang.org/grpc/health/grpc_health_v1"

	pb "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/dataproc/manager/v1"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
)

var _ grpchealth.HealthServer = &Server{}

// Ping handler
func (s *Server) Check(ctx context.Context, _ *grpchealth.HealthCheckRequest) (*grpchealth.HealthCheckResponse, error) {
	req := &pb.ClusterHealthRequest{Cid: "just_check_that_redis_returns_not_found_error"}
	_, err := s.ClusterHealth(ctx, req)
	if err != nil {
		semantic := semerr.AsSemanticError(err)
		if semantic != nil && semantic.Semantic == semerr.SemanticNotFound {
			return &grpchealth.HealthCheckResponse{Status: grpchealth.HealthCheckResponse_SERVING}, nil
		}

		s.logger.Errorf("ClusterHealth returned unexpected error: %s", err)
	} else {
		s.logger.Error("ClusterHealth unexpectedly didn't return any error")
	}

	return &grpchealth.HealthCheckResponse{Status: grpchealth.HealthCheckResponse_NOT_SERVING}, nil
}

func (s *Server) Watch(*grpchealth.HealthCheckRequest, grpchealth.Health_WatchServer) error {
	return semerr.NotImplemented("Watch is not implemented")
}
