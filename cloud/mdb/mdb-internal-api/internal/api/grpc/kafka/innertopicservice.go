package kafka

import (
	"context"

	kfv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/kafka/v1/inner"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/kafka"
)

type InnerTopicService struct {
	kfv1.UnimplementedTopicServiceServer

	Kafka kafka.Kafka
}

func (ts *InnerTopicService) List(ctx context.Context, req *kfv1.ListTopicsRequest) (*kfv1.ListTopicsResponse, error) {
	topicsToSync, err := ts.Kafka.TopicsToSync(ctx, req.GetClusterId())
	if err != nil {
		return nil, err
	}

	return &kfv1.ListTopicsResponse{
		UpdateAllowed:              topicsToSync.UpdateAllowed,
		Revision:                   topicsToSync.Revision,
		Topics:                     topicsToSync.Topics,
		KnownTopicConfigProperties: topicsToSync.KnownTopicConfigProperties,
	}, nil
}

func (ts *InnerTopicService) Update(ctx context.Context, req *kfv1.UpdateTopicsRequest) (*kfv1.UpdateTopicsResponse, error) {
	updateAccepted, err := ts.Kafka.SyncTopics(ctx, req.GetClusterId(), req.GetRevision(), req.GetChangedTopics(), req.GetDeletedTopics())
	if err != nil {
		return nil, err
	}

	return &kfv1.UpdateTopicsResponse{UpdateAccepted: updateAccepted}, nil
}
