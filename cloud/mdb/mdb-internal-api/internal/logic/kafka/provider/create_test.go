package provider

import (
	"context"
	"sort"
	"testing"

	"github.com/gofrs/uuid"
	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/compute"
	"a.yandex-team.ru/cloud/mdb/internal/featureflags"
	networkProvider "a.yandex-team.ru/cloud/mdb/internal/network"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/secret"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/crypto"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic"
	clusterslogic "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/sessions"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/kafka"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/kafka/kfmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/kafka/provider/internal/kfpillars"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/environment"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/pillars"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/resources"
	"a.yandex-team.ru/library/go/ptr"
)

const (
	regionID                    = "eu-central-1"
	projectID                   = "projectID"
	clusterID                   = "clusterID"
	clusterName                 = "clusterName"
	clusterDescription          = "clusterDescription"
	kafkaSubClusterID           = "kafkaSubClusterID"
	kafkaDiskSize               = int64(555111)
	kafkaDiskTypeExtID          = "DiskTypeExtIDKafka"
	kafkaResourcePresetExtID    = "ResourcePresetExtIDKafka"
	zkSubClusterID              = "zkSubClusterID"
	zkResourcePresetExtID       = "ResourcePresetExtIDZK"
	zkDiskTypeExtID             = "DiskTypeExtIDZK"
	zkDiskSize                  = int64(333111)
	sessionFolderCoordsFolderID = int64(5000)
	folderExtID                 = "folderExtID"
	cloudExtID                  = "cloudExtID"
	privateKey                  = "somePrivateKey"
	revisionID                  = int64(12345)
	networkID                   = "defaultNetworkID"

	zoneA             = "us-east-1a"
	zoneB             = "us-east-1b"
	zoneC             = "us-east-1c"
	zoneD             = "us-east-1d"
	subnetIDA         = "subnetA"
	subnetIDB         = "subnetB"
	subnetIDC         = "subnetC"
	kafkaFQDN1        = "a-euc1-az1-kafka1-1.yadc.io"
	kafkaFQDN2        = "a-euc1-az2-kafka1-1.yadc.io"
	kafkaFQDN3        = "a-euc1-az3-kafka1-1.yadc.io"
	privateKafkaFQDN1 = "a-euc1-az1-kafka1-1.private.yadc.io"
	privateKafkaFQDN2 = "a-euc1-az2-kafka1-1.private.yadc.io"
	privateKafkaFQDN3 = "a-euc1-az3-kafka1-1.private.yadc.io"
	zkFQDN1           = "a-euc1-az1-zk1-1.yadc.io"
	zkFQDN2           = "a-euc1-az2-zk2-1.yadc.io"
	zkFQDN3           = "a-euc1-az3-zk3-1.yadc.io"

	ownerPassword   = "ownerPassword"
	adminPassword   = "adminPassword"
	monitorPassword = "monitorPassword"
	zkAdminPassword = "zkAdminPassword"
)

var zonesAB, zonesABC, zonesABCD []string
var resultOperation operations.Operation
var kafkaPresetID, zkPresetID resources.PresetID
var subnets []networkProvider.Subnet
var subnetA, subnetB, subnetC networkProvider.Subnet
var network networkProvider.Network
var networks []networkProvider.Network

func init() {
	zonesAB = []string{zoneA, zoneB}
	zonesABC = []string{zoneA, zoneB, zoneC}
	zonesABCD = []string{zoneA, zoneB, zoneC, zoneD}
	resultOperation = operations.Operation{OperationID: "operationId"}

	kafkaPresetUUID, _ := uuid.NewGen().NewV4()
	kafkaPresetID = resources.PresetID(kafkaPresetUUID)

	zkPresetUUID, _ := uuid.NewGen().NewV4()
	zkPresetID = resources.PresetID(zkPresetUUID)

	subnetA = networkProvider.Subnet{ID: subnetIDA}
	subnetB = networkProvider.Subnet{ID: subnetIDB}
	subnetC = networkProvider.Subnet{ID: subnetIDC}
	subnets = []networkProvider.Subnet{subnetA, subnetB, subnetC}
	network = networkProvider.Network{ID: networkID}
	networks = []networkProvider.Network{network}
}

