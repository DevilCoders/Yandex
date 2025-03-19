package postgresql

import (
	"context"

	pgv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/postgresql/v1"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api"
	grpcapi "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/logs"
)

// ClusterService implements DB-specific gRPC methods
type ClusterService struct {
	pgv1.UnimplementedClusterServiceServer

	cs *grpcapi.ClusterService
}

var _ pgv1.ClusterServiceServer = &ClusterService{}

func NewClusterService(cs *grpcapi.ClusterService) *ClusterService {
	return &ClusterService{cs: cs}
}

func (cs *ClusterService) ListLogs(ctx context.Context, req *pgv1.ListClusterLogsRequest) (*pgv1.ListClusterLogsResponse, error) {
	lst, err := ListLogsServiceTypeFromGRPC(req.GetServiceType())
	if err != nil {
		return nil, err
	}

	logsList, token, more, err := cs.cs.ListLogs(ctx, lst, req)
	if err != nil {
		return nil, err
	}

	var respLogs []*pgv1.LogRecord
	for _, l := range logsList {
		respLogs = append(respLogs, &pgv1.LogRecord{Timestamp: grpcapi.TimeToGRPC(l.Timestamp), Message: l.Message})
	}

	resp := &pgv1.ListClusterLogsResponse{Logs: respLogs}
	if more || req.GetAlwaysNextPageToken() {
		resp.NextPageToken = api.PagingTokenToGRPC(token)
	}

	return resp, nil
}

func (cs *ClusterService) StreamLogs(req *pgv1.StreamClusterLogsRequest, stream pgv1.ClusterService_StreamLogsServer) error {
	lst, err := StreamLogsServiceTypeFromGRPC(req.GetServiceType())
	if err != nil {
		return err
	}

	return cs.cs.StreamLogs(
		stream.Context(),
		lst,
		req,
		func(l logs.Message) error {
			return stream.Send(
				&pgv1.StreamLogRecord{
					Record: &pgv1.LogRecord{
						Timestamp: grpcapi.TimeToGRPC(l.Timestamp),
						Message:   l.Message,
					},
					NextRecordToken: api.PagingTokenToGRPC(l.NextMessageToken),
				},
			)
		},
	)
}
