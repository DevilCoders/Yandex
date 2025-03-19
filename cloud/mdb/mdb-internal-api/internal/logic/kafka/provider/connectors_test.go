package provider

import (
	"testing"

	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/secret"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/crypto/mocks"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/kafka/kfmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/kafka/provider/internal/kfpillars"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/pillars"
)

func TestKafkaConnect_UpdateClusterConnection(t *testing.T) {
	cryptoProvider := mocks.MockCrypto{}
	t.Run("SuccessAliasUpdate", func(t *testing.T) {
		clusterConnectionCurrent := kfpillars.ClusterConnection{
			Alias: "target",
			Type:  kfmodels.ClusterConnectionTypeThisCluster,
		}
		updateClusterConnectionSpec := kfmodels.UpdateClusterConnectionSpec{}
		updateClusterConnectionSpec.Alias.Set("test_alias")
		clusterConnectionUpdated, hasChanges, err := UpdateClusterConnection(
			clusterConnectionCurrent, updateClusterConnectionSpec, &cryptoProvider,
		)
		clusterConnectionExpected := kfpillars.ClusterConnection{
			Alias: "test_alias",
			Type:  kfmodels.ClusterConnectionTypeThisCluster,
		}
		require.NoError(t, err)
		require.True(t, hasChanges)
		require.Equal(t, clusterConnectionUpdated, clusterConnectionExpected)
	})

	t.Run("SuccessTypeUpdate", func(t *testing.T) {
		clusterConnectionCurrent := kfpillars.ClusterConnection{
			Alias: "target",
			Type:  kfmodels.ClusterConnectionTypeExternal,
		}
		updateClusterConnectionSpec := kfmodels.UpdateClusterConnectionSpec{}
		updateClusterConnectionSpec.Type.Set(kfmodels.ClusterConnectionTypeThisCluster)
		clusterConnectionUpdated, hasChanges, err := UpdateClusterConnection(
			clusterConnectionCurrent, updateClusterConnectionSpec, &cryptoProvider,
		)
		clusterConnectionExpected := kfpillars.ClusterConnection{
			Alias: "target",
			Type:  kfmodels.ClusterConnectionTypeThisCluster,
		}
		require.NoError(t, err)
		require.True(t, hasChanges)
		require.Equal(t, clusterConnectionUpdated, clusterConnectionExpected)
	})

	t.Run("SuccessFullClusterConnectionUpdate", func(t *testing.T) {
		clusterConnectionCurrent := kfpillars.ClusterConnection{
			Alias:            "target",
			Type:             kfmodels.ClusterConnectionTypeExternal,
			BootstrapServers: "kafka-yandex-999.db.yandex.net:9091",
			SaslUsername:     "egor",
			SecurityProtocol: "sasl_ssl",
			SaslMechanism:    "scram-sha-512",
		}
		updateClusterConnectionSpec := kfmodels.UpdateClusterConnectionSpec{}
		updateClusterConnectionSpec.Type.Set(kfmodels.ClusterConnectionTypeExternal)
		updateClusterConnectionSpec.Alias.Set("test_alias")
		updateClusterConnectionSpec.BootstrapServers.Set("test_bootstrap_servers")
		updateClusterConnectionSpec.SaslUsername.Set("test_sasl_username")
		clusterConnectionUpdated, hasChanges, err := UpdateClusterConnection(
			clusterConnectionCurrent, updateClusterConnectionSpec, &cryptoProvider,
		)
		clusterConnectionExpected := kfpillars.ClusterConnection{
			Alias:            "test_alias",
			Type:             kfmodels.ClusterConnectionTypeExternal,
			BootstrapServers: "test_bootstrap_servers",
			SaslUsername:     "test_sasl_username",
			SecurityProtocol: "sasl_ssl",
			SaslMechanism:    "scram-sha-512",
		}
		require.NoError(t, err)
		require.True(t, hasChanges)
		require.Equal(t, clusterConnectionUpdated, clusterConnectionExpected)
	})
}