func TestKafka_convertDataCloudArgs_checkFillingAllFields(t *testing.T) {
	kafkaFixture := newKafkaFixture(t)

	t.Run("Check minimal fields when networkID is set", func(t *testing.T) {
		args := kafka.CreateDataCloudClusterArgs{
			NetworkID: optional.NewString(networkID),
		}
		mockGenerateRandomString(kafkaFixture, ownerPassword)
		mockValidKafkaAndZookeeperDefaultResources(kafkaFixture)
		mockAvailableZones(kafkaFixture, "", "", 3, zonesABCD)

		result, err := kafkaFixture.Kafka.convertDataCloudArgs(kafkaFixture.Context, kafkaFixture.Session, kafkaFixture.Creator, args)

		require.NoError(t, err)
		require.Equal(t, createClusterImplArgs{
			ConfigSpec:     dataCloudConfigSpec("", []string{}, 0, clusters.Access{}, clusters.Encryption{}),
			UserSpecs:      validUserSpecs(),
			Environment:    environment.SaltEnvComputeProd,
			ZkZones:        zonesABC,
			OneHostMode:    false,
			ZkScramEnabled: true,
			SyncTopics:     true,
			NetworkID:      networkID,
			AdminUserName:  "admin",
		}, result)
	})

	t.Run("Check minimal fields when networkID is not set", func(t *testing.T) {
		args := kafka.CreateDataCloudClusterArgs{
			ProjectID: projectID,
			RegionID:  regionID,
		}
		mockComputeNetworks(kafkaFixture, projectID, regionID, networks, nil)
		mockGenerateRandomString(kafkaFixture, ownerPassword)
		mockValidKafkaAndZookeeperDefaultResources(kafkaFixture)
		mockAvailableZones(kafkaFixture, "", regionID, 3, zonesABCD)

		result, err := kafkaFixture.Kafka.convertDataCloudArgs(kafkaFixture.Context, kafkaFixture.Session, kafkaFixture.Creator, args)

		require.NoError(t, err)
		require.Equal(t, createClusterImplArgs{
			RegionID:       regionID,
			FolderExtID:    projectID,
			NetworkID:      networkID,
			ConfigSpec:     dataCloudConfigSpec("", []string{}, 0, clusters.Access{}, clusters.Encryption{}),
			UserSpecs:      validUserSpecs(),
			Environment:    environment.SaltEnvComputeProd,
			ZkZones:        zonesABC,
			OneHostMode:    false,
			ZkScramEnabled: true,
			SyncTopics:     true,
			AdminUserName:  "admin",
		}, result)
	})

	t.Run("When access is not valid should return error", func(t *testing.T) {
		args := kafka.CreateDataCloudClusterArgs{
			ClusterSpec: kfmodels.DataCloudClusterSpec{
				Access: clusters.Access{Ipv4CidrBlocks: []clusters.CidrBlock{{
					Value: "invalidValue",
				}}},
			},
		}
		mockGenerateRandomString(kafkaFixture, ownerPassword)
		mockValidKafkaAndZookeeperDefaultResources(kafkaFixture)
		mockAvailableZones(kafkaFixture, "", "", 3, zonesABCD)

		_, err := kafkaFixture.Kafka.convertDataCloudArgs(kafkaFixture.Context, kafkaFixture.Session, kafkaFixture.Creator, args)

		require.EqualError(t, err, "provided string could not be parsed as valid IP address in CIDR notation: invalidValue")
	})

	t.Run("When kafka version is allowed should not return error", func(t *testing.T) {
		args := kafka.CreateDataCloudClusterArgs{
			Name:        clusterName,
			RegionID:    regionID,
			ProjectID:   projectID,
			Description: clusterDescription,
			CloudType:   environment.CloudTypeAWS,
			ClusterSpec: kfmodels.DataCloudClusterSpec{
				Version:      kfmodels.Version2_8,
				BrokersCount: 1,
				ZoneCount:    3,
				Access:       validClustersAccess(),
				Encryption:   validClustersEncryption(),
			},
			NetworkID: optional.NewString(networkID),
		}
		mockGenerateRandomString(kafkaFixture, ownerPassword)
		mockValidKafkaAndZookeeperDefaultResources(kafkaFixture)
		mockAvailableZones(kafkaFixture, environment.CloudTypeAWS, regionID, 3, zonesABCD)

		result, err := kafkaFixture.Kafka.convertDataCloudArgs(kafkaFixture.Context, kafkaFixture.Session, kafkaFixture.Creator, args)

		require.NoError(t, err)
		require.Equal(t, createClusterImplArgs{
			Name:           clusterName,
			RegionID:       regionID,
			FolderExtID:    projectID,
			CloudType:      environment.CloudTypeAWS,
			ConfigSpec:     dataCloudConfigSpec(kfmodels.Version2_8, zonesABC, 1, validClustersAccess(), validClustersEncryption()),
			UserSpecs:      validUserSpecs(),
			Environment:    environment.SaltEnvComputeProd,
			Description:    clusterDescription,
			ZkZones:        zonesABC,
			OneHostMode:    false,
			ZkScramEnabled: true,
			SyncTopics:     true,
			NetworkID:      networkID,
			AdminUserName:  "admin",
		}, result)
	})
}

func TestKafka_selectDataCloudKafkaAndZookeeperHostZones(t *testing.T) {
	kafkaFixture := newKafkaFixture(t)

	t.Run("When not enough zones in region should return error", func(t *testing.T) {
		kafkaFixture.Creator.EXPECT().SelectZonesForCloudAndRegion(kafkaFixture.Context, kafkaFixture.Session, environment.CloudTypeYandex, regionID, true, 3).
			Return(nil, semerr.InvalidInput("not found enough available zones")).Times(1)

		_, _, err := selectDataCloudKafkaAndZookeeperHostZones(kafkaFixture.Context, kafkaFixture.Session, kafkaFixture.Creator, environment.CloudTypeYandex,
			regionID, 3, true)

		require.EqualError(t, err, "not found enough available zones")
	})

	t.Run("When one host mode should return empty zk zones", func(t *testing.T) {
		mockAvailableZones(kafkaFixture, environment.CloudTypeYandex, regionID, 2, []string{zoneA, zoneB})

		kafkaZones, zkZones, err := selectDataCloudKafkaAndZookeeperHostZones(kafkaFixture.Context, kafkaFixture.Session,
			kafkaFixture.Creator, environment.CloudTypeYandex, regionID, 2, true)

		require.NoError(t, err)
		require.Equal(t, zonesAB, kafkaZones)
		require.Equal(t, []string{}, zkZones)
	})

	t.Run("When not one host mode should return first three zk zones and kafka zones from parameter", func(t *testing.T) {
		mockAvailableZones(kafkaFixture, environment.CloudTypeYandex, regionID, 3, zonesABC)

		kafkaZones, zkZones, err := selectDataCloudKafkaAndZookeeperHostZones(kafkaFixture.Context, kafkaFixture.Session,
			kafkaFixture.Creator, environment.CloudTypeYandex, regionID, 2, false)

		require.NoError(t, err)
		require.Equal(t, zonesAB, kafkaZones)
		require.Equal(t, zonesABC, zkZones)
	})

	t.Run("When not one host mode and more zones selected then zk zones should return first three zk zones and "+
		"kafka zones from parameter", func(t *testing.T) {
		mockAvailableZones(kafkaFixture, environment.CloudTypeYandex, regionID, 4, zonesABCD)

		kafkaZones, zkZones, err := selectDataCloudKafkaAndZookeeperHostZones(kafkaFixture.Context, kafkaFixture.Session,
			kafkaFixture.Creator, environment.CloudTypeYandex, regionID, 4, false)

		require.NoError(t, err)
		require.Equal(t, zonesABCD, kafkaZones)
		require.Equal(t, zonesABC, zkZones)
	})
}

