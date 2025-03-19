package mysql

import (
	"context"

	mysqlv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/mysql/v1"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api"
	grpcapi "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/mysql"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/logs"
)

// ClusterService implements DB-specific gRPC methods
type ClusterService struct {
	mysqlv1.UnimplementedClusterServiceServer

	cs    *grpcapi.ClusterService
	mysql mysql.MySQL
}

var _ mysqlv1.ClusterServiceServer = &ClusterService{}

func NewClusterService(cs *grpcapi.ClusterService, mysql mysql.MySQL) *ClusterService {
	return &ClusterService{cs: cs, mysql: mysql}
}

func (cs *ClusterService) ListLogs(ctx context.Context, req *mysqlv1.ListClusterLogsRequest) (*mysqlv1.ListClusterLogsResponse, error) {
	lst, err := ListLogsServiceTypeFromGRPC(req.GetServiceType())
	if err != nil {
		return nil, err
	}

	logsList, token, more, err := cs.cs.ListLogs(ctx, lst, req)
	if err != nil {
		return nil, err
	}

	var respLogs []*mysqlv1.LogRecord
	for _, l := range logsList {
		respLogs = append(respLogs, &mysqlv1.LogRecord{Timestamp: grpcapi.TimeToGRPC(l.Timestamp), Message: l.Message})
	}

	resp := &mysqlv1.ListClusterLogsResponse{Logs: respLogs}
	if more || req.GetAlwaysNextPageToken() {
		resp.NextPageToken = api.PagingTokenToGRPC(token)
	}

	return resp, nil
}

func (cs *ClusterService) StreamLogs(req *mysqlv1.StreamClusterLogsRequest, stream mysqlv1.ClusterService_StreamLogsServer) error {
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
				&mysqlv1.StreamLogRecord{
					Record: &mysqlv1.LogRecord{
						Timestamp: grpcapi.TimeToGRPC(l.Timestamp),
						Message:   l.Message,
					},
					NextRecordToken: api.PagingTokenToGRPC(l.NextMessageToken),
				},
			)
		},
	)
}
