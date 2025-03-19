package provider

import (
	"context"
	"encoding/json"
	"reflect"
	"sort"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/reflectutil"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	clusterslogic "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/sessions"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/kafka"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/kafka/kfmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/kafka/provider/internal/kfpillars"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	kafkamodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/kafka"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/tasks"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func topicFromPillar(pillar *kfpillars.Cluster, cid string, topicName string) (kfmodels.Topic, error) {
	pillarTopic, ok := pillar.Data.Kafka.Topics[topicName]
	if !ok {
		return kfmodels.Topic{}, semerr.NotFoundf("topic %q not found", topicName)
	}
	topic := kfmodels.Topic{
		ClusterID:         cid,
		Name:              pillarTopic.Name,
		Partitions:        pillarTopic.Partitions,
		ReplicationFactor: pillarTopic.ReplicationFactor,
	}
	reflectutil.CopyStructFieldsStrict(&pillarTopic.Config, &topic.Config)
	topic.Config.Version = pillar.Data.Kafka.Version
	return topic, nil
}

func (kf *Kafka) Topic(ctx context.Context, cid, name string) (kfmodels.Topic, error) {
	var res kfmodels.Topic
	if err := kf.operator.ReadOnCluster(ctx, cid, clusters.TypeKafka,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, cl clusterslogic.Cluster) error {
			var err error
			if name == "" {
				return semerr.InvalidInput("topic name must be specified")
			}

			var pillar kfpillars.Cluster
			if err = cl.Pillar(&pillar); err != nil {
				return err
			}

			// We consider topic list empty in unmanaged mode
			if pillar.Data.Kafka.TopicsAreManagedViaAdminAPIOnly() {
				return semerr.NotFound("in unmanaged mode you have to use AdminAPI for topics management")
			}

			// Construct response
			res, err = topicFromPillar(&pillar, cid, name)
			if err != nil {
				return err
			}
			return nil
		},
		clusterslogic.WithPermission(kfmodels.PermClustersGet),
	); err != nil {
		return kfmodels.Topic{}, err
	}

	return res, nil
}

func (kf *Kafka) Topics(ctx context.Context, cid string, pageSize int64, pageToken kafkamodels.TopicPageToken) ([]kfmodels.Topic, kafkamodels.TopicPageToken, error) {
	var res []kfmodels.Topic
	nextPageToken := kafkamodels.TopicPageToken{}
	if err := kf.operator.ReadOnCluster(ctx, cid, clusters.TypeKafka,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, cl clusterslogic.Cluster) error {
			var err error

			pillar := kfpillars.NewCluster()
			if err = cl.Pillar(pillar); err != nil {
				return err
			}

			// Return empty list of topics if unmanaged flag is on, until topic sync developed
			if pillar.Data.Kafka.TopicsAreManagedViaAdminAPIOnly() {
				return nil
			}

			// Limit and offset filtering in memory. Sorting by topic name
			topicNames := make([]string, 0, len(pillar.Data.Kafka.Topics))
			for name := range pillar.Data.Kafka.Topics {
				topicNames = append(topicNames, name)
			}
			sort.Strings(topicNames)
			res = make([]kfmodels.Topic, 0, pageSize)
			for _, name := range topicNames {
				if name <= pageToken.LastTopicName {
					continue
				}

				topic, err := topicFromPillar(pillar, cid, name)
				if err != nil {
					return err
				}
				res = append(res, topic)

				if int64(len(res)) == pageSize {
					nextPageToken = kafkamodels.TopicPageToken{
						LastTopicName: name,
						More:          true,
					}
					break
				}
			}
			return nil
		},
		clusterslogic.WithPermission(kfmodels.PermClustersGet),
	); err != nil {
		return nil, nextPageToken, err
	}

	return res, nextPageToken, nil
}

func (kf *Kafka) TopicsToSync(ctx context.Context, cid string) (kafka.TopicsToSync, error) {
	res := kafka.TopicsToSync{
		KnownTopicConfigProperties: kfpillars.KnownTopicConfigProperties,
	}
	err := kf.operator.ReadOnCluster(ctx, cid, clusters.TypeKafka,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, cluster clusterslogic.Cluster) error {
			if cluster.Status != clusters.StatusRunning {
				res.UpdateAllowed = false
				return nil
			}

			pillar := kfpillars.NewCluster()
			if err := cluster.Pillar(pillar); err != nil {
				return err
			}

			topicNames := make([]string, 0, len(pillar.Data.Kafka.Topics))
			for name := range pillar.Data.Kafka.Topics {
				topicNames = append(topicNames, name)
			}
			sort.Strings(topicNames)

			res.Topics = make([]string, 0, len(pillar.Data.Kafka.Topics))
			for _, topicName := range topicNames {
				topicData, _ := pillar.GetTopic(topicName)
				marshaled, err := json.Marshal(topicData)
				if err != nil {
					return xerrors.Errorf("marshal topic to json: %w", err)
				}

				res.Topics = append(res.Topics, string(marshaled))
			}

			res.Revision = cluster.Revision
			res.UpdateAllowed = true
			return nil
		},
		clusterslogic.WithPermission(kfmodels.PermSyncTopics),
	)
	if err != nil {
		return kafka.TopicsToSync{}, err
	}

	return res, nil
}