func TestKafka_CreateDataCloudCluster(t *testing.T) {
	t.Run("Minimal happy path for 3 zones and 1 brokers in aws", func(t *testing.T) {
		kafkaFixture := initKafkaFixture(t)
		args := kafka.CreateDataCloudClusterArgs{
			Name:        clusterName,
			RegionID:    regionID,
			ProjectID:   projectID,
			Description: clusterDescription,
			CloudType:   environment.CloudTypeAWS,
			ClusterSpec: kfmodels.DataCloudClusterSpec{
				Version:      kfmodels.Version2_8,
				BrokersCount: 1,
				ZoneCount:    3,
				Access:       validClustersAccess(),
				Encryption:   validClustersEncryption(),
				Kafka:        kfmodels.KafkaConfigSpec{Resources: validKafkaResources()},
			},
		}
		mockValidKafkaAndZookeeperDefaultResources(kafkaFixture)
		mockCreatingCluster(kafkaFixture, environment.SaltEnvComputeProd)
		mockOperatorCreate(kafkaFixture, projectID)
		mockAWSPasswords(kafkaFixture, privateKey)
		mockCreatingAWSHosts(kafkaFixture)

		assertValidPillar(kafkaFixture, validAwsKafkaPillar())
		assertCreateTask(kafkaFixture, resultOperation)

		actualOperation, err := kafkaFixture.Kafka.CreateDataCloudCluster(kafkaFixture.Context, args)

		require.NoError(t, err)
		require.Equal(t, resultOperation, actualOperation)
	})
}

func TestKafka_CreateMDBCluster(t *testing.T) {

	t.Run("Create kafka cluster successfully", func(t *testing.T) {
		kafkaFixture := initMdbKafkaFixture(t)
		args := kafka.CreateMDBClusterArgs{
			Name:        clusterName,
			Description: clusterDescription,
			FolderExtID: projectID,
			Environment: environment.SaltEnvDev,
			NetworkID:   networkID,
			ConfigSpec: kfmodels.MDBClusterSpec{
				UnmanagedTopics: true,
				Version:         kfmodels.Version3_2,
				BrokersCount:    1,
				ZoneID:          []string{zoneA, zoneB, zoneC},
			},
		}
		mockOperatorCreate(kafkaFixture, projectID)
		mockMDBPasswords(kafkaFixture, privateKey)
		mockCreatingCluster(kafkaFixture, environment.SaltEnvDev)
		mockCreatingMDBHosts(kafkaFixture)
		assertValidPillar(kafkaFixture, validMdbKafkaPillar())
		assertCreateTask(kafkaFixture, resultOperation)

		actualOperation, err := kafkaFixture.Kafka.CreateMDBCluster(kafkaFixture.Context, args)

		require.NoError(t, err)
		require.Equal(t, resultOperation, actualOperation)
	})

	t.Run("Create kafka 3.2 in production env should return error", func(t *testing.T) {
		kafkaFixture := initMdbKafkaFixture(t)
		args := kafka.CreateMDBClusterArgs{
			Name:        clusterName,
			Description: clusterDescription,
			FolderExtID: projectID,
			Environment: environment.SaltEnvComputeProd,

			ConfigSpec: kfmodels.MDBClusterSpec{
				Version:      kfmodels.Version3_2,
				BrokersCount: 1,
				ZoneID:       []string{zoneA},
			},
		}
		mockOperatorCreate(kafkaFixture, projectID)

		_, err := kafkaFixture.Kafka.CreateMDBCluster(kafkaFixture.Context, args)

		require.EqualError(t, err, "version 3.2 can be created only in prestable environment")
	})
}

func initKafkaFixture(t *testing.T) kafkaFixture {
	kafkaFixture := newKafkaFixture(t)
	kafkaFixture.Session.FolderCoords.CloudExtID = cloudExtID
	kafkaFixture.Session.FolderCoords.FolderExtID = folderExtID
	kafkaFixture.Session.FolderCoords.FolderID = sessionFolderCoordsFolderID

	return kafkaFixture
}

func initMdbKafkaFixture(t *testing.T) kafkaFixture {
	kafkaFixture := initKafkaFixture(t)
	kafkaFixture.Session.FeatureFlags = featureflags.NewFeatureFlags([]string{kfmodels.KafkaClusterFeatureFlag})
	kafkaFixture.Kafka.cfg.SaltEnvs.Prestable = environment.SaltEnvDev
	kafkaFixture.Kafka.cfg.Kafka.ZooKeeperZones = zonesABC
	kafkaFixture.Kafka.cfg.Kafka.SyncTopics = true
	kafkaFixture.Kafka.cfg.DefaultResources.ByClusterType = map[string]map[string]logic.DefaultResourcesRecord{
		clusters.TypeKafka.Stringified(): {
			hosts.RoleKafka.Stringified(): {
				ResourcePresetExtID: kafkaResourcePresetExtID,
				DiskTypeExtID:       kafkaDiskTypeExtID,
				DiskSize:            kafkaDiskSize,
			},
			hosts.RoleZooKeeper.Stringified(): {
				ResourcePresetExtID: zkResourcePresetExtID,
				DiskTypeExtID:       zkDiskTypeExtID,
				DiskSize:            zkDiskSize,
			},
		},
	}

	return kafkaFixture
}