func TestKafkaConnect_UpdateS3Connection(t *testing.T) {
	cryptoProvider := mocks.MockCrypto{}
	t.Run("SuccessS3ConnectionUpdate", func(t *testing.T) {
		s3ConnectionCurrent := kfpillars.S3Connection{
			Type:        kfmodels.S3ConnectionTypeExternal,
			AccessKeyID: "test",
			BucketName:  "test",
			Endpoint:    "test",
			Region:      "test",
		}
		updateS3ConnectionSpec := kfmodels.UpdateS3ConnectionSpec{}
		updateS3ConnectionSpec.AccessKeyID.Set("updated")
		updateS3ConnectionSpec.BucketName.Set("updated")
		updateS3ConnectionSpec.Endpoint.Set("updated")
		updateS3ConnectionSpec.Region.Set("updated")
		s3ConnectionUpdated, hasChanges, err := UpdateS3Connection(
			s3ConnectionCurrent, updateS3ConnectionSpec, &cryptoProvider,
		)
		s3ConnectionExpected := kfpillars.S3Connection{
			Type:        kfmodels.S3ConnectionTypeExternal,
			AccessKeyID: "updated",
			BucketName:  "updated",
			Endpoint:    "updated",
			Region:      "updated",
		}
		require.NoError(t, err)
		require.True(t, hasChanges)
		require.Equal(t, s3ConnectionExpected, s3ConnectionUpdated)
	})
}

func TestKafkaConnect_UpdateMirrormakerConfig(t *testing.T) {
	cryptoProvider := mocks.MockCrypto{}
	t.Run("SuccessFullMirrormakerConfigUpdate", func(t *testing.T) {
		mirrorMakerConfigCurrent := kfpillars.MirrorMakerConfig{
			Topics:            "topic*",
			ReplicationFactor: 2,
			SourceCluster: kfpillars.ClusterConnection{
				Alias:            "source_alias",
				Type:             kfmodels.ClusterConnectionTypeExternal,
				BootstrapServers: "bootstrap_servers",
				SaslUsername:     "sasl_username",
				SecurityProtocol: "sasl_ssl",
				SaslMechanism:    "scram-sha-512",
			},
			TargetCluster: kfpillars.ClusterConnection{
				Alias: "target",
				Type:  kfmodels.ClusterConnectionTypeThisCluster,
			},
		}
		updateMirrormakerConfigSpec := kfmodels.UpdateMirrormakerConfigSpec{
			SourceCluster: kfmodels.UpdateClusterConnectionSpec{},
			TargetCluster: kfmodels.UpdateClusterConnectionSpec{},
		}
		updateMirrormakerConfigSpec.Topics.Set("test_topic")
		updateMirrormakerConfigSpec.ReplicationFactor.Set(5)
		updateMirrormakerConfigSpec.SourceCluster.Alias.Set("source_alias_updated")
		updateMirrormakerConfigSpec.SourceCluster.Type.Set(kfmodels.ClusterConnectionTypeExternal)
		updateMirrormakerConfigSpec.SourceCluster.BootstrapServers.Set("bootstrap_servers_updated")
		updateMirrormakerConfigSpec.SourceCluster.SaslUsername.Set("sasl_username_updated")
		updateMirrormakerConfigSpec.TargetCluster.Alias.Set("target_alias")
		updateMirrormakerConfigSpec.TargetCluster.Type.Set(kfmodels.ClusterConnectionTypeExternal)
		updateMirrormakerConfigSpec.TargetCluster.BootstrapServers.Set("bootstrap_servers_target")
		updateMirrormakerConfigSpec.TargetCluster.SaslUsername.Set("sasl_username_target")
		updateMirrormakerConfigSpec.TargetCluster.SecurityProtocol.Set("sasl_ssl")
		updateMirrormakerConfigSpec.TargetCluster.SaslMechanism.Set("scram-sha-512")
		mirrorMakerConfigUpdated, hasChanges, err := UpdateMirrormakerConfig(
			mirrorMakerConfigCurrent, updateMirrormakerConfigSpec, &cryptoProvider,
		)
		mirrorMakerConfigExpected := kfpillars.MirrorMakerConfig{
			Topics:            "test_topic",
			ReplicationFactor: 5,
			SourceCluster: kfpillars.ClusterConnection{
				Alias:            "source_alias_updated",
				Type:             kfmodels.ClusterConnectionTypeExternal,
				BootstrapServers: "bootstrap_servers_updated",
				SaslUsername:     "sasl_username_updated",
				SecurityProtocol: "sasl_ssl",
				SaslMechanism:    "scram-sha-512",
			},
			TargetCluster: kfpillars.ClusterConnection{
				Alias:            "target_alias",
				Type:             kfmodels.ClusterConnectionTypeExternal,
				BootstrapServers: "bootstrap_servers_target",
				SaslUsername:     "sasl_username_target",
				SecurityProtocol: "sasl_ssl",
				SaslMechanism:    "scram-sha-512",
			},
		}
		require.NoError(t, err)
		require.True(t, hasChanges)
		require.Equal(t, mirrorMakerConfigExpected, mirrorMakerConfigUpdated)
	})
}

