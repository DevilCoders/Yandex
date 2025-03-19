package datacloudgrpc

import (
	"context"
	"testing"
	"time"

	"github.com/golang/protobuf/proto"
	"github.com/stretchr/testify/require"

	kfv1 "a.yandex-team.ru/cloud/mdb/datacloud/private_api/datacloud/kafka/v1"
	apiv1 "a.yandex-team.ru/cloud/mdb/datacloud/private_api/datacloud/v1"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
	"a.yandex-team.ru/library/go/core/log/nop"
)

const (
	OperationTypeClusterCreate operations.Type = "kafka_cluster_create"
)

func init() {
	operations.Register(
		OperationTypeClusterCreate,
		"Create Kafka cluster",
		MetadataCreateCluster{},
	)
}

type MetadataCreateCluster struct{}

func (md MetadataCreateCluster) Build(op operations.Operation) proto.Message {
	return &kfv1.CreateClusterRequest{
		Name: op.TargetID,
	}
}

func Test_createClusterArgsFromGRPC(t *testing.T) {
	t.Run("When operations are empty should return emtpy list", func(t *testing.T) {
		result, err := OperationsToGRPC(context.Background(), []operations.Operation{}, "projectID", &nop.Logger{})
		require.NoError(t, err)
		require.Equal(t, []*apiv1.Operation(nil), result)
	})

	t.Run("When list is not empty should convert to api entities", func(t *testing.T) {
		createdAt := time.Date(2021, 10, 21, 12, 00, 00, 00, time.Local)
		modifiedAt := time.Date(2022, 11, 22, 12, 00, 00, 00, time.Local)
		startedAt := time.Date(2021, 12, 21, 12, 00, 00, 00, time.Local)
		result, err := OperationsToGRPC(context.Background(), []operations.Operation{{
			Type:        OperationTypeClusterCreate,
			MetaData:    MetadataCreateCluster{},
			OperationID: "operationID",
			CreatedAt:   createdAt,
			ModifiedAt:  modifiedAt,
			StartedAt:   startedAt,
			Status:      operations.StatusDone,
		}}, "projectID", &nop.Logger{})

		require.NoError(t, err)
		require.Equal(t, []*apiv1.Operation{{
			Id:          "operationID",
			ProjectId:   "projectID",
			Description: "Create Kafka cluster",
			Metadata: map[string]string{
				"access":      "",
				"cloud_type":  "",
				"description": "",
				"encryption":  "",
				"name":        "",
				"network_id":  "",
				"project_id":  "",
				"region_id":   "",
				"resources":   "",
				"version":     "",
			},
			CreateTime: grpc.TimeToGRPC(createdAt),
			StartTime:  grpc.TimeToGRPC(startedAt),
			FinishTime: grpc.TimeToGRPC(modifiedAt),
			Status:     apiv1.Operation_STATUS_DONE,
		}}, result)
	})
}