func (kf *Kafka) SyncTopics(ctx context.Context, cid string, revision int64, updatedTopics []string, deletedTopics []string) (bool, error) {
	updateAccepted := false
	// synthetic error that is used to rollback transaction, but is not returned to the client code
	updateRejected := xerrors.New("update rejected")
	_, err := kf.operator.ModifyOnCluster(ctx, cid, clusters.TypeKafka,
		func(ctx context.Context, session sessions.Session, _ clusterslogic.Reader, modifier clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {
			if cluster.Status != clusters.StatusRunning {
				return operations.Operation{}, updateRejected
			}

			// Compare with cluster revision decreased by one because it is already increased on ModifyOnCluster
			if revision != cluster.Revision-1 {
				return operations.Operation{}, updateRejected
			}

			pillar := kfpillars.NewCluster()
			if err := cluster.Pillar(pillar); err != nil {
				return operations.Operation{}, err
			}

			hasChanges := false
			for _, topic := range updatedTopics {
				updatedTopic := kfpillars.TopicData{}
				if err := json.Unmarshal([]byte(topic), &updatedTopic); err != nil {
					return operations.Operation{}, semerr.InvalidInputf("unmarshal json encoded topic: %s", err)
				}
				pillarTopic, exists := pillar.GetTopic(updatedTopic.Name)
				if !exists || !reflect.DeepEqual(pillarTopic, updatedTopic) {
					hasChanges = true
					pillar.SetTopic(updatedTopic)
				}
			}

			for _, topicName := range deletedTopics {
				if _, exists := pillar.GetTopic(topicName); exists {
					hasChanges = true
					if err := pillar.DeleteTopic(topicName); err != nil {
						return operations.Operation{}, err
					}
				}
			}

			if !hasChanges {
				return operations.Operation{}, semerr.AlreadyExistsf("no changes made")
			}

			// Currently we allow to create and modify topics via Kafka Admin API and do not perform any validations.
			// This allows to create topic that will make cluster pillar invalid in terms of our validation algorithm.
			// In turn this will make it impossible to sync such a topic to intapi. For the user, this will look like
			// a broken synchronization mechanism. To avoid this, we disable validation.
			pillar.SetSkipValidation(true)
			if err := modifier.UpdatePillar(ctx, cluster.ClusterID, cluster.Revision, pillar); err != nil {
				return operations.Operation{}, err
			}

			op, err := kf.tasks.CreateFinishedTask(
				ctx,
				session,
				cluster.ClusterID,
				cluster.Revision,
				kfmodels.OperationTypeTopicModify,
				nil,
				false,
			)
			if err != nil {
				return operations.Operation{}, err
			}

			updateAccepted = true
			return op, nil
		},
		clusterslogic.WithPermission(kfmodels.PermSyncTopics),
	)
	if err != nil && !xerrors.Is(err, updateRejected) {
		return false, err
	}

	return updateAccepted, nil
}

func (kf *Kafka) CreateTopic(ctx context.Context, cid string, spec kfmodels.TopicSpec) (operations.Operation, error) {
	if err := spec.Validate(); err != nil {
		return operations.Operation{}, err
	}

	return kf.operator.CreateOnCluster(ctx, cid, clusters.TypeKafka,
		func(ctx context.Context, session sessions.Session, _ clusterslogic.Reader, modifier clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {
			pillar := kfpillars.NewCluster()
			if err := cluster.Pillar(pillar); err != nil {
				return operations.Operation{}, err
			}

			if pillar.Data.Kafka.TopicsAreManagedViaAdminAPIOnly() {
				return operations.Operation{}, semerr.NotImplemented("add topic in unmanaged mode is unimplemented")
			}

			topicData := kfpillars.TopicData{
				Name:              spec.Name,
				Partitions:        spec.Partitions,
				ReplicationFactor: spec.ReplicationFactor,
			}
			reflectutil.CopyStructFieldsStrict(&spec.Config, &topicData.Config)
			if err := pillar.AddTopic(topicData); err != nil {
				return operations.Operation{}, err
			}

			if err := modifier.UpdatePillar(ctx, cluster.ClusterID, cluster.Revision, pillar); err != nil {
				return operations.Operation{}, err
			}

			args := make(map[string]interface{})
			args["target-topic"] = spec.Name
			args["feature_flags"] = session.FeatureFlags

			op, err := kf.tasks.CreateTask(
				ctx,
				session,
				tasks.CreateTaskArgs{
					ClusterID:     cid,
					FolderID:      session.FolderCoords.FolderID,
					Auth:          session.Subject,
					TaskType:      kfmodels.TaskTypeTopicCreate,
					TaskArgs:      args,
					OperationType: kfmodels.OperationTypeTopicAdd,
					Metadata:      kfmodels.MetadataCreateTopic{TopicName: spec.Name},
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

func (kf *Kafka) UpdateTopic(ctx context.Context, args kafka.UpdateTopicArgs) (operations.Operation, error) {
	return kf.operator.ModifyOnCluster(ctx, args.ClusterID, clusters.TypeKafka,
		func(ctx context.Context, session sessions.Session, _ clusterslogic.Reader, modifier clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {
			if args.Name == "" {
				return operations.Operation{}, semerr.InvalidInput("topic name must be specified")
			}

			pillar := kfpillars.NewCluster()
			if err := cluster.Pillar(pillar); err != nil {
				return operations.Operation{}, err
			}

			// We consider topic list empty in unmanaged mode
			if pillar.Data.Kafka.TopicsAreManagedViaAdminAPIOnly() {
				return operations.Operation{}, semerr.NotFound("in unmanaged mode you have to use AdminAPI for topics management")
			}

			topic, ok := pillar.GetTopic(args.Name)
			if !ok {
				return operations.Operation{}, semerr.NotFoundf("topic %q not found", args.Name)
			}

			changes := false

			if args.TopicSpec.Partitions.Valid && args.TopicSpec.Partitions.Int64 != topic.Partitions {
				changes = true
				if topic.Partitions > args.TopicSpec.Partitions.Int64 {
					return operations.Operation{}, semerr.InvalidInputf("reducing the number of topic partitions is not supported, you are trying to decrease number of partitions for topic %q from %d to %d", args.Name, topic.Partitions, args.TopicSpec.Partitions.Int64)
				}
				topic.Partitions = args.TopicSpec.Partitions.Int64
			}

			if args.TopicSpec.ReplicationFactor.Valid && args.TopicSpec.ReplicationFactor.Int64 != topic.ReplicationFactor {
				changes = true
				topic.ReplicationFactor = args.TopicSpec.ReplicationFactor.Int64
			}

			configHasChanges := optional.ApplyUpdate(&topic.Config, &args.TopicSpec.Config)
			if configHasChanges {
				changes = true
			}

			if !changes {
				return operations.Operation{}, semerr.InvalidInput("no changes found in request")
			}

			pillar.SetTopic(topic)
			if err := modifier.UpdatePillar(ctx, cluster.ClusterID, cluster.Revision, pillar); err != nil {
				return operations.Operation{}, err
			}

			taskArgs := make(map[string]interface{})
			taskArgs["target-topic"] = args.Name
			taskArgs["feature_flags"] = session.FeatureFlags

			op, err := kf.tasks.CreateTask(
				ctx,
				session,
				tasks.CreateTaskArgs{
					ClusterID:     cluster.ClusterID,
					FolderID:      session.FolderCoords.FolderID,
					Auth:          session.Subject,
					TaskType:      kfmodels.TaskTypeTopicModify,
					TaskArgs:      taskArgs,
					OperationType: kfmodels.OperationTypeTopicModify,
					Metadata:      kfmodels.MetadataModifyTopic{TopicName: args.Name},
					Revision:      cluster.Revision,
				},
			)
			if err != nil {
				return operations.Operation{}, err
			}
			return op, nil
		},
		clusterslogic.WithPermission(kfmodels.PermClustersUpdate),
	)
}

func (kf *Kafka) DeleteTopic(ctx context.Context, cid string, name string) (operations.Operation, error) {
	return kf.operator.DeleteOnCluster(ctx, cid, clusters.TypeKafka,
		func(ctx context.Context, session sessions.Session, _ clusterslogic.Reader, modifier clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {
			if name == "" {
				return operations.Operation{}, semerr.InvalidInput("topic name must be specified")
			}

			pillar := kfpillars.NewCluster()
			if err := cluster.Pillar(pillar); err != nil {
				return operations.Operation{}, err
			}

			// We consider topic list empty in unmanaged mode
			if pillar.Data.Kafka.TopicsAreManagedViaAdminAPIOnly() {
				return operations.Operation{}, semerr.NotFound("in unmanaged mode you have to use AdminAPI for topics management")
			}

			if err := pillar.DeleteTopic(name); err != nil {
				return operations.Operation{}, err
			}

			if err := modifier.UpdatePillar(ctx, cluster.ClusterID, cluster.Revision, pillar); err != nil {
				return operations.Operation{}, err
			}

			args := make(map[string]interface{})
			args["target-topic"] = name
			args["feature_flags"] = session.FeatureFlags

			op, err := kf.tasks.CreateTask(
				ctx,
				session,
				tasks.CreateTaskArgs{
					ClusterID:     cid,
					FolderID:      session.FolderCoords.FolderID,
					Auth:          session.Subject,
					TaskType:      kfmodels.TaskTypeTopicDelete,
					TaskArgs:      args,
					OperationType: kfmodels.OperationTypeTopicDelete,
					Metadata:      kfmodels.MetadataDeleteTopic{TopicName: name},
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
