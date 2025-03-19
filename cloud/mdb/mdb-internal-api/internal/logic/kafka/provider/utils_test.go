package provider

import (
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/environment"
)

func Test_selectDataCloudKafkaAndZookeeperHostZones(t *testing.T) {
	kafkaFixture := newKafkaFixture(t)

	t.Run("When modifier return error should return error", func(t *testing.T) {
		kafkaFixture.Modifier.EXPECT().SelectZonesForCloudAndRegion(kafkaFixture.Context, kafkaFixture.Session, environment.CloudTypeAWS, "us-east-1", true, 1).
			Return(nil, semerr.NotFoundf("not found available zones"))

		_, _, err := selectDataCloudKafkaAndZookeeperHostZones(kafkaFixture.Context, kafkaFixture.Session, kafkaFixture.Modifier, environment.CloudTypeAWS,
			"us-east-1", 1, true)

		require.EqualError(t, err, "not found available zones")
	})

	t.Run("When modifier return enough available zones and one host mode should return only one kafka zone", func(t *testing.T) {
		kafkaFixture.Modifier.EXPECT().SelectZonesForCloudAndRegion(kafkaFixture.Context, kafkaFixture.Session, environment.CloudTypeAWS, "us-east-1", true, 1).
			Return([]string{"us-east-1a"}, nil)

		kafkaZones, zkZones, err := selectDataCloudKafkaAndZookeeperHostZones(kafkaFixture.Context, kafkaFixture.Session, kafkaFixture.Modifier, environment.CloudTypeAWS,
			"us-east-1", 1, true)

		require.NoError(t, err)
		require.Equal(t, []string{"us-east-1a"}, kafkaZones)
		require.Equal(t, []string{}, zkZones)
	})

	t.Run("When modifier return enough available zones and one host mode should return all kafka zones and three zk zones", func(t *testing.T) {
		kafkaFixture.Modifier.EXPECT().SelectZonesForCloudAndRegion(kafkaFixture.Context, kafkaFixture.Session, environment.CloudTypeAWS, "us-east-1", true, 5).
			Return([]string{"us-east-1a", "us-east-1b", "us-east-1c", "us-east-1d", "us-east-1e"}, nil)

		kafkaZones, zkZones, err := selectDataCloudKafkaAndZookeeperHostZones(kafkaFixture.Context, kafkaFixture.Session, kafkaFixture.Modifier, environment.CloudTypeAWS,
			"us-east-1", 5, false)

		require.NoError(t, err)
		require.Equal(t, []string{"us-east-1a", "us-east-1b", "us-east-1c", "us-east-1d", "us-east-1e"}, kafkaZones)
		require.Equal(t, []string{"us-east-1a", "us-east-1b", "us-east-1c"}, zkZones)
	})
}