func TestKafkaConnect_UpdateS3SinkConnectorConfig(t *testing.T) {
	cryptoProvider := mocks.MockCrypto{}
	t.Run("SuccessS3SinkConnectorConfigUpdate", func(t *testing.T) {
		s3SinkConnectorConfigCurrent := kfpillars.S3SinkConnectorConfig{
			S3Connection: kfpillars.S3Connection{
				Type:        kfmodels.S3ConnectionTypeExternal,
				AccessKeyID: "test",
				BucketName:  "test",
				Endpoint:    "test",
				Region:      "test",
			},
			Topics:              "test",
			FileCompressionType: "test",
			FileMaxRecords:      int64(100),
		}
		updateS3SinkConnectorConfigSpec := kfmodels.UpdateS3SinkConnectorConfigSpec{
			S3Connection: kfmodels.UpdateS3ConnectionSpec{},
		}
		updateS3SinkConnectorConfigSpec.Topics.Set("updated")
		updateS3SinkConnectorConfigSpec.FileMaxRecords.Set(int64(200))
		updateS3SinkConnectorConfigSpec.S3Connection.AccessKeyID.Set("updated")
		updateS3SinkConnectorConfigSpec.S3Connection.BucketName.Set("updated")
		updateS3SinkConnectorConfigSpec.S3Connection.Endpoint.Set("updated")
		updateS3SinkConnectorConfigSpec.S3Connection.Region.Set("updated")
		updateS3SinkConnectorConfigSpec.S3Connection.SecretAccessKeyUpdated = false
		s3SinkConnectorConfigUpdated, hasChanges, err := UpdateS3SinkConnectorConfig(
			s3SinkConnectorConfigCurrent, updateS3SinkConnectorConfigSpec, &cryptoProvider,
		)
		s3SinkConnectorConfigExpected := kfpillars.S3SinkConnectorConfig{
			S3Connection: kfpillars.S3Connection{
				Type:        kfmodels.S3ConnectionTypeExternal,
				AccessKeyID: "updated",
				BucketName:  "updated",
				Endpoint:    "updated",
				Region:      "updated",
			},
			Topics:              "updated",
			FileCompressionType: "test",
			FileMaxRecords:      int64(200),
		}
		require.NoError(t, err)
		require.True(t, hasChanges)
		require.Equal(t, s3SinkConnectorConfigExpected, s3SinkConnectorConfigUpdated)
	})
}

