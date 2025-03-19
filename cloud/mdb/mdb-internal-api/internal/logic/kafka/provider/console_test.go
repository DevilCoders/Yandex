package provider

import (
	"encoding/json"
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func TestKafka_Version(t *testing.T) {
	kafkaFixture := newKafkaFixture(t)
	CID := "cid"

	t.Run("When metadb return error should return error", func(t *testing.T) {
		err := xerrors.Errorf("metadb error")
		kafkaFixture.MetaDB.EXPECT().ClusterByClusterID(kafkaFixture.Context, CID, models.VisibilityVisible).Return(metadb.Cluster{}, err)

		_, actualErr := kafkaFixture.Kafka.Version(kafkaFixture.Context, kafkaFixture.MetaDB, CID)

		require.Error(t, actualErr)
	})

	t.Run("When metadb returns kafka cluster should return kafka version from pillar", func(t *testing.T) {
		kafkaFixture.MetaDB.EXPECT().ClusterByClusterID(kafkaFixture.Context, CID, models.VisibilityVisible).
			Return(metaDBCluster(`{"data": {"kafka": {"version": "2.8.1"}}}`), nil)

		version, err := kafkaFixture.Kafka.Version(kafkaFixture.Context, kafkaFixture.MetaDB, CID)

		require.NoError(t, err)
		require.Equal(t, "2.8.1", version)
	})
}

func metaDBCluster(rawPillar string) metadb.Cluster {
	return metadb.Cluster{
		Pillar: json.RawMessage(rawPillar),
	}
}
