package provider

import (
	"context"
	"reflect"
	"sort"
	"strconv"

	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/crypto"
	clusterslogic "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/sessions"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/kafka"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/kafka/kfmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/kafka/provider/internal/kfpillars"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts/services"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/tasks"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (kf *Kafka) ClusterConnectionToPillar(spec kfmodels.ClusterConnectionSpec) (kfpillars.ClusterConnection, error) {
	if spec.Type == kfmodels.ClusterConnectionTypeExternal {
		passwordEncrypted, err := kf.cryptoProvider.Encrypt([]byte(spec.SaslPassword.Unmask()))
		if err != nil {
			return kfpillars.ClusterConnection{}, xerrors.Errorf("failed to encrypt connector password: %w", err)
		}
		return kfpillars.ClusterConnection{
			Alias:                     spec.Alias,
			Type:                      spec.Type,
			BootstrapServers:          spec.BootstrapServers,
			SaslUsername:              spec.SaslUsername,
			SaslPassword:              &passwordEncrypted,
			SaslMechanism:             spec.SaslMechanism,
			SecurityProtocol:          spec.SecurityProtocol,
			SslTruststoreCertificates: spec.SslTruststoreCertificates,
		}, nil
	}
	if spec.Type == kfmodels.ClusterConnectionTypeThisCluster {
		return kfpillars.ClusterConnection{
			Alias: spec.Alias,
			Type:  spec.Type,
		}, nil
	}
	return kfpillars.ClusterConnection{}, nil
}

func S3ConnectionToPillar(spec kfmodels.S3ConnectionSpec, cryptoProvider crypto.Crypto) (kfpillars.S3Connection, error) {
	secretAccessKeyEncrypted, err := cryptoProvider.Encrypt([]byte(spec.SecretAccessKey.Unmask()))
	if err != nil {
		return kfpillars.S3Connection{}, xerrors.Errorf("failed to encrypt S3 secretAccessKey: %w", err)
	}
	return kfpillars.S3Connection{
		Type:            spec.Type,
		AccessKeyID:     spec.AccessKeyID,
		SecretAccessKey: &secretAccessKeyEncrypted,
		BucketName:      spec.BucketName,
		Endpoint:        spec.Endpoint,
		Region:          spec.Region,
	}, nil
}

func (kf *Kafka) MirrormakerConfigSpecToPillar(spec kfmodels.MirrormakerConfigSpec) (kfpillars.MirrorMakerConfig, error) {
	sourceCluster, err := kf.ClusterConnectionToPillar(spec.SourceCluster)
	if err != nil {
		return kfpillars.MirrorMakerConfig{}, err
	}
	targetCluster, err := kf.ClusterConnectionToPillar(spec.TargetCluster)
	if err != nil {
		return kfpillars.MirrorMakerConfig{}, err
	}
	mirrormakerConfig := kfpillars.MirrorMakerConfig{
		SourceCluster:     sourceCluster,
		TargetCluster:     targetCluster,
		Topics:            spec.Topics,
		ReplicationFactor: spec.ReplicationFactor,
	}
	return mirrormakerConfig, nil
}

func S3SinkConnectorConfigSpecToPillar(spec kfmodels.S3SinkConnectorConfigSpec, cryptoProvider crypto.Crypto) (kfpillars.S3SinkConnectorConfig, error) {
	s3Connection, err := S3ConnectionToPillar(spec.S3Connection, cryptoProvider)
	if err != nil {
		return kfpillars.S3SinkConnectorConfig{}, err
	}
	return kfpillars.S3SinkConnectorConfig{
		S3Connection:        s3Connection,
		Topics:              spec.Topics,
		FileCompressionType: spec.FileCompressionType,
		FileMaxRecords:      spec.FileMaxRecords,
	}, nil
}

func (kf *Kafka) ConnectorSpecToPillar(spec kfmodels.ConnectorSpec) (kfpillars.ConnectorData, error) {
	connectorData := kfpillars.ConnectorData{
		Type:       spec.Type,
		Name:       spec.Name,
		TasksMax:   spec.TasksMax,
		Properties: spec.Properties,
	}
	if spec.Type == kfmodels.ConnectorTypeMirrormaker {
		mirrormakerConfig, err := kf.MirrormakerConfigSpecToPillar(spec.MirrormakerConfig)
		if err != nil {
			return kfpillars.ConnectorData{}, err
		}
		connectorData.MirrorMakerConfig = mirrormakerConfig
	} else if spec.Type == kfmodels.ConnectorTypeS3Sink {
		s3SinkConnectorConfig, err := S3SinkConnectorConfigSpecToPillar(spec.S3SinkConnectorConfig, kf.cryptoProvider)
		if err != nil {
			return kfpillars.ConnectorData{}, err
		}
		connectorData.S3SinkConnectorConfig = s3SinkConnectorConfig
	} else {
		return kfpillars.ConnectorData{}, semerr.InvalidInput("unknown connector type")
	}
	return connectorData, nil
}

func UpdateConnectorData(data kfpillars.ConnectorData, upd kfmodels.UpdateConnectorSpec, cryptoProvider crypto.Crypto) (kfpillars.ConnectorData, bool, error) {
	hasChanges := false
	if upd.TasksMax.Valid && upd.TasksMax.Int64 != data.TasksMax {
		data.TasksMax = upd.TasksMax.Int64
		hasChanges = true
	}
	if upd.Type.Valid {
		if upd.Type.String != data.Type {
			return kfpillars.ConnectorData{}, false, semerr.InvalidInput("connector type cannot be changed")
		} else if upd.Type.String == kfmodels.ConnectorTypeMirrormaker {
			cfg, changes, err := UpdateMirrormakerConfig(data.MirrorMakerConfig, upd.MirrormakerConfig, cryptoProvider)
			if err != nil {
				return kfpillars.ConnectorData{}, false, err
			}
			if changes {
				hasChanges = true
			}
			data.MirrorMakerConfig = cfg
		} else if upd.Type.String == kfmodels.ConnectorTypeS3Sink {
			cfg, changes, err := UpdateS3SinkConnectorConfig(data.S3SinkConnectorConfig, upd.S3SinkConnectorConfig, cryptoProvider)
			if err != nil {
				return kfpillars.ConnectorData{}, false, err
			}
			if changes {
				hasChanges = true
			}
			data.S3SinkConnectorConfig = cfg
		}
	}
	if upd.PropertiesOverwrite && !reflect.DeepEqual(data.Properties, upd.Properties) {
		data.Properties = upd.Properties
		hasChanges = true
	}
	for key, updPropertiesContains := range upd.PropertiesContains {
		if !updPropertiesContains {
			if _, ok := data.Properties[key]; ok {
				delete(data.Properties, key)
				hasChanges = true
			}
		} else {
			if dataVal, ok := data.Properties[key]; ok {
				if dataVal != upd.Properties[key] {
					data.Properties[key] = upd.Properties[key]
					hasChanges = true
				}
			} else {
				data.Properties[key] = upd.Properties[key]
				hasChanges = true
			}
		}
	}
	return data, hasChanges, nil
}

func UpdateMirrormakerConfig(cfg kfpillars.MirrorMakerConfig, upd kfmodels.UpdateMirrormakerConfigSpec, cryptoProvider crypto.Crypto) (kfpillars.MirrorMakerConfig, bool, error) {
	hasChanges := false
	if upd.Topics.Valid && cfg.Topics != upd.Topics.String {
		cfg.Topics = upd.Topics.String
		hasChanges = true
	}
	if upd.ReplicationFactor.Valid && cfg.ReplicationFactor != upd.ReplicationFactor.Int64 {
		cfg.ReplicationFactor = upd.ReplicationFactor.Int64
		hasChanges = true
	}
	cc, changes, err := UpdateClusterConnection(cfg.SourceCluster, upd.SourceCluster, cryptoProvider)
	if err != nil {
		return kfpillars.MirrorMakerConfig{}, false, err
	}
	if changes {
		hasChanges = true
	}
	cfg.SourceCluster = cc
	cc, changes, err = UpdateClusterConnection(cfg.TargetCluster, upd.TargetCluster, cryptoProvider)
	if err != nil {
		return kfpillars.MirrorMakerConfig{}, false, err
	}
	if changes {
		hasChanges = true
	}
	cfg.TargetCluster = cc
	return cfg, hasChanges, nil
}

func UpdateS3SinkConnectorConfig(cfg kfpillars.S3SinkConnectorConfig, upd kfmodels.UpdateS3SinkConnectorConfigSpec, cryptoProvider crypto.Crypto) (kfpillars.S3SinkConnectorConfig, bool, error) {
	hasChanges := false
	if upd.Topics.Valid && cfg.Topics != upd.Topics.String {
		cfg.Topics = upd.Topics.String
		hasChanges = true
	}
	s3, changes, err := UpdateS3Connection(cfg.S3Connection, upd.S3Connection, cryptoProvider)
	if err != nil {
		return kfpillars.S3SinkConnectorConfig{}, false, err
	}
	if changes {
		hasChanges = true
	}
	cfg.S3Connection = s3
	if upd.FileMaxRecords.Valid && cfg.FileMaxRecords != upd.FileMaxRecords.Int64 {
		cfg.FileMaxRecords = upd.FileMaxRecords.Int64
		hasChanges = true
	}
	return cfg, hasChanges, nil
}

func UpdateClusterConnection(cc kfpillars.ClusterConnection, upd kfmodels.UpdateClusterConnectionSpec, cryptoProvider crypto.Crypto) (kfpillars.ClusterConnection, bool, error) {
	hasChanges := false
	if upd.Alias.Valid && cc.Alias != upd.Alias.String {
		cc.Alias = upd.Alias.String
		hasChanges = true
	}
	if upd.Type.Valid && cc.Type != upd.Type.String {
		cc.Type = upd.Type.String
		hasChanges = true
		if cc.Type == kfmodels.ClusterConnectionTypeThisCluster {
			cc.BootstrapServers = ""
			cc.SaslMechanism = ""
			cc.SecurityProtocol = ""
			cc.SaslUsername = ""
			cc.SaslPassword = nil
		}
	}
	if cc.Type == kfmodels.ClusterConnectionTypeExternal {
		if upd.BootstrapServers.Valid && cc.BootstrapServers != upd.BootstrapServers.String {
			cc.BootstrapServers = upd.BootstrapServers.String
			hasChanges = true
		}
		if upd.SecurityProtocol.Valid && cc.SecurityProtocol != upd.SecurityProtocol.String {
			cc.SecurityProtocol = upd.SecurityProtocol.String
			hasChanges = true
			if cc.SecurityProtocol == kfmodels.SecurityProtocolPlaintext {
				cc.SaslMechanism = ""
				cc.SaslUsername = ""
				cc.SaslPassword = nil
			} else if cc.SecurityProtocol == kfmodels.SecurityProtocolSaslPlaintext {
				cc.SaslMechanism = ""
			}
		}
		if upd.SaslMechanism.Valid && cc.SaslMechanism != upd.SaslMechanism.String {
			cc.SaslMechanism = upd.SaslMechanism.String
			hasChanges = true
		}
		if upd.SaslUsername.Valid && cc.SaslUsername != upd.SaslUsername.String {
			cc.SaslUsername = upd.SaslUsername.String
			hasChanges = true
		}
		if upd.SaslPasswordUpdated {
			passwordEncrypted, err := cryptoProvider.Encrypt([]byte(upd.SaslPassword.Unmask()))
			if err != nil {
				return kfpillars.ClusterConnection{}, false, xerrors.Errorf("failed to encrypt SaslPassword of connector: %w", err)
			}
			cc.SaslPassword = &passwordEncrypted
			hasChanges = true
		}
		if upd.SslTruststoreCertificates.Valid && cc.SslTruststoreCertificates != upd.SslTruststoreCertificates.String {
			cc.SslTruststoreCertificates = upd.SslTruststoreCertificates.String
			hasChanges = true
		}
	}
	return cc, hasChanges, nil
}

func UpdateS3Connection(s3 kfpillars.S3Connection, upd kfmodels.UpdateS3ConnectionSpec, cryptoProvider crypto.Crypto) (kfpillars.S3Connection, bool, error) {
	hasChanges := false
	if upd.Type.Valid && s3.Type != upd.Type.String {
		s3.Type = upd.Type.String
		hasChanges = true
	}
	if upd.AccessKeyID.Valid && s3.AccessKeyID != upd.AccessKeyID.String {
		s3.AccessKeyID = upd.AccessKeyID.String
		hasChanges = true
	}
	if upd.SecretAccessKeyUpdated {
		secretAccessKeyEncrypted, err := cryptoProvider.Encrypt([]byte(upd.SecretAccessKey.Unmask()))
		if err != nil {
			return kfpillars.S3Connection{}, false, xerrors.Errorf("failed to encrypt S3 secretAccessKey: %w", err)
		}
		s3.SecretAccessKey = &secretAccessKeyEncrypted
		hasChanges = true
	}
	if upd.BucketName.Valid && s3.BucketName != upd.BucketName.String {
		s3.BucketName = upd.BucketName.String
		hasChanges = true
	}
	if upd.Endpoint.Valid && s3.Endpoint != upd.Endpoint.String {
		s3.Endpoint = upd.Endpoint.String
		hasChanges = true
	}
	if upd.Region.Valid && s3.Region != upd.Region.String {
		s3.Region = upd.Region.String
		hasChanges = true
	}
	return s3, hasChanges, nil
}

func (kf *Kafka) CreateConnector(ctx context.Context, cid string, spec kfmodels.ConnectorSpec) (operations.Operation, error) {
	if err := spec.Validate(); err != nil {
		return operations.Operation{}, err
	}
	return kf.operator.CreateOnCluster(ctx, cid, clusters.TypeKafka,
		func(ctx context.Context, session sessions.Session, _ clusterslogic.Reader, modifier clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {
			pillar := kfpillars.NewCluster()
			if err := cluster.Pillar(pillar); err != nil {
				return operations.Operation{}, err
			}

			enableKafkaConnect := false
			if !pillar.Data.Kafka.ConnectEnabled {
				pillar.Data.Kafka.ConnectEnabled = true
				enableKafkaConnect = true
			}

			connectorData, err := kf.ConnectorSpecToPillar(spec)
			if err != nil {
				return operations.Operation{}, err
			}
			if err := pillar.AddConnector(connectorData); err != nil {
				return operations.Operation{}, err
			}

			if err := modifier.UpdatePillar(ctx, cluster.ClusterID, cluster.Revision, pillar); err != nil {
				return operations.Operation{}, err
			}

			args := make(map[string]interface{})
			args["target-connector"] = spec.Name
			args["feature_flags"] = session.FeatureFlags
			if enableKafkaConnect {
				args["enable-connect"] = true
			}

			op, err := kf.tasks.CreateTask(
				ctx,
				session,
				tasks.CreateTaskArgs{
					ClusterID:     cid,
					FolderID:      session.FolderCoords.FolderID,
					Auth:          session.Subject,
					TaskType:      kfmodels.TaskTypeConnectorCreate,
					TaskArgs:      args,
					OperationType: kfmodels.OperationTypeConnectorCreate,
					Metadata:      kfmodels.MetadataCreateConnector{ConnectorName: spec.Name},
					Revision:      cluster.Revision,
				},
			)
			if err != nil {
				return operations.Operation{}, err
			}

			if err := kf.toSearchQueue(ctx, session.FolderCoords, op); err != nil {
				return operations.Operation{}, err
			}

			return op, nil
		},
		clusterslogic.WithPermission(kfmodels.PermClustersUpdate),
	)
}

func (kf *Kafka) UpdateConnector(ctx context.Context, args kafka.UpdateConnectorArgs) (operations.Operation, error) {
	return kf.operator.ModifyOnCluster(ctx, args.ClusterID, clusters.TypeKafka,
		func(ctx context.Context, session sessions.Session, _ clusterslogic.Reader, modifier clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {
			if args.Name == "" {
				return operations.Operation{}, semerr.InvalidInput("connector name must be specified")
			}
			pillar := kfpillars.NewCluster()
			if err := cluster.Pillar(pillar); err != nil {
				return operations.Operation{}, err
			}
			connector, ok := pillar.GetConnector(args.Name)
			if !ok {
				return operations.Operation{}, semerr.NotFoundf("connector %q not found", args.Name)
			}
			updatedConnector, changes, err := UpdateConnectorData(connector, args.ConnectorSpec, kf.cryptoProvider)
			if err != nil {
				return operations.Operation{}, err
			}
			if !changes {
				return operations.Operation{}, semerr.InvalidInput("no fields to update")
			}
			if err := pillar.SetConnector(updatedConnector); err != nil {
				return operations.Operation{}, err
			}
			if err := modifier.UpdatePillar(ctx, cluster.ClusterID, cluster.Revision, pillar); err != nil {
				return operations.Operation{}, err
			}
			taskArgs := make(map[string]interface{})
			taskArgs["target-connector"] = args.Name
			taskArgs["feature_flags"] = session.FeatureFlags
			op, err := kf.tasks.CreateTask(
				ctx,
				session,
				tasks.CreateTaskArgs{
					ClusterID:     args.ClusterID,
					FolderID:      session.FolderCoords.FolderID,
					Auth:          session.Subject,
					TaskType:      kfmodels.TaskTypeConnectorUpdate,
					TaskArgs:      taskArgs,
					OperationType: kfmodels.OperationTypeConnectorUpdate,
					Metadata:      kfmodels.MetadataUpdateConnector{ConnectorName: args.Name},
					Revision:      cluster.Revision,
				},
			)
			if err != nil {
				return operations.Operation{}, err
			}
			if err := kf.toSearchQueue(ctx, session.FolderCoords, op); err != nil {
				return operations.Operation{}, err
			}
			return op, nil
		},
		clusterslogic.WithPermission(kfmodels.PermClustersUpdate),
	)
}

func (kf *Kafka) Connector(ctx context.Context, cid string, name string) (kfmodels.Connector, error) {
	if name == "" {
		return kfmodels.Connector{}, semerr.InvalidInput("connector name must be specified")
	}
	var res kfmodels.Connector
	connectorsStatusMetric, err := kf.getConnectorsStatusMetric(ctx, cid)
	if err != nil {
		return res, err
	}
	connectorsStatus := connectorsMetricToStatus(connectorsStatusMetric)
	if err := kf.operator.ReadOnCluster(ctx, cid, clusters.TypeKafka,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, cl clusterslogic.Cluster) error {
			var err error

			pillar := kfpillars.NewCluster()
			if err = cl.Pillar(pillar); err != nil {
				return err
			}

			connector, ok := pillar.Data.Kafka.Connectors[name]
			if !ok {
				return semerr.NotFoundf("connector %q not found", name)
			}

			res = connectorFromPillar(connector)
			status, ok := connectorsStatus[name]
			if !ok {
				status = kfmodels.ConnectorStatusUnspecified
			}
			res.Status = status
			res.Health = connectorHealthFromStatus(status)

			return nil
		},
		clusterslogic.WithPermission(kfmodels.PermClustersGet),
	); err != nil {
		return kfmodels.Connector{}, err
	}

	return res, nil
}

func (kf *Kafka) getConnectorsStatusMetric(ctx context.Context, cid string) (map[string]string, error) {
	hosts, _, _, err := clusterslogic.ListHosts(ctx, kf.operator, cid, clusters.TypeKafka, 1024, 0)
	if err != nil {
		return nil, err
	}
	for _, host := range hosts {
		for _, srv := range host.Health.Services {
			if srv.Type == services.TypeKafkaConnect {
				if len(srv.Metrics) > 0 {
					return srv.Metrics, nil
				}
			}
		}
	}
	return nil, nil
}

func metricsStatusToConnectorStatus(metricsStatus int) kfmodels.ConnectorStatus {
	mapMetricStatusToConnectorStatus := map[int]kfmodels.ConnectorStatus{
		0: kfmodels.ConnectorStatusUnspecified,
		1: kfmodels.ConnectorStatusRunning,
		2: kfmodels.ConnectorStatusPaused,
		3: kfmodels.ConnectorStatusError,
	}
	v, ok := mapMetricStatusToConnectorStatus[metricsStatus]
	if !ok {
		return kfmodels.ConnectorStatusUnspecified
	}
	return v
}

func connectorsMetricToStatus(metrics map[string]string) map[string]kfmodels.ConnectorStatus {
	res := make(map[string]kfmodels.ConnectorStatus, len(metrics))
	for name, statusStr := range metrics {
		statusInt, err := strconv.Atoi(statusStr)
		if err != nil {
			statusInt = 0
		}
		res[name] = metricsStatusToConnectorStatus(statusInt)
	}
	return res
}

func connectorHealthFromStatus(status kfmodels.ConnectorStatus) kfmodels.ConnectorHealth {
	if status == kfmodels.ConnectorStatusRunning {
		return kfmodels.ConnectorHealthAlive
	}
	if status == kfmodels.ConnectorStatusError {
		return kfmodels.ConnectorHealthDead
	}
	return kfmodels.ConnectorHealthUnknown
}

func (kf *Kafka) Connectors(ctx context.Context, cid string, limit, offset int64) ([]kfmodels.Connector, error) {
	var res []kfmodels.Connector

	connectorsStatusMetric, err := kf.getConnectorsStatusMetric(ctx, cid)
	if err != nil {
		return res, err
	}
	connectorsStatus := connectorsMetricToStatus(connectorsStatusMetric)

	if err := kf.operator.ReadOnCluster(ctx, cid, clusters.TypeKafka,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, cl clusterslogic.Cluster) error {
			var err error

			pillar := kfpillars.NewCluster()
			if err = cl.Pillar(pillar); err != nil {
				return err
			}

			// Limit and offset filtering in memory. Sorting by connector name
			connectorNames := make([]string, 0, len(pillar.Data.Kafka.Connectors))
			for name := range pillar.Data.Kafka.Connectors {
				connectorNames = append(connectorNames, name)
			}
			sort.Strings(connectorNames)
			if offset > 0 {
				if offset > int64(len(connectorNames)) {
					return nil
				}
				connectorNames = connectorNames[offset:]
			}
			if limit < 1 {
				limit = 100
			}
			if limit < int64(len(connectorNames)) {
				connectorNames = connectorNames[:limit]
			}

			// Construct response
			res = make([]kfmodels.Connector, 0, len(connectorNames))
			for _, name := range connectorNames {
				conn := connectorFromPillar(pillar.Data.Kafka.Connectors[name])
				status, ok := connectorsStatus[name]
				if !ok {
					status = kfmodels.ConnectorStatusUnspecified
				}
				conn.Status = status
				conn.Health = connectorHealthFromStatus(status)
				res = append(res, conn)
			}
			return nil
		},
		clusterslogic.WithPermission(kfmodels.PermClustersGet),
	); err != nil {
		return nil, err
	}

	return res, nil
}

func (kf *Kafka) DeleteConnector(ctx context.Context, cid string, name string) (operations.Operation, error) {
	if name == "" {
		return operations.Operation{}, semerr.InvalidInput("connector name must be specified")
	}
	return kf.operator.DeleteOnCluster(ctx, cid, clusters.TypeKafka,
		func(ctx context.Context, session sessions.Session, _ clusterslogic.Reader, modifier clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {
			pillar := kfpillars.NewCluster()
			if err := cluster.Pillar(pillar); err != nil {
				return operations.Operation{}, err
			}

			if err := pillar.DeleteConnector(name); err != nil {
				return operations.Operation{}, err
			}

			if err := modifier.UpdatePillar(ctx, cluster.ClusterID, cluster.Revision, pillar); err != nil {
				return operations.Operation{}, err
			}

			args := make(map[string]interface{})
			args["target-connector"] = name
			args["feature_flags"] = session.FeatureFlags

			op, err := kf.tasks.CreateTask(
				ctx,
				session,
				tasks.CreateTaskArgs{
					ClusterID:     cid,
					FolderID:      session.FolderCoords.FolderID,
					Auth:          session.Subject,
					TaskType:      kfmodels.TaskTypeConnectorDelete,
					TaskArgs:      args,
					OperationType: kfmodels.OperationTypeConnectorDelete,
					Metadata:      kfmodels.MetadataDeleteConnector{ConnectorName: name},
					Revision:      cluster.Revision,
				},
			)
			if err != nil {
				return operations.Operation{}, err
			}

			if err := kf.toSearchQueue(ctx, session.FolderCoords, op); err != nil {
				return operations.Operation{}, err
			}

			return op, nil
		},
		clusterslogic.WithPermission(kfmodels.PermClustersUpdate),
	)
}

func (kf *Kafka) PauseConnector(ctx context.Context, cid string, name string) (operations.Operation, error) {
	if name == "" {
		return operations.Operation{}, semerr.InvalidInput("connector name must be specified")
	}
	return kf.operator.ModifyOnCluster(ctx, cid, clusters.TypeKafka,
		func(ctx context.Context, session sessions.Session, _ clusterslogic.Reader, modifier clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {
			pillar := kfpillars.NewCluster()
			if err := cluster.Pillar(pillar); err != nil {
				return operations.Operation{}, err
			}

			if err := pillar.HasConnector(name); err != nil {
				return operations.Operation{}, err
			}

			args := make(map[string]interface{})
			args["target-connector"] = name
			args["feature_flags"] = session.FeatureFlags

			op, err := kf.tasks.CreateTask(
				ctx,
				session,
				tasks.CreateTaskArgs{
					ClusterID:     cid,
					FolderID:      session.FolderCoords.FolderID,
					Auth:          session.Subject,
					TaskType:      kfmodels.TaskTypeConnectorPause,
					TaskArgs:      args,
					OperationType: kfmodels.OperationTypeConnectorPause,
					Metadata:      kfmodels.MetadataPauseConnector{ConnectorName: name},
					Revision:      cluster.Revision,
				},
			)
			if err != nil {
				return operations.Operation{}, err
			}

			if err := kf.toSearchQueue(ctx, session.FolderCoords, op); err != nil {
				return operations.Operation{}, err
			}

			return op, nil
		},
		clusterslogic.WithPermission(kfmodels.PermClustersUpdate),
	)
}

func (kf *Kafka) ResumeConnector(ctx context.Context, cid string, name string) (operations.Operation, error) {
	if name == "" {
		return operations.Operation{}, semerr.InvalidInput("connector name must be specified")
	}
	return kf.operator.ModifyOnCluster(ctx, cid, clusters.TypeKafka,
		func(ctx context.Context, session sessions.Session, _ clusterslogic.Reader, modifier clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {
			pillar := kfpillars.NewCluster()
			if err := cluster.Pillar(pillar); err != nil {
				return operations.Operation{}, err
			}

			if err := pillar.HasConnector(name); err != nil {
				return operations.Operation{}, err
			}

			args := make(map[string]interface{})
			args["target-connector"] = name
			args["feature_flags"] = session.FeatureFlags

			op, err := kf.tasks.CreateTask(
				ctx,
				session,
				tasks.CreateTaskArgs{
					ClusterID:     cid,
					FolderID:      session.FolderCoords.FolderID,
					Auth:          session.Subject,
					TaskType:      kfmodels.TaskTypeConnectorResume,
					TaskArgs:      args,
					OperationType: kfmodels.OperationTypeConnectorResume,
					Metadata:      kfmodels.MetadataResumeConnector{ConnectorName: name},
					Revision:      cluster.Revision,
				},
			)
			if err != nil {
				return operations.Operation{}, err
			}

			if err := kf.toSearchQueue(ctx, session.FolderCoords, op); err != nil {
				return operations.Operation{}, err
			}

			return op, nil
		},
		clusterslogic.WithPermission(kfmodels.PermClustersUpdate),
	)
}

func connectorFromPillar(pillarConnector kfpillars.ConnectorData) kfmodels.Connector {
	res := kfmodels.Connector{
		Name:       pillarConnector.Name,
		TasksMax:   pillarConnector.TasksMax,
		Properties: pillarConnector.Properties,
		Type:       pillarConnector.Type,
	}
	if pillarConnector.Type == kfmodels.ConnectorTypeMirrormaker {
		res.MirrormakerConfig = mirrormakerConfigFromPillar(pillarConnector.MirrorMakerConfig)
	} else if pillarConnector.Type == kfmodels.ConnectorTypeS3Sink {
		res.S3SinkConnectorConfig = s3SinkConnectorConfigFromPillar(pillarConnector.S3SinkConnectorConfig)
	}
	return res
}

func mirrormakerConfigFromPillar(mirrormakerConfig kfpillars.MirrorMakerConfig) kfmodels.MirrormakerConfig {
	return kfmodels.MirrormakerConfig{
		SourceCluster:     clusterConnectionFromPillar(mirrormakerConfig.SourceCluster),
		TargetCluster:     clusterConnectionFromPillar(mirrormakerConfig.TargetCluster),
		Topics:            mirrormakerConfig.Topics,
		ReplicationFactor: mirrormakerConfig.ReplicationFactor,
	}
}

func s3SinkConnectorConfigFromPillar(s3SinkConnectorConfig kfpillars.S3SinkConnectorConfig) kfmodels.S3SinkConnectorConfig {
	return kfmodels.S3SinkConnectorConfig{
		S3Connection:        s3ConnectionFromPillar(s3SinkConnectorConfig.S3Connection),
		Topics:              s3SinkConnectorConfig.Topics,
		FileCompressionType: s3SinkConnectorConfig.FileCompressionType,
		FileMaxRecords:      s3SinkConnectorConfig.FileMaxRecords,
	}
}

func clusterConnectionFromPillar(clusterConnection kfpillars.ClusterConnection) kfmodels.ClusterConnection {
	if clusterConnection.Type == kfmodels.ClusterConnectionTypeThisCluster {
		return kfmodels.ClusterConnection{
			Alias: clusterConnection.Alias,
			Type:  clusterConnection.Type,
		}
	}
	if clusterConnection.Type == kfmodels.ClusterConnectionTypeExternal {
		return kfmodels.ClusterConnection{
			Alias:            clusterConnection.Alias,
			Type:             clusterConnection.Type,
			BootstrapServers: clusterConnection.BootstrapServers,
			SaslUsername:     clusterConnection.SaslUsername,
			SaslMechanism:    clusterConnection.SaslMechanism,
			SecurityProtocol: clusterConnection.SecurityProtocol,
		}
	}
	return kfmodels.ClusterConnection{}
}

func s3ConnectionFromPillar(s3Connection kfpillars.S3Connection) kfmodels.S3Connection {
	return kfmodels.S3Connection{
		Type:        s3Connection.Type,
		AccessKeyID: s3Connection.AccessKeyID,
		BucketName:  s3Connection.BucketName,
		Endpoint:    s3Connection.Endpoint,
		Region:      s3Connection.Region,
	}
}
