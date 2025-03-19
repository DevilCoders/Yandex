package clickhouse

import (
	"fmt"
	"strings"

	"github.com/golang/protobuf/ptypes/timestamp"
	"google.golang.org/protobuf/types/known/wrapperspb"

	chconsolev1 "a.yandex-team.ru/cloud/mdb/datacloud/private_api/datacloud/clickhouse/console/v1"
	chv1 "a.yandex-team.ru/cloud/mdb/datacloud/private_api/datacloud/clickhouse/v1"
	"a.yandex-team.ru/cloud/mdb/datacloud/private_api/datacloud/v1"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/datacloudgrpc"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/clickhouse"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/clickhouse/chmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/backups"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	consolemodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/console"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/environment"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
)

func CreateClusterArgsFromGRPC(req *chv1.CreateClusterRequest) (clickhouse.CreateDataCloudClusterArgs, error) {
	cloudType, err := environment.ParseCloudType(req.GetCloudType())
	if err != nil {
		return clickhouse.CreateDataCloudClusterArgs{}, err
	}

	if req.GetResources() == nil {
		return clickhouse.CreateDataCloudClusterArgs{}, semerr.InvalidInput("clickhouse resources must be set")
	}

	resources := req.GetResources()
	if req.GetResources().Clickhouse.ReplicaCount == nil {
		return clickhouse.CreateDataCloudClusterArgs{}, semerr.InvalidInput("clickhouse replica count must be set")
	}

	clusterSpec := chmodels.DataCloudClusterSpec{}
	clusterSpec.Version = req.GetVersion()
	clusterSpec.ClickHouseResources = datacloudgrpc.ResourcesFromGRPC(resources.Clickhouse)
	clusterSpec.Access = DataCloudAccessFromGRPC(req.Access)
	clusterSpec.Encryption = DataCloudEncryptionFromGRPC(req.Encryption)

	return clickhouse.CreateDataCloudClusterArgs{
		ProjectID:    req.GetProjectId(),
		Name:         req.GetName(),
		Description:  req.GetDescription(),
		RegionID:     req.GetRegionId(),
		CloudType:    cloudType,
		ReplicaCount: resources.Clickhouse.GetReplicaCount().GetValue(),
		ShardCount:   grpc.OptionalInt64FromGRPC(resources.Clickhouse.GetShardCount()),
		ClusterSpec:  clusterSpec,
		NetworkID:    grpc.OptionalStringFromGRPC(req.GetNetworkId()),
	}, nil
}

func restoreClusterArgsFromGRPC(req *chv1.RestoreClusterRequest) (clickhouse.CreateDataCloudClusterArgs, error) {
	args := clickhouse.CreateDataCloudClusterArgs{
		ProjectID:   req.GetProjectId(),
		Name:        req.GetName(),
		Description: req.GetDescription(),
		RegionID:    req.GetRegionId(),
		NetworkID:   grpc.OptionalStringFromGRPC(req.GetNetworkId()),
	}

	if resources := req.GetResources(); resources != nil {
		if rc := resources.GetClickhouse().GetReplicaCount(); rc != nil {
			args.ReplicaCount = rc.GetValue()
		}

		args.ClusterSpec.ClickHouseResources = datacloudgrpc.ResourcesFromGRPC(resources.Clickhouse)
	}
	args.ClusterSpec.Access = DataCloudAccessFromGRPC(req.Access)
	args.ClusterSpec.Version = req.GetVersion()
	args.ClusterSpec.Encryption = DataCloudEncryptionFromGRPC(req.Encryption)
	return args, nil
}

func ConnectionInfoToGRPC(info chmodels.ConnectionInfo) *chv1.ConnectionInfo {
	return &chv1.ConnectionInfo{
		Host:     info.Hostname,
		User:     info.Username,
		Password: info.Password.Unmask(),

		HttpsPort:     wrapperspb.Int64(info.HTTPSPort),
		TcpPortSecure: wrapperspb.Int64(info.TCPPortSecure),

		NativeProtocol: info.NativeProtocol,
		HttpsUri:       info.HTTPSURI,
		JdbcUri:        info.JDBCURI,
		OdbcUri:        info.ODBCURI,
	}
}

func ConnectionInfoToPrivateGrpc(info chmodels.ConnectionInfo, domain api.DomainConfig) *chv1.PrivateConnectionInfo {
	return &chv1.PrivateConnectionInfo{
		Host:     switchToPrivateDomain(info.Hostname, domain),
		User:     info.Username,
		Password: info.Password.Unmask(),

		HttpsPort:     wrapperspb.Int64(info.HTTPSPort),
		TcpPortSecure: wrapperspb.Int64(info.TCPPortSecure),

		NativeProtocol: switchToPrivateDomain(info.NativeProtocol, domain),
		HttpsUri:       switchToPrivateDomain(info.HTTPSURI, domain),
		JdbcUri:        switchToPrivateDomain(info.JDBCURI, domain),
		OdbcUri:        switchToPrivateDomain(info.ODBCURI, domain),
	}
}