func validAwsKafkaPillar() *kfpillars.Cluster {
	return &kfpillars.Cluster{
		Data: kfpillars.ClusterData{
			Kafka: kfpillars.KafkaData{
				AdminPassword:   *cryptoKey(adminPassword),
				MonitorPassword: *cryptoKey(monitorPassword),
				Users: map[string]kfpillars.UserData{
					dataCloudOwnerUserName: {
						Name:     dataCloudOwnerUserName,
						Password: *cryptoKey(ownerPassword),
						Permissions: []kfpillars.PermissionsData{
							{
								TopicName: "*",
								Role:      "admin",
							},
						},
					},
				},
				Nodes: map[string]kfpillars.KafkaNodeData{
					kafkaFQDN1: {
						ID:          1,
						FQDN:        kafkaFQDN1,
						PrivateFQDN: privateKafkaFQDN1,
						Rack:        zoneA,
					},
					kafkaFQDN2: {
						ID:          2,
						FQDN:        kafkaFQDN2,
						PrivateFQDN: privateKafkaFQDN2,
						Rack:        zoneB,
					},
					kafkaFQDN3: {
						ID:          3,
						FQDN:        kafkaFQDN3,
						PrivateFQDN: privateKafkaFQDN3,
						Rack:        zoneC,
					},
				},
				MainBrokerID:                  1,
				BrokersCount:                  1,
				AssignPublicIP:                false,
				ZoneID:                        zonesABC,
				Version:                       kfmodels.Version2_8,
				InterBrokerProtocolVersion:    kfmodels.Version2_8,
				PackageVersion:                "2.8.1.1-java11",
				HasZkSubcluster:               true,
				UnmanagedTopics:               true,
				SyncTopics:                    true,
				UseCustomAuthorizerForCluster: true,
				AclsViaPy4j:                   true,
				UsePlainSasl:                  true,
				AdminUserName:                 "admin",
				KnownTopicConfigProperties: []string{
					"cleanup.policy",
					"compression.type",
					"delete.retention.ms",
					"file.delete.delay.ms",
					"flush.messages",
					"flush.ms",
					"max.message.bytes",
					"min.compaction.lag.ms",
					"min.insync.replicas",
					"preallocate",
					"retention.bytes",
					"retention.ms",
					"segment.bytes",
				},
				Resources:         validKafkaResources(),
				Topics:            map[string]kfpillars.TopicData{},
				DeletedTopics:     map[string]kfpillars.DeletedTopic{},
				DeletedUsers:      map[string]kfpillars.DeletedUser{},
				Connectors:        map[string]kfpillars.ConnectorData{},
				DeletedConnectors: map[string]kfpillars.DeletedConnector{},
			},
			ZooKeeper: kfpillars.ZooKeeperData{
				Resources: validZkResources(),
				Config: kfpillars.ZooKeeperConfig{
					DataDir: "/var/lib/zookeeper",
				},
				Nodes: map[string]int{
					zkFQDN1: 1,
					zkFQDN2: 2,
					zkFQDN3: 3,
				},
				ScramAuthEnabled:   true,
				ScramAdminPassword: cryptoKey(zkAdminPassword),
			},
			ClusterPrivateKey: *cryptoKey(privateKey),
			CloudType:         environment.CloudTypeAWS,
			RegionID:          regionID,
			Access:            validAccessSettings(),
			Encryption:        validEncryptionSettings(),
		},
	}
}

func validMdbKafkaPillar() *kfpillars.Cluster {
	return &kfpillars.Cluster{
		Data: kfpillars.ClusterData{
			Kafka: kfpillars.KafkaData{
				AdminPassword:   *cryptoKey(adminPassword),
				MonitorPassword: *cryptoKey(monitorPassword),
				Users:           map[string]kfpillars.UserData{},
				Nodes: map[string]kfpillars.KafkaNodeData{
					kafkaFQDN1: {
						ID:          1,
						FQDN:        kafkaFQDN1,
						PrivateFQDN: privateKafkaFQDN1,
						Rack:        zoneA,
					},
					kafkaFQDN2: {
						ID:          2,
						FQDN:        kafkaFQDN2,
						PrivateFQDN: privateKafkaFQDN2,
						Rack:        zoneB,
					},
					kafkaFQDN3: {
						ID:          3,
						FQDN:        kafkaFQDN3,
						PrivateFQDN: privateKafkaFQDN3,
						Rack:        zoneC,
					},
				},
				MainBrokerID:                  1,
				BrokersCount:                  1,
				AssignPublicIP:                false,
				ZoneID:                        zonesABC,
				Version:                       kfmodels.Version3_2,
				InterBrokerProtocolVersion:    kfmodels.Version3_2,
				PackageVersion:                "3.2.0-java11",
				HasZkSubcluster:               true,
				UnmanagedTopics:               true,
				SyncTopics:                    true,
				UseCustomAuthorizerForCluster: true,
				AclsViaPy4j:                   true,
				UsePlainSasl:                  true,
				KnownTopicConfigProperties: []string{
					"cleanup.policy",
					"compression.type",
					"delete.retention.ms",
					"file.delete.delay.ms",
					"flush.messages",
					"flush.ms",
					"max.message.bytes",
					"min.compaction.lag.ms",
					"min.insync.replicas",
					"preallocate",
					"retention.bytes",
					"retention.ms",
					"segment.bytes",
				},
				Resources:         validKafkaResources(),
				Topics:            map[string]kfpillars.TopicData{},
				DeletedTopics:     map[string]kfpillars.DeletedTopic{},
				DeletedUsers:      map[string]kfpillars.DeletedUser{},
				Connectors:        map[string]kfpillars.ConnectorData{},
				DeletedConnectors: map[string]kfpillars.DeletedConnector{},
			},
			ZooKeeper: kfpillars.ZooKeeperData{
				Resources: validZkResources(),
				Config: kfpillars.ZooKeeperConfig{
					DataDir: "/var/lib/zookeeper",
				},
				Nodes: map[string]int{
					zkFQDN1: 1,
					zkFQDN2: 2,
					zkFQDN3: 3,
				},
			},
			ClusterPrivateKey: *cryptoKey(privateKey),
			CloudType:         environment.CloudTypeYandex,
		},
	}
}

func assertValidPillar(kafkaFixture kafkaFixture, expectedPillar *kfpillars.Cluster) {
	kafkaFixture.Creator.EXPECT().
		AddClusterPillar(kafkaFixture.Context, clusterID, revisionID, gomock.Any()).
		DoAndReturn(func(ctx context.Context, cid string, revision int64, pillar *kfpillars.Cluster) error {
			require.Equal(kafkaFixture.T, expectedPillar, pillar)
			return nil
		})
}

func assertCreateTask(kafkaFixture kafkaFixture, resultOperation operations.Operation) {
	kafkaFixture.Tasks.EXPECT().
		CreateCluster(
			kafkaFixture.Context,
			kafkaFixture.Session,
			clusterID,
			revisionID,
			kfmodels.TaskTypeClusterCreate,
			kfmodels.OperationTypeClusterCreate,
			kfmodels.MetadataCreateCluster{},
			optional.String{},
			nil,
			kafkaService,
			gomock.Any()).
		Return(resultOperation, nil)
}

func cryptoKey(key string) *pillars.CryptoKey {
	return &pillars.CryptoKey{
		Data:              key,
		EncryptionVersion: 1,
	}
}

func mockComputeNetworks(kafkaFixture kafkaFixture, projectID string, regionID string, returnedNetwork []networkProvider.Network, returnedError error) {
	kafkaFixture.Compute.EXPECT().
		Networks(kafkaFixture.Context, projectID, regionID).
		Return(returnedNetwork, returnedError)
}