func TestKafkaConnect_UpdateConnectorData(t *testing.T) {
	cryptoProvider := mocks.MockCrypto{}

	currentConnectorGen := func() kfpillars.ConnectorData {
		return kfpillars.ConnectorData{
			Name:     "test_connector",
			Type:     kfmodels.ConnectorTypeMirrormaker,
			TasksMax: 1,
			Properties: map[string]string{
				"key_to_stay":   "value",
				"key_to_delete": "value",
				"key_to_update": "value",
			},
			MirrorMakerConfig: kfpillars.MirrorMakerConfig{
				Topics:            "topic*",
				ReplicationFactor: 2,
				SourceCluster: kfpillars.ClusterConnection{
					Alias:            "source_alias",
					Type:             kfmodels.ClusterConnectionTypeExternal,
					BootstrapServers: "bootstrap_servers",
					SaslUsername:     "sasl_username",
					SecurityProtocol: "sasl_ssl",
					SaslMechanism:    "scram-sha-512",
				},
				TargetCluster: kfpillars.ClusterConnection{
					Alias: "target",
					Type:  kfmodels.ClusterConnectionTypeThisCluster,
				},
			},
		}
	}

	t.Run("SuccessFullConnectorDataUpdate", func(t *testing.T) {
		connectorDataCurrent := currentConnectorGen()
		updateConnectorSpec := kfmodels.UpdateConnectorSpec{
			Properties: map[string]string{
				"key_to_update": "updated_value",
			},
			PropertiesContains: map[string]bool{
				"key_to_update": true,
				"key_to_delete": false,
			},
			MirrormakerConfig: kfmodels.UpdateMirrormakerConfigSpec{
				SourceCluster: kfmodels.UpdateClusterConnectionSpec{},
				TargetCluster: kfmodels.UpdateClusterConnectionSpec{},
			},
		}
		updateConnectorSpec.TasksMax.Set(2)
		updateConnectorSpec.Type.Set(kfmodels.ConnectorTypeMirrormaker)
		updateConnectorSpec.MirrormakerConfig.Topics.Set("test_topic")
		updateConnectorSpec.MirrormakerConfig.ReplicationFactor.Set(5)
		updateConnectorSpec.MirrormakerConfig.SourceCluster.Alias.Set("source_alias_updated")
		updateConnectorSpec.MirrormakerConfig.SourceCluster.Type.Set(kfmodels.ClusterConnectionTypeExternal)
		updateConnectorSpec.MirrormakerConfig.SourceCluster.BootstrapServers.Set("bootstrap_servers_updated")
		updateConnectorSpec.MirrormakerConfig.SourceCluster.SaslUsername.Set("sasl_username_updated")
		updateConnectorSpec.MirrormakerConfig.TargetCluster.Alias.Set("target_alias")
		updateConnectorSpec.MirrormakerConfig.TargetCluster.Type.Set(kfmodels.ClusterConnectionTypeExternal)
		updateConnectorSpec.MirrormakerConfig.TargetCluster.BootstrapServers.Set("bootstrap_servers_target")
		updateConnectorSpec.MirrormakerConfig.TargetCluster.SaslUsername.Set("sasl_username_target")
		updateConnectorSpec.MirrormakerConfig.TargetCluster.SecurityProtocol.Set("sasl_ssl")
		updateConnectorSpec.MirrormakerConfig.TargetCluster.SaslMechanism.Set("scram-sha-512")
		connectorDataUpdated, hasChanges, err := UpdateConnectorData(
			connectorDataCurrent, updateConnectorSpec, &cryptoProvider,
		)
		connectorDataExpected := kfpillars.ConnectorData{
			Name:     "test_connector",
			Type:     "mirrormaker",
			TasksMax: 2,
			Properties: map[string]string{
				"key_to_stay":   "value",
				"key_to_update": "updated_value",
			},
			MirrorMakerConfig: kfpillars.MirrorMakerConfig{
				Topics:            "test_topic",
				ReplicationFactor: 5,
				SourceCluster: kfpillars.ClusterConnection{
					Alias:            "source_alias_updated",
					Type:             kfmodels.ClusterConnectionTypeExternal,
					BootstrapServers: "bootstrap_servers_updated",
					SaslUsername:     "sasl_username_updated",
					SecurityProtocol: "sasl_ssl",
					SaslMechanism:    "scram-sha-512",
				},
				TargetCluster: kfpillars.ClusterConnection{
					Alias:            "target_alias",
					Type:             kfmodels.ClusterConnectionTypeExternal,
					BootstrapServers: "bootstrap_servers_target",
					SaslUsername:     "sasl_username_target",
					SecurityProtocol: "sasl_ssl",
					SaslMechanism:    "scram-sha-512",
				},
			},
		}
		require.NoError(t, err)
		require.True(t, hasChanges)
		require.Equal(t, connectorDataExpected, connectorDataUpdated)
	})

	t.Run("ErrorWhenChangeConnectorType", func(t *testing.T) {
		connectorDataCurrent := currentConnectorGen()
		updateConnectorSpec := kfmodels.UpdateConnectorSpec{
			Properties: map[string]string{
				"key_to_update": "updated_value",
			},
			PropertiesContains: map[string]bool{
				"key_to_update": true,
				"key_to_delete": false,
			},
			MirrormakerConfig: kfmodels.UpdateMirrormakerConfigSpec{
				SourceCluster: kfmodels.UpdateClusterConnectionSpec{},
				TargetCluster: kfmodels.UpdateClusterConnectionSpec{},
			},
		}
		updateConnectorSpec.Type.Set("s3-source")
		_, _, err := UpdateConnectorData(
			connectorDataCurrent, updateConnectorSpec, &cryptoProvider,
		)
		require.Error(t, err)
		require.True(t, semerr.IsInvalidInput(err))
		require.Equal(t, "connector type cannot be changed", err.Error())
	})

	t.Run("WhenNoFieldsToUpdate", func(t *testing.T) {
		connectorDataCurrent := currentConnectorGen()
		updateConnectorSpec := kfmodels.UpdateConnectorSpec{
			Properties:         map[string]string{},
			PropertiesContains: map[string]bool{},
			MirrormakerConfig: kfmodels.UpdateMirrormakerConfigSpec{
				SourceCluster: kfmodels.UpdateClusterConnectionSpec{},
				TargetCluster: kfmodels.UpdateClusterConnectionSpec{},
			},
		}
		updateConnectorSpec.Type.Set(kfmodels.ConnectorTypeMirrormaker)
		_, hasChanges, err := UpdateConnectorData(
			connectorDataCurrent, updateConnectorSpec, &cryptoProvider,
		)
		require.NoError(t, err)
		require.False(t, hasChanges)
	})

	t.Run("ProperttiesOverWrite", func(t *testing.T) {
		connectorDataCurrent := currentConnectorGen()
		updateConnectorSpec := kfmodels.UpdateConnectorSpec{
			Properties: map[string]string{
				"new_key_1": "new_key_1_value",
				"new_key_2": "new_key_2_value",
			},
			PropertiesOverwrite: true,
		}
		connectorDataExpected := kfpillars.ConnectorData{
			Name:     "test_connector",
			Type:     "mirrormaker",
			TasksMax: 1,
			Properties: map[string]string{
				"new_key_1": "new_key_1_value",
				"new_key_2": "new_key_2_value",
			},
			MirrorMakerConfig: kfpillars.MirrorMakerConfig{
				Topics:            "topic*",
				ReplicationFactor: 2,
				SourceCluster: kfpillars.ClusterConnection{
					Alias:            "source_alias",
					Type:             kfmodels.ClusterConnectionTypeExternal,
					BootstrapServers: "bootstrap_servers",
					SaslUsername:     "sasl_username",
					SecurityProtocol: "sasl_ssl",
					SaslMechanism:    "scram-sha-512",
				},
				TargetCluster: kfpillars.ClusterConnection{
					Alias: "target",
					Type:  kfmodels.ClusterConnectionTypeThisCluster,
				},
			},
		}
		updateConnectorSpec.Type.Set(kfmodels.ConnectorTypeMirrormaker)
		connectorDataUpdated, hasChanges, err := UpdateConnectorData(
			connectorDataCurrent, updateConnectorSpec, &cryptoProvider,
		)
		require.NoError(t, err)
		require.True(t, hasChanges)
		require.Equal(t, connectorDataExpected, connectorDataUpdated)
	})
}