func ClusterToGRPC(cluster chmodels.DataCloudCluster, domain api.DomainConfig) *chv1.Cluster {
	v := &chv1.Cluster{
		Id:                    cluster.ClusterID,
		ProjectId:             cluster.FolderExtID,
		CloudType:             cluster.CloudType,
		RegionId:              cluster.RegionID,
		CreateTime:            grpc.TimeToGRPC(cluster.CreatedAt),
		Name:                  cluster.Name,
		Description:           cluster.Description,
		Status:                datacloudgrpc.StatusToGRPC(cluster.Status, cluster.Health),
		ConnectionInfo:        ConnectionInfoToGRPC(cluster.ConnectionInfo),
		PrivateConnectionInfo: ConnectionInfoToPrivateGrpc(cluster.ConnectionInfo, domain),
		Version:               cluster.Version,
		Resources: &chv1.ClusterResources{
			Clickhouse: &chv1.ClusterResources_Clickhouse{
				ResourcePresetId: cluster.Resources.ResourcePresetID.String,
				DiskSize:         grpc.OptionalInt64ToGRPC(cluster.Resources.DiskSize),
				ReplicaCount:     grpc.OptionalInt64ToGRPC(cluster.Resources.ReplicaCount),
				ShardCount:       grpc.OptionalInt64ToGRPC(cluster.Resources.ShardCount),
			},
		},
		Access:     DataCloudAccessToGRPC(cluster.Access),
		Encryption: DataCloudEncryptionToGRPC(cluster.Encryption),
		NetworkId:  cluster.NetworkID,
	}
	return v
}

func ClustersToGRPC(clusters []chmodels.DataCloudCluster, domain api.DomainConfig) []*chv1.Cluster {
	var v []*chv1.Cluster
	for _, cluster := range clusters {
		v = append(v, ClusterToGRPC(cluster, domain))
	}
	return v
}

func HostToGRPC(host hosts.HostExtended) *chv1.Host {
	h := &chv1.Host{
		Name:      host.FQDN,
		ClusterId: host.ClusterID,
		ShardName: host.ShardID.String,
	}
	return h
}

func HostsToGRPC(hosts []hosts.HostExtended) []*chv1.Host {
	var v []*chv1.Host
	for _, host := range hosts {
		v = append(v, HostToGRPC(host))
	}
	return v
}

func VersionToGRPC(version logic.Version) *chv1.Version {
	return &chv1.Version{Id: chmodels.CutVersionToMajor(version.ID), Name: version.Name, Deprecated: version.Deprecated,
		UpdatableTo: chmodels.CutVersionsToMajors(version.UpdatableTo)}
}

func VersionsToGRPC(versions []logic.Version) []*chv1.Version {
	v := make([]*chv1.Version, 0, len(versions))
	for _, version := range versions {
		v = append(v, VersionToGRPC(version))
	}
	return v
}

func CloudsToGRPC(clouds []consolemodels.Cloud) []*chconsolev1.Cloud {
	res := make([]*chconsolev1.Cloud, 0, len(clouds))
	for _, cloud := range clouds {
		res = append(res, &chconsolev1.Cloud{
			CloudType: string(cloud.Cloud),
			RegionIds: cloud.Regions,
		})
	}

	return res
}

func UpdateClusterArgsFromGRPC(req *chv1.UpdateClusterRequest) clickhouse.UpdateDataCloudClusterArgs {
	args := clickhouse.UpdateDataCloudClusterArgs{
		ClusterID:   req.GetClusterId(),
		Name:        grpc.OptionalStringFromGRPC(req.GetName()),
		Description: grpc.OptionalStringFromGRPC(req.GetDescription()),
		ConfigSpec: chmodels.DataCloudConfigSpecUpdate{
			Version: grpc.OptionalStringFromGRPC(chmodels.CutVersionToMajor(req.GetVersion())),
		},
	}

	if res := req.GetResources(); res != nil {
		args.ConfigSpec.Resources = chmodels.DataCloudResources{
			ResourcePresetID: grpc.OptionalStringFromGRPC(res.Clickhouse.GetResourcePresetId()),
			DiskSize:         grpc.OptionalInt64FromGRPC(res.Clickhouse.GetDiskSize()),
			ReplicaCount:     grpc.OptionalInt64FromGRPC(res.Clickhouse.GetReplicaCount()),
			ShardCount:       grpc.OptionalInt64FromGRPC(res.Clickhouse.GetShardCount()),
		}
	}

	if acc := req.GetAccess(); acc != nil {
		args.ConfigSpec.Access = DataCloudAccessFromGRPC(acc)
	}
	return args
}

