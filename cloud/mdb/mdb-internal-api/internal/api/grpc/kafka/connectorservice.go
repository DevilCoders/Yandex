package kafka

import (
	"context"

	kfv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/kafka/v1"
	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/operation"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api"
	grpcapi "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/kafka"
	"a.yandex-team.ru/library/go/core/log"
)

// ConnectorService implements gRPC methods for user management.
type ConnectorService struct {
	kfv1.UnimplementedConnectorServiceServer

	Kafka kafka.Kafka
	L     log.Logger
}

var _ kfv1.ConnectorServiceServer = &ConnectorService{}

func (cs *ConnectorService) Create(ctx context.Context, req *kfv1.CreateConnectorRequest) (*operation.Operation, error) {
	op, err := cs.Kafka.CreateConnector(ctx, req.GetClusterId(), ConnectorSpecFromGRPC(req.GetConnectorSpec()))
	if err != nil {
		return nil, err
	}

	return grpcapi.OperationToGRPC(ctx, op, cs.L)
}

func (cs *ConnectorService) Update(ctx context.Context, req *kfv1.UpdateConnectorRequest) (*operation.Operation, error) {
	args, err := UpdateConnectorArgsFromGRPC(req)
	if err != nil {
		return nil, err
	}
	op, err := cs.Kafka.UpdateConnector(ctx, args)
	if err != nil {
		return nil, err
	}

	return grpcapi.OperationToGRPC(ctx, op, cs.L)
}

func (cs *ConnectorService) Get(ctx context.Context, req *kfv1.GetConnectorRequest) (*kfv1.Connector, error) {
	cn, err := cs.Kafka.Connector(ctx, req.GetClusterId(), req.GetConnectorName())
	if err != nil {
		return nil, err
	}

	res := ConnectorToGRPC(cn)
	res.ClusterId = req.GetClusterId()
	return res, nil
}

func (cs *ConnectorService) List(ctx context.Context, req *kfv1.ListConnectorsRequest) (*kfv1.ListConnectorsResponse, error) {
	offset, _, err := api.PageTokenFromGRPC(req.GetPageToken())
	if err != nil {
		return nil, err
	}

	cns, err := cs.Kafka.Connectors(ctx, req.GetClusterId(), req.GetPageSize(), offset)
	if err != nil {
		return nil, err
	}

	connectors := ConnectorsToGRPC(cns)
	for _, connector := range connectors {
		connector.ClusterId = req.GetClusterId()
	}
	return &kfv1.ListConnectorsResponse{
		Connectors: connectors,
	}, nil
}

func (cs *ConnectorService) Delete(ctx context.Context, req *kfv1.DeleteConnectorRequest) (*operation.Operation, error) {
	op, err := cs.Kafka.DeleteConnector(ctx, req.GetClusterId(), req.GetConnectorName())
	if err != nil {
		return nil, err
	}

	return grpcapi.OperationToGRPC(ctx, op, cs.L)
}

func (cs *ConnectorService) Pause(ctx context.Context, req *kfv1.PauseConnectorRequest) (*operation.Operation, error) {
	op, err := cs.Kafka.PauseConnector(ctx, req.GetClusterId(), req.GetConnectorName())
	if err != nil {
		return nil, err
	}

	return grpcapi.OperationToGRPC(ctx, op, cs.L)
}

func (cs *ConnectorService) Resume(ctx context.Context, req *kfv1.ResumeConnectorRequest) (*operation.Operation, error) {
	op, err := cs.Kafka.ResumeConnector(ctx, req.GetClusterId(), req.GetConnectorName())
	if err != nil {
		return nil, err
	}

	return grpcapi.OperationToGRPC(ctx, op, cs.L)
}