func TestKafkaConnect_S3Connection(t *testing.T) {
	t.Run("S3ConnectionToPillarCorrect", func(t *testing.T) {
		ctrl := gomock.NewController(t)
		cryptoProvider := mocks.NewMockCrypto(ctrl)
		cryptoProvider.EXPECT().Encrypt(gomock.Any()).Return(pillars.CryptoKey{
			Data:              "test",
			EncryptionVersion: 1,
		}, nil)
		modelSpec := kfmodels.S3ConnectionSpec{
			Type:            kfmodels.S3ConnectionTypeExternal,
			AccessKeyID:     "test",
			SecretAccessKey: secret.NewString("test"),
			BucketName:      "test",
			Endpoint:        "test",
			Region:          "test",
		}
		pillarSpecActual, err := S3ConnectionToPillar(modelSpec, cryptoProvider)
		pillarSpecExpected := kfpillars.S3Connection{
			Type:            kfmodels.S3ConnectionTypeExternal,
			AccessKeyID:     "test",
			SecretAccessKey: pillarSpecActual.SecretAccessKey,
			BucketName:      "test",
			Endpoint:        "test",
			Region:          "test",
		}
		require.NoError(t, err)
		require.Equal(t, pillarSpecExpected, pillarSpecActual)
	})

	t.Run("S3ConnectionFromPillarCorrect", func(t *testing.T) {
		pillarSpec := kfpillars.S3Connection{
			Type:        kfmodels.S3ConnectionTypeExternal,
			AccessKeyID: "test",
			SecretAccessKey: &pillars.CryptoKey{
				Data:              "test",
				EncryptionVersion: 1,
			},
			BucketName: "test",
			Endpoint:   "test",
			Region:     "test",
		}
		modelSpecActual := s3ConnectionFromPillar(pillarSpec)
		modelSpecExpected := kfmodels.S3Connection{
			Type:        kfmodels.S3ConnectionTypeExternal,
			AccessKeyID: "test",
			BucketName:  "test",
			Endpoint:    "test",
			Region:      "test",
		}
		require.Equal(t, modelSpecActual, modelSpecExpected)
	})
}