func BackupToGRPC(backup backups.Backup) *chv1.Backup {
	meta := backup.Metadata.(chmodels.BackupLabels)
	name := meta.Name
	backupType := chv1.Backup_TYPE_MANUAL
	if name == "" {
		backupType = chv1.Backup_TYPE_AUTOMATED
		name = fmt.Sprintf("%s:%s", backup.SourceClusterID, backup.ID)
	}
	name = fmt.Sprintf("%s:%s", name, meta.ShardName)

	return &chv1.Backup{
		Id:        backups.EncodeGlobalBackupID(backup.SourceClusterID, backup.ID),
		ProjectId: backup.FolderID,
		Name:      name,
		CreateTime: &timestamp.Timestamp{
			Seconds: backup.CreatedAt.Unix(),
			Nanos:   int32(backup.CreatedAt.Nanosecond()),
		},
		StartTime:       grpc.OptionalTimeToGRPC(backup.StartedAt),
		SourceClusterId: backup.SourceClusterID,
		Size:            backup.Size,
		Type:            backupType,
	}
}

func BackupsToGRPC(backups []backups.Backup) []*chv1.Backup {
	res := make([]*chv1.Backup, 0, len(backups))
	for _, b := range backups {
		res = append(res, BackupToGRPC(b))
	}

	return res
}

func switchToPrivateDomain(origin string, domain api.DomainConfig) string {
	return strings.Replace(origin, domain.Public, domain.Private, -1)
}

func DataCloudAccessToGRPC(access clusters.Access) *datacloud.Access {
	grpcDataCloudAccess := datacloud.Access{}
	settingsExists := false

	if access.Ipv4CidrBlocks != nil {
		var cidrBlocks []*datacloud.Access_CidrBlock
		for _, block := range access.Ipv4CidrBlocks {
			cidrBlocks = append(cidrBlocks, &datacloud.Access_CidrBlock{Value: block.Value, Description: block.Description})
		}
		grpcDataCloudAccess.Ipv4CidrBlocks = &datacloud.Access_CidrBlockList{Values: cidrBlocks}
		settingsExists = true
	}

	if access.Ipv6CidrBlocks != nil {
		var cidrBlocks []*datacloud.Access_CidrBlock
		for _, block := range access.Ipv6CidrBlocks {
			cidrBlocks = append(cidrBlocks, &datacloud.Access_CidrBlock{Value: block.Value, Description: block.Description})
		}
		grpcDataCloudAccess.Ipv6CidrBlocks = &datacloud.Access_CidrBlockList{Values: cidrBlocks}
		settingsExists = true
	}

	var dataServices []datacloud.Access_DataService
	if b, err := access.DataLens.Get(); err == nil && b {
		dataServices = append(dataServices, datacloud.Access_DATA_SERVICE_VISUALIZATION)
		settingsExists = true
	}
	if b, err := access.DataTransfer.Get(); err == nil && b {
		dataServices = append(dataServices, datacloud.Access_DATA_SERVICE_TRANSFER)
		settingsExists = true
	}
	grpcDataCloudAccess.DataServices = &datacloud.Access_DataServiceList{Values: dataServices}

	if !settingsExists {
		return nil
	}
	return &grpcDataCloudAccess
}

func DataCloudAccessFromGRPC(reqAccess *datacloud.Access) clusters.Access {
	access := clusters.Access{}
	if reqAccess == nil {
		return access
	}
	if reqAccess.Ipv4CidrBlocks != nil {
		var blocks []clusters.CidrBlock
		if reqAccess.Ipv4CidrBlocks.GetValues() != nil {
			for _, cidrBlock := range reqAccess.Ipv4CidrBlocks.Values {
				blocks = append(blocks, clusters.CidrBlock{Value: cidrBlock.Value, Description: cidrBlock.Description})
			}
		}
		access.Ipv4CidrBlocks = blocks
	}

	if reqAccess.Ipv6CidrBlocks != nil {
		var blocks []clusters.CidrBlock
		if reqAccess.Ipv6CidrBlocks.GetValues() != nil {
			for _, cidrBlock := range reqAccess.Ipv6CidrBlocks.Values {
				blocks = append(blocks, clusters.CidrBlock{Value: cidrBlock.Value, Description: cidrBlock.Description})
			}
		}
		access.Ipv6CidrBlocks = blocks
	}
	if reqAccess.DataServices != nil {
		access.DataLens = optional.NewBool(false)
		access.DataTransfer = optional.NewBool(false)
		for _, ds := range reqAccess.DataServices.GetValues() {
			switch ds {
			case datacloud.Access_DATA_SERVICE_VISUALIZATION:
				access.DataLens = optional.NewBool(true)
			case datacloud.Access_DATA_SERVICE_TRANSFER:
				access.DataTransfer = optional.NewBool(true)
			}
		}
	}
	return access
}

func DataCloudEncryptionFromGRPC(reqEncryption *datacloud.DataEncryption) clusters.Encryption {
	encryption := clusters.Encryption{}
	if reqEncryption == nil {
		return encryption
	}

	encryption.Enabled = optional.NewBool(reqEncryption.GetEnabled().GetValue())
	return encryption
}

func DataCloudEncryptionToGRPC(encryption clusters.Encryption) *datacloud.DataEncryption {
	grpcDataCloudEncryption := datacloud.DataEncryption{
		Enabled: grpc.OptionalBoolToGRPC(encryption.Enabled),
	}

	if encryption.Enabled.Valid {
		return &grpcDataCloudEncryption
	}

	return nil
}
