package provider

import (
	"context"
	"encoding/json"
	"testing"

	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/secret"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/crypto"
	clusterslogic "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/kafka/kfmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/kafka/provider/internal/kfpillars"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/pillars"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/tasks"
)

func TestKafka_ResetCredentials_whenNoOwnerInPillarShouldReturnError(t *testing.T) {
	const (
		pillarRaw       = "{}"
		clusterID       = "cid"
		clusterRevision = int64(12345)
	)
	kafkaFixture := newKafkaFixture(t)
	mockOperatorModifyOnNotStoppedCluster(kafkaFixture, clusterID, clusterRevision, pillarRaw)

	_, err := kafkaFixture.Kafka.ResetCredentials(kafkaFixture.Context, clusterID)

	require.Error(t, err, "can't find user: %s", dataCloudOwnerUserName)

}

func TestKafka_ResetCredentials(t *testing.T) {
	const (
		pillarRaw = `
{
	"data": {
		"kafka": {
			"users": {
				"owner": {
					"password": {
						"data": "somePassword",
						"encryption_version": 0
					}
				}
			}
		}
	}
}`
		encryptedPassword = "encryptedPassword"
		encryptedVersion  = int64(333)
		clusterID         = "cid"
		clusterRevision   = int64(12345)
	)
	pillar := mockPillar(encryptedPassword, encryptedVersion)
	kafkaFixture := newKafkaFixture(t)
	mockOperatorModifyOnNotStoppedCluster(kafkaFixture, clusterID, clusterRevision, pillarRaw)
	mockCryptoGenerateEncryptedPassword(kafkaFixture, encryptedPassword, encryptedVersion)
	kafkaFixture.Modifier.
		EXPECT().UpdatePillar(kafkaFixture.Context, clusterID, clusterRevision, pillar).
		Return(nil)

	expectedOperation := operations.Operation{}
	kafkaFixture.Tasks.EXPECT().CreateTask(kafkaFixture.Context, kafkaFixture.Session, tasks.CreateTaskArgs{
		ClusterID:     clusterID,
		FolderID:      kafkaFixture.Session.FolderCoords.FolderID,
		Auth:          kafkaFixture.Session.Subject,
		OperationType: kfmodels.OperationTypeUserModify,
		TaskType:      kfmodels.TaskTypeUserModify,
		Metadata:      kfmodels.MetadataModifyUser{UserName: pillar.GetAdminUserName()},
		TaskArgs:      map[string]interface{}{"target-user": pillar.GetAdminUserName()},
		Revision:      clusterRevision,
	}).Return(expectedOperation, nil)

	actualOperation, err := kafkaFixture.Kafka.ResetCredentials(kafkaFixture.Context, clusterID)

	require.NoError(t, err)
	require.Equal(t, expectedOperation, actualOperation)
}

func mockOperatorModifyOnNotStoppedCluster(kafkaFixture kafkaFixture, cid string, revision int64, pillarRaw string) {
	kafkaFixture.Operator.
		EXPECT().ModifyOnNotStoppedCluster(kafkaFixture.Context, cid, clusters.TypeKafka, gomock.Any(), gomock.Any()).
		DoAndReturn(

			func(ctx context.Context, cid string, typ clusters.Type, do clusterslogic.ModifyOnNotStoppedClusterFunc,
				opts ...clusterslogic.OperatorOption) (operations.Operation, error) {

				return do(ctx, kafkaFixture.Session, kafkaFixture.Reader, kafkaFixture.Modifier,
					clusterslogic.NewClusterModel(clusters.Cluster{Revision: revision}, json.RawMessage(pillarRaw)))

			},
		)
}

func mockPillar(encryptedPassword string, encryptedVersion int64) *kfpillars.Cluster {
	pillar := kfpillars.NewCluster()
	pillar.Data.Kafka.Users = make(map[string]kfpillars.UserData)
	pillar.Data.Kafka.Users[pillar.GetAdminUserName()] = kfpillars.UserData{
		Password: pillars.CryptoKey{
			Data:              encryptedPassword,
			EncryptionVersion: encryptedVersion,
		},
	}
	pillar.Data.ZooKeeper.Config.DataDir = "/data/zookeper"

	return pillar
}

func mockCryptoGenerateEncryptedPassword(kafkaFixture kafkaFixture, encryptedPassword string, encryptedVersion int64) {
	generatedPassword := secret.NewString("generatedPassword")
	kafkaFixture.CryptoProvider.
		EXPECT().GenerateRandomString(passwordGenLen, []rune(crypto.PasswordValidRunes)).
		Return(generatedPassword, nil)

	kafkaFixture.CryptoProvider.EXPECT().Encrypt([]byte(generatedPassword.Unmask())).Return(pillars.CryptoKey{
		Data:              encryptedPassword,
		EncryptionVersion: encryptedVersion,
	}, nil)
}
