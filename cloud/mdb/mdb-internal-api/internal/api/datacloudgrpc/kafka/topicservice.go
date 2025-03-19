package kafka

import (
	"context"

	kfv1 "a.yandex-team.ru/cloud/mdb/datacloud/private_api/datacloud/kafka/v1"
	apiv1 "a.yandex-team.ru/cloud/mdb/datacloud/private_api/datacloud/v1"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api"
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

	topic, err := TopicToGRPC(db)
	if err != nil {
		return nil, err
	}

	return topic, nil
}

func (ts *TopicService) List(ctx context.Context, req *kfv1.ListTopicsRequest) (*kfv1.ListTopicsResponse, error) {
	var pageToken kafkamodels.TopicPageToken
	err := api.ParsePageTokenFromGRPC(req.GetPaging().GetPageToken(), &pageToken)
	if err != nil {
		return nil, err
	}

	pageSize := pagination.SanePageSize(req.GetPaging().GetPageSize())

	topics, topicPageToken, err := ts.Kafka.Topics(ctx, req.GetClusterId(), pageSize, pageToken)
	if err != nil {
		return nil, err
	}

	nextPageToken, err := api.BuildPageTokenToGRPC(topicPageToken, false)
	if err != nil {
		return nil, err
	}

	grpcTopics, err := TopicsToGRPC(topics)
	if err != nil {
		return nil, err
	}

	return &kfv1.ListTopicsResponse{
		Topics: grpcTopics,
		NextPage: &apiv1.NextPage{
			Token: nextPageToken,
		},
	}, nil
}

func (ts *TopicService) Create(ctx context.Context, req *kfv1.CreateTopicRequest) (*kfv1.CreateTopicResponse, error) {
	topicSpec, err := TopicSpecFromGRPC(req.GetTopicSpec())
	if err != nil {
		return nil, err
	}

	op, err := ts.Kafka.CreateTopic(ctx, req.GetClusterId(), topicSpec)
	if err != nil {
		return nil, err
	}

	return &kfv1.CreateTopicResponse{
		OperationId: op.OperationID,
	}, nil
}

func (ts *TopicService) Update(ctx context.Context, req *kfv1.UpdateTopicRequest) (*kfv1.UpdateTopicResponse, error) {
	args, err := UpdateTopicArgsFromGRPC(req)
	if err != nil {
		return nil, err
	}
	op, err := ts.Kafka.UpdateTopic(ctx, args)
	if err != nil {
		return nil, err
	}

	return &kfv1.UpdateTopicResponse{
		OperationId: op.OperationID,
	}, nil
}

func (ts *TopicService) Delete(ctx context.Context, req *kfv1.DeleteTopicRequest) (*kfv1.DeleteTopicResponse, error) {
	op, err := ts.Kafka.DeleteTopic(ctx, req.GetClusterId(), req.GetTopicName())
	if err != nil {
		return nil, err
	}

	return &kfv1.DeleteTopicResponse{
		OperationId: op.OperationID,
	}, nil
}