func TestKafkaConnect_S3SinkConnector(t *testing.T) {
	t.Run("S3SinkConnectorConfigSpecToPillarCorrect", func(t *testing.T) {
		ctrl := gomock.NewController(t)
		cryptoProvider := mocks.NewMockCrypto(ctrl)
		cryptoProvider.EXPECT().Encrypt(gomock.Any()).Return(pillars.CryptoKey{
			Data:              "test",
			EncryptionVersion: 1,
		}, nil)
		modelSpec := kfmodels.S3SinkConnectorConfigSpec{
			S3Connection: kfmodels.S3ConnectionSpec{
				Type:            kfmodels.S3ConnectionTypeExternal,
				AccessKeyID:     "test",
				SecretAccessKey: secret.NewString("test"),
				BucketName:      "test",
				Endpoint:        "test",
				Region:          "test",
			},
			Topics:              "test",
			FileCompressionType: "test",
			FileMaxRecords:      int64(100),
		}
		pillarSpecActual, err := S3SinkConnectorConfigSpecToPillar(modelSpec, cryptoProvider)
		pillarSpecExpected := kfpillars.S3SinkConnectorConfig{
			S3Connection: kfpillars.S3Connection{
				Type:            kfmodels.S3ConnectionTypeExternal,
				AccessKeyID:     "test",
				SecretAccessKey: pillarSpecActual.S3Connection.SecretAccessKey,
				BucketName:      "test",
				Endpoint:        "test",
				Region:          "test",
			},
			Topics:              "test",
			FileCompressionType: "test",
			FileMaxRecords:      int64(100),
		}
		require.NoError(t, err)
		require.Equal(t, pillarSpecExpected, pillarSpecActual)
	})

	t.Run("S3SinkConnectorConfigFromPillarCorrect", func(t *testing.T) {
		pillarSpec := kfpillars.S3SinkConnectorConfig{
			S3Connection: kfpillars.S3Connection{
				Type:        kfmodels.S3ConnectionTypeExternal,
				AccessKeyID: "test",
				SecretAccessKey: &pillars.CryptoKey{
					Data:              "test",
					EncryptionVersion: 1,
				},
				BucketName: "test",
				Endpoint:   "test",
				Region:     "test",
			},
			Topics:              "test",
			FileCompressionType: "test",
			FileMaxRecords:      int64(100),
		}
		modelSpecActual := s3SinkConnectorConfigFromPillar(pillarSpec)
		modelSpecExpected := kfmodels.S3SinkConnectorConfig{
			S3Connection: kfmodels.S3Connection{
				Type:        kfmodels.S3ConnectionTypeExternal,
				AccessKeyID: "test",
				BucketName:  "test",
				Endpoint:    "test",
				Region:      "test",
			},
			Topics:              "test",
			FileCompressionType: "test",
			FileMaxRecords:      int64(100),
		}
		require.Equal(t, modelSpecActual, modelSpecExpected)
	})
}