func mockComputeNetworkAndSubnets(kafkaFixture kafkaFixture, networkID string, returnedNetwork networkProvider.Network,
	returnedSubnets []networkProvider.Subnet, returnedError error) {
	kafkaFixture.Compute.EXPECT().
		NetworkAndSubnets(kafkaFixture.Context, networkID).
		Return(returnedNetwork, returnedSubnets, returnedError)
}

func mockCreatingCluster(kafkaFixture kafkaFixture, env environment.SaltEnv) {
	mockCreatorCreateCluster(kafkaFixture, models.CreateClusterArgs{
		Name:        clusterName,
		ClusterType: clusters.TypeKafka,
		Environment: env,
		FolderID:    sessionFolderCoordsFolderID,
		Description: clusterDescription,
		NetworkID:   networkID,
	}, clusters.Cluster{
		ClusterID: clusterID,
		Revision:  revisionID,
	}, []byte(privateKey), nil)

	mockCreatorCreateSubCluster(kafkaFixture, models.CreateSubClusterArgs{
		ClusterID: clusterID,
		Name:      kfmodels.KafkaSubClusterName,
		Roles:     []hosts.Role{hosts.RoleKafka},
		Revision:  revisionID,
	}, clusters.SubCluster{SubClusterID: kafkaSubClusterID}, nil)

	mockCreatorCreateSubCluster(kafkaFixture, models.CreateSubClusterArgs{
		ClusterID: clusterID,
		Name:      kfmodels.ZooKeeperSubClusterName,
		Roles:     []hosts.Role{hosts.RoleZooKeeper},
		Revision:  revisionID,
	}, clusters.SubCluster{SubClusterID: zkSubClusterID}, nil)
}

func mockCreatingMDBHosts(kafkaFixture kafkaFixture) {
	mockComputeNetworkAndSubnets(kafkaFixture, networkID, network, subnets, nil)
	mockValidResources(kafkaFixture, environment.VTypeCompute, kafkaPresetID, zkPresetID)
	mockValidKafkaFQDNs(kafkaFixture, environment.CloudTypeYandex, environment.VTypeCompute)
	mockValidZkFQDNs(kafkaFixture, environment.CloudTypeYandex, environment.VTypeCompute)
	mockComputePickSubnet(kafkaFixture, subnets, environment.VTypeCompute, zoneA, false, folderExtID, subnetA, nil)
	mockComputePickSubnet(kafkaFixture, subnets, environment.VTypeCompute, zoneA, false, folderExtID, subnetA, nil)
	mockComputePickSubnet(kafkaFixture, subnets, environment.VTypeCompute, zoneB, false, folderExtID, subnetB, nil)
	mockComputePickSubnet(kafkaFixture, subnets, environment.VTypeCompute, zoneB, false, folderExtID, subnetB, nil)
	mockComputePickSubnet(kafkaFixture, subnets, environment.VTypeCompute, zoneC, false, folderExtID, subnetC, nil)
	mockComputePickSubnet(kafkaFixture, subnets, environment.VTypeCompute, zoneC, false, folderExtID, subnetC, nil)
	mockCreatorAddHost(kafkaFixture, []models.AddHostArgs{
		{
			SubClusterID:     kafkaSubClusterID,
			SpaceLimit:       kafkaDiskSize,
			DiskTypeExtID:    kafkaDiskTypeExtID,
			Revision:         revisionID,
			ClusterID:        clusterID,
			AssignPublicIP:   false,
			ResourcePresetID: kafkaPresetID,
			ZoneID:           zoneA,
			SubnetID:         subnetIDA,
			FQDN:             kafkaFQDN1,
		},
		{
			SubClusterID:     kafkaSubClusterID,
			SpaceLimit:       kafkaDiskSize,
			DiskTypeExtID:    kafkaDiskTypeExtID,
			Revision:         revisionID,
			ClusterID:        clusterID,
			AssignPublicIP:   false,
			ResourcePresetID: kafkaPresetID,
			ZoneID:           zoneB,
			SubnetID:         subnetIDB,
			FQDN:             kafkaFQDN2,
		},
		{
			SubClusterID:     kafkaSubClusterID,
			SpaceLimit:       kafkaDiskSize,
			DiskTypeExtID:    kafkaDiskTypeExtID,
			Revision:         revisionID,
			ClusterID:        clusterID,
			AssignPublicIP:   false,
			ResourcePresetID: kafkaPresetID,
			ZoneID:           zoneC,
			SubnetID:         subnetIDC,
			FQDN:             kafkaFQDN3,
		},
	}, nil, nil)

	mockCreatorAddHost(kafkaFixture, []models.AddHostArgs{
		{
			SubClusterID:     zkSubClusterID,
			SpaceLimit:       zkDiskSize,
			DiskTypeExtID:    zkDiskTypeExtID,
			Revision:         revisionID,
			ClusterID:        clusterID,
			AssignPublicIP:   false,
			ResourcePresetID: zkPresetID,
			ZoneID:           zoneA,
			SubnetID:         subnetIDA,
			FQDN:             zkFQDN1,
		},
		{
			SubClusterID:     zkSubClusterID,
			SpaceLimit:       zkDiskSize,
			DiskTypeExtID:    zkDiskTypeExtID,
			Revision:         revisionID,
			ClusterID:        clusterID,
			AssignPublicIP:   false,
			ResourcePresetID: zkPresetID,
			ZoneID:           zoneB,
			SubnetID:         subnetIDB,
			FQDN:             zkFQDN2,
		},
		{
			SubClusterID:     zkSubClusterID,
			SpaceLimit:       zkDiskSize,
			DiskTypeExtID:    zkDiskTypeExtID,
			Revision:         revisionID,
			ClusterID:        clusterID,
			AssignPublicIP:   false,
			ResourcePresetID: zkPresetID,
			ZoneID:           zoneC,
			SubnetID:         subnetIDC,
			FQDN:             zkFQDN3,
		},
	}, nil, nil)
}

