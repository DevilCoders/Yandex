package kafka

import (
	"context"

	kfv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/kafka/v1"
	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/operation"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api"
	grpcapi "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/kafka"
	kafkamodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/kafka"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/pagination"
	"a.yandex-team.ru/library/go/core/log"
)

// TopicService implements topic-specific gRPC methods
type TopicService struct {
	kfv1.UnimplementedTopicServiceServer

	Kafka kafka.Kafka
	L     log.Logger
}

var _ kfv1.TopicServiceServer = &TopicService{}

func (ts *TopicService) Get(ctx context.Context, req *kfv1.GetTopicRequest) (*kfv1.Topic, error) {
	db, err := ts.Kafka.Topic(ctx, req.GetClusterId(), req.GetTopicName())
	if err != nil {
		return nil, err
	}

	return TopicToGRPC(db), nil
}

func (ts *TopicService) List(ctx context.Context, req *kfv1.ListTopicsRequest) (*kfv1.ListTopicsResponse, error) {
	var pageToken kafkamodels.TopicPageToken
	err := api.ParsePageTokenFromGRPC(req.GetPageToken(), &pageToken)
	if err != nil {
		return nil, err
	}

	pageSize := pagination.SanePageSize(req.GetPageSize())

	topics, topicPageToken, err := ts.Kafka.Topics(ctx, req.GetClusterId(), pageSize, pageToken)
	if err != nil {
		return nil, err
	}

	nextPageToken, err := api.BuildPageTokenToGRPC(topicPageToken, false)
	if err != nil {
		return nil, err
	}

	return &kfv1.ListTopicsResponse{
		Topics:        TopicsToGRPC(topics),
		NextPageToken: nextPageToken,
	}, nil
}

func (ts *TopicService) Create(ctx context.Context, req *kfv1.CreateTopicRequest) (*operation.Operation, error) {
	op, err := ts.Kafka.CreateTopic(ctx, req.GetClusterId(), TopicSpecFromGRPC(req.GetTopicSpec()))
	if err != nil {
		return nil, err
	}

	return grpcapi.OperationToGRPC(ctx, op, ts.L)
}

func (ts *TopicService) Update(ctx context.Context, req *kfv1.UpdateTopicRequest) (*operation.Operation, error) {
	args, err := updateTopicArgsFromGRPC(req)
	if err != nil {
		return nil, err
	}
	op, err := ts.Kafka.UpdateTopic(ctx, args)
	if err != nil {
		return nil, err
	}

	return grpcapi.OperationToGRPC(ctx, op, ts.L)
}

func (ts *TopicService) Delete(ctx context.Context, req *kfv1.DeleteTopicRequest) (*operation.Operation, error) {
	op, err := ts.Kafka.DeleteTopic(ctx, req.GetClusterId(), req.GetTopicName())
	if err != nil {
		return nil, err
	}

	return grpcapi.OperationToGRPC(ctx, op, ts.L)
}