func mockCreatingAWSHosts(kafkaFixture kafkaFixture) {
	mockComputeNetworks(kafkaFixture, projectID, regionID, networks, nil)
	mockComputeNetworkAndSubnets(kafkaFixture, networkID, network, subnets, nil)
	mockValidResources(kafkaFixture, environment.VTypeAWS, kafkaPresetID, zkPresetID)
	mockAvailableZones(kafkaFixture, environment.CloudTypeAWS, regionID, 3, zonesABCD)
	mockComputePickSubnet(kafkaFixture, subnets, environment.VTypeAWS, zoneA, false, folderExtID, subnetA, nil)
	mockComputePickSubnet(kafkaFixture, subnets, environment.VTypeAWS, zoneA, false, folderExtID, subnetA, nil)
	mockComputePickSubnet(kafkaFixture, subnets, environment.VTypeAWS, zoneB, false, folderExtID, subnetB, nil)
	mockComputePickSubnet(kafkaFixture, subnets, environment.VTypeAWS, zoneB, false, folderExtID, subnetB, nil)
	mockComputePickSubnet(kafkaFixture, subnets, environment.VTypeAWS, zoneC, false, folderExtID, subnetC, nil)
	mockComputePickSubnet(kafkaFixture, subnets, environment.VTypeAWS, zoneC, false, folderExtID, subnetC, nil)
	mockValidKafkaFQDNs(kafkaFixture, environment.CloudTypeAWS, environment.VTypeAWS)
	mockValidZkFQDNs(kafkaFixture, environment.CloudTypeAWS, environment.VTypeAWS)

	mockCreatorAddHost(kafkaFixture, []models.AddHostArgs{
		{
			SubClusterID:     kafkaSubClusterID,
			SpaceLimit:       kafkaDiskSize,
			DiskTypeExtID:    kafkaDiskTypeExtID,
			Revision:         revisionID,
			ClusterID:        clusterID,
			AssignPublicIP:   false,
			ResourcePresetID: kafkaPresetID,
			ZoneID:           zoneA,
			SubnetID:         subnetIDA,
			FQDN:             kafkaFQDN1,
		},
		{
			SubClusterID:     kafkaSubClusterID,
			SpaceLimit:       kafkaDiskSize,
			DiskTypeExtID:    kafkaDiskTypeExtID,
			Revision:         revisionID,
			ClusterID:        clusterID,
			AssignPublicIP:   false,
			ResourcePresetID: kafkaPresetID,
			ZoneID:           zoneB,
			SubnetID:         subnetIDB,
			FQDN:             kafkaFQDN2,
		},
		{
			SubClusterID:     kafkaSubClusterID,
			SpaceLimit:       kafkaDiskSize,
			DiskTypeExtID:    kafkaDiskTypeExtID,
			Revision:         revisionID,
			ClusterID:        clusterID,
			AssignPublicIP:   false,
			ResourcePresetID: kafkaPresetID,
			ZoneID:           zoneC,
			SubnetID:         subnetIDC,
			FQDN:             kafkaFQDN3,
		},
	}, nil, nil)

	mockCreatorAddHost(kafkaFixture, []models.AddHostArgs{
		{
			SubClusterID:     zkSubClusterID,
			SpaceLimit:       zkDiskSize,
			DiskTypeExtID:    zkDiskTypeExtID,
			Revision:         revisionID,
			ClusterID:        clusterID,
			AssignPublicIP:   false,
			ResourcePresetID: zkPresetID,
			ZoneID:           zoneA,
			SubnetID:         subnetIDA,
			FQDN:             zkFQDN1,
		},
		{
			SubClusterID:     zkSubClusterID,
			SpaceLimit:       zkDiskSize,
			DiskTypeExtID:    zkDiskTypeExtID,
			Revision:         revisionID,
			ClusterID:        clusterID,
			AssignPublicIP:   false,
			ResourcePresetID: zkPresetID,
			ZoneID:           zoneB,
			SubnetID:         subnetIDB,
			FQDN:             zkFQDN2,
		},
		{
			SubClusterID:     zkSubClusterID,
			SpaceLimit:       zkDiskSize,
			DiskTypeExtID:    zkDiskTypeExtID,
			Revision:         revisionID,
			ClusterID:        clusterID,
			AssignPublicIP:   false,
			ResourcePresetID: zkPresetID,
			ZoneID:           zoneC,
			SubnetID:         subnetIDC,
			FQDN:             zkFQDN3,
		},
	}, nil, nil)
}

func mockCreatorAddHost(kafkaFixture kafkaFixture, expectedArgs []models.AddHostArgs, returnedValue []hosts.Host, returnedError error) {
	kafkaFixture.Creator.EXPECT().
		AddHosts(kafkaFixture.Context, gomock.Any()).
		DoAndReturn(func(ctx context.Context, actualArgs []models.AddHostArgs) ([]hosts.Host, error) {
			sort.Slice(expectedArgs, func(i, j int) bool {
				return expectedArgs[i].FQDN < expectedArgs[j].FQDN
			})
			sort.Slice(actualArgs, func(i, j int) bool {
				return actualArgs[i].FQDN < actualArgs[j].FQDN
			})
			require.EqualValues(kafkaFixture.T, expectedArgs, actualArgs)
			return returnedValue, returnedError
		})
}

func mockValidKafkaFQDNs(kafkaFixture kafkaFixture, cloudType environment.CloudType, vType environment.VType) {
	zonesList := clusterslogic.ZoneHostsList{
		clusterslogic.ZoneHosts{ZoneID: zoneA, Count: 1},
		clusterslogic.ZoneHosts{ZoneID: zoneB, Count: 1},
		clusterslogic.ZoneHosts{ZoneID: zoneC, Count: 1},
	}
	result := map[string][]string{
		zoneA: {kafkaFQDN1},
		zoneB: {kafkaFQDN2},
		zoneC: {kafkaFQDN3},
	}
	mockCreatorGenerateSemanticFQDNs(kafkaFixture, cloudType, zonesList,
		vType, clusterID, "", result, nil)
}

func mockValidZkFQDNs(kafkaFixture kafkaFixture, cloudType environment.CloudType, vType environment.VType) {
	zonesList := clusterslogic.ZoneHostsList{
		clusterslogic.ZoneHosts{ZoneID: zoneA, Count: 1},
		clusterslogic.ZoneHosts{ZoneID: zoneB, Count: 1},
		clusterslogic.ZoneHosts{ZoneID: zoneC, Count: 1},
	}
	result := map[string][]string{
		zoneA: {zkFQDN1},
		zoneB: {zkFQDN2},
		zoneC: {zkFQDN3},
	}
	mockCreatorGenerateSemanticFQDNs(kafkaFixture, cloudType, zonesList,
		vType, clusterID, "zk", result, nil)
}

func mockCreatorGenerateSemanticFQDNs(kafkaFixture kafkaFixture, cloudType environment.CloudType, zonesList clusterslogic.ZoneHostsList,
	vType environment.VType, cid string, shardName string, returnedValue map[string][]string, returnedError error) {
	kafkaFixture.Creator.EXPECT().
		GenerateSemanticFQDNs(cloudType, clusters.TypeKafka, zonesList,
			nil, shardName, cid, vType, compute.Ubuntu).
		Return(returnedValue, returnedError)
}

func mockValidResources(kafkaFixture kafkaFixture, vType environment.VType, kafkaPresetID, zkPresetID resources.PresetID) {
	hostsToAdd := []clusterslogic.ZoneHosts{
		{
			ZoneID: zoneA,
			Count:  1,
		},
		{
			ZoneID: zoneB,
			Count:  1,
		},
		{
			ZoneID: zoneC,
			Count:  1,
		},
	}
	kafkaResourcePreset := resources.Preset{
		ID:    kafkaPresetID,
		VType: vType,
	}
	mockCreatorValidateResources(kafkaFixture, clusterslogic.HostGroup{
		Role:                   hosts.RoleKafka,
		NewResourcePresetExtID: optional.NewString(kafkaResourcePresetExtID),
		DiskTypeExtID:          kafkaDiskTypeExtID,
		NewDiskSize:            optional.NewInt64(kafkaDiskSize),
		HostsToAdd:             hostsToAdd,
	}, clusterslogic.NewResolvedHostGroups([]clusterslogic.ResolvedHostGroup{{
		HostGroup: clusterslogic.HostGroup{
			Role:                   hosts.RoleKafka,
			NewResourcePresetExtID: optional.NewString(kafkaResourcePresetExtID),
			HostsToAdd:             hostsToAdd,
		},
		NewResourcePreset: kafkaResourcePreset,
	},
	}), true, nil)

	zkResourcePreset := resources.Preset{
		ID:    zkPresetID,
		VType: vType,
	}
	mockCreatorValidateResources(kafkaFixture, clusterslogic.HostGroup{
		Role:                   hosts.RoleZooKeeper,
		NewResourcePresetExtID: optional.NewString(zkResourcePresetExtID),
		DiskTypeExtID:          zkDiskTypeExtID,
		NewDiskSize:            optional.NewInt64(zkDiskSize),
		HostsToAdd:             hostsToAdd,
		SkipValidations: clusterslogic.SkipValidations{
			DecommissionedZone: true,
		},
	}, clusterslogic.NewResolvedHostGroups([]clusterslogic.ResolvedHostGroup{{
		HostGroup: clusterslogic.HostGroup{
			Role:                   hosts.RoleZooKeeper,
			NewResourcePresetExtID: optional.NewString(zkResourcePresetExtID),
			HostsToAdd:             hostsToAdd,
		},
		NewResourcePreset: zkResourcePreset,
	},
	}), true, nil)
}

func mockMDBPasswords(kafkaFixture kafkaFixture, privateKey string) {
	mockGenerateEncryptedPassword(kafkaFixture, adminPassword)
	mockGenerateEncryptedPassword(kafkaFixture, monitorPassword)
	mockCryptoProviderEncrypt(kafkaFixture, privateKey)
}

func mockAWSPasswords(kafkaFixture kafkaFixture, privateKey string) {
	mockGenerateEncryptedPassword(kafkaFixture, ownerPassword)
	mockGenerateEncryptedPassword(kafkaFixture, adminPassword)
	mockGenerateEncryptedPassword(kafkaFixture, monitorPassword)
	mockGenerateEncryptedPassword(kafkaFixture, zkAdminPassword)
	mockCryptoProviderEncrypt(kafkaFixture, privateKey)
}

func mockComputePickSubnet(kafkaFixture kafkaFixture, subnets []networkProvider.Subnet, vtype environment.VType, geo string,
	publicIP bool, clusterFolderExtID string, returnedVpc networkProvider.Subnet, returnedError error) {
	kafkaFixture.Compute.EXPECT().
		PickSubnet(kafkaFixture.Context, subnets, vtype, geo, publicIP, optional.String{}, clusterFolderExtID).
		Return(returnedVpc, returnedError)
}

func mockCreatorCreateSubCluster(kafkaFixture kafkaFixture, expectedArgs models.CreateSubClusterArgs, returnedSubCluster clusters.SubCluster, returnedError error) {
	kafkaFixture.Creator.EXPECT().
		CreateSubCluster(kafkaFixture.Context, expectedArgs).
		Return(returnedSubCluster, returnedError).
		Times(1)
}

func mockCreatorValidateResources(kafkaFixture kafkaFixture, expectedHostGroup clusterslogic.HostGroup,
	resolvedHostGroups clusterslogic.ResolvedHostGroups, returnedHasChanges bool, returnedError error) {
	kafkaFixture.Creator.EXPECT().
		ValidateResources(kafkaFixture.Context, kafkaFixture.Session, clusters.TypeKafka, gomock.Any()).
		DoAndReturn(func(ctx context.Context, session sessions.Session, typ clusters.Type,
			hostGroups ...clusterslogic.HostGroup) (clusterslogic.ResolvedHostGroups, bool, error) {

			require.Equal(kafkaFixture.T, expectedHostGroup, hostGroups[0])
			return resolvedHostGroups, returnedHasChanges, returnedError
		}).
		Times(1)
}

func mockCreatorCreateCluster(kafkaFixture kafkaFixture, expectedCreateClusterArgs models.CreateClusterArgs,
	returnedCluster clusters.Cluster, returnedPrivKey []byte, returnedError error) {
	kafkaFixture.Creator.EXPECT().
		CreateCluster(kafkaFixture.Context, gomock.Any()).
		DoAndReturn(func(ctx context.Context, args models.CreateClusterArgs) (clusters.Cluster, []byte, error) {
			require.Equal(kafkaFixture.T, expectedCreateClusterArgs, args)
			return returnedCluster, returnedPrivKey, returnedError
		}).
		Times(1)
}

func mockValidKafkaAndZookeeperDefaultResources(kafkaFixture kafkaFixture) {
	kafkaFixture.Kafka.cfg.DefaultResources = logic.DefaultResourcesConfig{
		ByClusterType: map[string]map[string]logic.DefaultResourcesRecord{
			clusters.TypeKafka.Stringified(): {
				hosts.RoleKafka.Stringified(): logic.DefaultResourcesRecord{
					ResourcePresetExtID: kafkaResourcePresetExtID,
					DiskSize:            kafkaDiskSize,
					DiskTypeExtID:       kafkaDiskTypeExtID,
				},
				hosts.RoleZooKeeper.Stringified(): logic.DefaultResourcesRecord{
					ResourcePresetExtID: zkResourcePresetExtID,
					DiskSize:            zkDiskSize,
					DiskTypeExtID:       zkDiskTypeExtID,
				},
			},
		},
	}
}

func mockGenerateRandomString(kafkaFixture kafkaFixture, generatedPassword string) {
	generatedPasswordSecret := secret.NewString(generatedPassword)
	kafkaFixture.CryptoProvider.
		EXPECT().GenerateRandomString(passwordGenLen, []rune(crypto.PasswordValidRunes)).
		Return(generatedPasswordSecret, nil)
}

func mockCryptoProviderEncrypt(kafkaFixture kafkaFixture, msg string) {
	kafkaFixture.CryptoProvider.EXPECT().Encrypt([]byte(msg)).Return(pillars.CryptoKey{
		Data:              msg,
		EncryptionVersion: 1,
	}, nil)
}

func mockGenerateEncryptedPassword(kafkaFixture kafkaFixture, generatedPassword string) {
	generatedPasswordSecret := secret.NewString(generatedPassword)
	kafkaFixture.CryptoProvider.
		EXPECT().GenerateRandomString(passwordGenLen, []rune(crypto.PasswordValidRunes)).
		Return(generatedPasswordSecret, nil)

	mockCryptoProviderEncrypt(kafkaFixture, generatedPassword)
}

func mockAvailableZones(kafkaFixture kafkaFixture, cloudType environment.CloudType,
	regionID string, zoneNumber int, availabilityZones []string) {
	kafkaFixture.Creator.EXPECT().SelectZonesForCloudAndRegion(kafkaFixture.Context, kafkaFixture.Session, cloudType, regionID, true, zoneNumber).
		Return(availabilityZones, nil).Times(1)
}

func mockOperatorCreate(kafkaFixture kafkaFixture, projectID string) {
	kafkaFixture.Operator.EXPECT().Create(gomock.Any(), projectID, gomock.Any(), gomock.Any()).
		DoAndReturn(func(ctx context.Context, folderExtID string, do clusterslogic.CreateFunc,
			opts ...clusterslogic.OperatorOption) (operations.Operation, error) {
			_, op, err := do(ctx, kafkaFixture.Session, kafkaFixture.Creator)
			return op, err
		}).AnyTimes()
}

func validClustersEncryption() clusters.Encryption {
	return clusters.Encryption{
		Enabled: optional.NewBool(true),
	}
}

func validZkConfigSpec() kfmodels.ZookeperConfigSpec {
	return kfmodels.ZookeperConfigSpec{
		Resources: validZkResources(),
	}
}

func validZkResources() models.ClusterResources {
	return models.ClusterResources{
		ResourcePresetExtID: zkResourcePresetExtID,
		DiskSize:            zkDiskSize,
		DiskTypeExtID:       zkDiskTypeExtID,
	}
}

func validKafkaResources() models.ClusterResources {
	return models.ClusterResources{
		ResourcePresetExtID: kafkaResourcePresetExtID,
		DiskSize:            kafkaDiskSize,
		DiskTypeExtID:       kafkaDiskTypeExtID,
	}
}

func validKafkaConfigSpec() kfmodels.KafkaConfigSpec {
	return kfmodels.KafkaConfigSpec{
		Resources: models.ClusterResources{
			DiskTypeExtID: kafkaDiskTypeExtID,
		},
	}
}

func validUserSpecs() []kfmodels.UserSpec {
	return []kfmodels.UserSpec{{
		Name:     dataCloudOwnerUserName,
		Password: secret.NewString(ownerPassword),
		Permissions: []kfmodels.Permission{{
			TopicName:  "*",
			AccessRole: kfmodels.AccessRoleAdmin,
		}},
	}}
}

func dataCloudConfigSpec(version string, zoneID []string, brokersCount int64, access clusters.Access, encryption clusters.Encryption) kfmodels.ClusterConfigSpec {
	return kfmodels.ClusterConfigSpec{
		Version:         version,
		Kafka:           validKafkaConfigSpec(),
		ZooKeeper:       validZkConfigSpec(),
		ZoneID:          zoneID,
		BrokersCount:    brokersCount,
		UnmanagedTopics: true,
		SyncTopics:      true,
		Access:          access,
		Encryption:      encryption,
	}
}

func validAccessSettings() kfpillars.AccessSettings {
	return kfpillars.AccessSettings{
		DataTransfer:   ptr.Bool(true),
		Ipv4CidrBlocks: validIpv4CidrBlocks(),
		Ipv6CidrBlocks: validIpv6CidrBlocks(),
	}
}

func validClustersAccess() clusters.Access {
	return clusters.Access{
		Ipv4CidrBlocks: validIpv4CidrBlocks(),
		Ipv6CidrBlocks: validIpv6CidrBlocks(),
		DataTransfer:   optional.NewBool(true),
	}
}

func validIpv4CidrBlocks() []clusters.CidrBlock {
	return []clusters.CidrBlock{
		{
			Value:       "0.0.0.0/0",
			Description: "ipv4 first",
		},
		{
			Value:       "192.168.1.10/32",
			Description: "ipv4 second",
		},
	}
}

func validIpv6CidrBlocks() []clusters.CidrBlock {
	return []clusters.CidrBlock{
		{
			Value:       "::/0",
			Description: "ipv6 first",
		},
		{
			Value:       "::/1",
			Description: "ipv6 second",
		},
	}
}

func validEncryptionSettings() kfpillars.EncryptionSettings {
	return kfpillars.EncryptionSettings{
		Enabled: ptr.Bool(true),
	}
}
