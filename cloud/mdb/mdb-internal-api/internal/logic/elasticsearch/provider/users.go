package provider

import (
	"context"
	"sort"

	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/elasticsearch"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/elasticsearch/esmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/elasticsearch/provider/internal/espillars"
	clusterslogic "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/sessions"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/tasks"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func userFromPillar(cid string, user espillars.UserData) esmodels.User {
	return esmodels.User{
		ClusterID: cid,
		Name:      user.Name,
	}
}

func (es *ElasticSearch) User(ctx context.Context, cid, name string) (esmodels.User, error) {
	var res esmodels.User
	if err := es.operator.ReadOnCluster(ctx, cid, clusters.TypeElasticSearch,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, cl clusterslogic.Cluster) error {
			var err error

			pillar := espillars.NewCluster()
			if err = cl.Pillar(pillar); err != nil {
				return err
			}

			user, ok := pillar.Data.ElasticSearch.Users[name]
			if !ok {
				return semerr.NotFoundf("user %q not found", name)
			}
			if user.Internal {
				return semerr.NotFoundf("user %q not found", name)
			}

			res = userFromPillar(cid, user)
			return nil
		},
		clusterslogic.WithPermission(esmodels.PermClustersGet),
	); err != nil {
		return esmodels.User{}, err
	}

	return res, nil
}

func (es *ElasticSearch) Users(ctx context.Context, cid string, limit, offset int64) ([]esmodels.User, error) {
	var res []esmodels.User
	if err := es.operator.ReadOnCluster(ctx, cid, clusters.TypeElasticSearch,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, cl clusterslogic.Cluster) error {
			var err error

			pillar := espillars.NewCluster()
			if err = cl.Pillar(pillar); err != nil {
				return err
			}

			// Limit and offset filtering in memory. Sorting by topic name
			userNames := make([]string, 0, len(pillar.Data.ElasticSearch.Users))
			for name, user := range pillar.Data.ElasticSearch.Users {
				if user.Internal {
					continue
				}
				userNames = append(userNames, name)
			}
			sort.Strings(userNames)
			if offset > 0 {
				if offset > int64(len(userNames)) {
					return nil
				}
				userNames = userNames[offset:]
			}
			if limit < 1 {
				limit = 100
			}
			if limit < int64(len(userNames)) {
				userNames = userNames[:limit]
			}

			// Construct response
			res = make([]esmodels.User, 0, len(userNames))
			for _, name := range userNames {
				user := pillar.Data.ElasticSearch.Users[name]
				res = append(res, userFromPillar(cid, user))
			}

			return nil
		},
		clusterslogic.WithPermission(esmodels.PermClustersGet),
	); err != nil {
		return nil, err
	}

	return res, nil
}

func (es *ElasticSearch) CreateUser(ctx context.Context, cid string, spec esmodels.UserSpec) (operations.Operation, error) {
	if err := spec.Validate(); err != nil {
		return operations.Operation{}, err
	}

	return es.operator.CreateOnCluster(ctx, cid, clusters.TypeElasticSearch,
		func(ctx context.Context, session sessions.Session, _ clusterslogic.Reader, modifier clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {
			pillar := espillars.NewCluster()
			if err := cluster.Pillar(pillar); err != nil {
				return operations.Operation{}, err
			}

			passwordEncrypted, err := es.cryptoProvider.Encrypt([]byte(spec.Password.Unmask()))
			if err != nil {
				return operations.Operation{}, xerrors.Errorf("failed to encrypt user %q password: %w", spec.Name, err)
			}

			err = pillar.AddUser(espillars.UserData{
				Name:     spec.Name,
				Password: passwordEncrypted,
				Roles:    []string{esmodels.RoleAdmin},
			})
			if err != nil {
				return operations.Operation{}, err
			}

			if err = modifier.UpdatePillar(ctx, cluster.ClusterID, cluster.Revision, pillar); err != nil {
				return operations.Operation{}, err
			}

			args := make(map[string]interface{})
			args["target-user"] = spec.Name
			args["feature_flags"] = session.FeatureFlags

			op, err := es.tasks.CreateTask(
				ctx,
				session,
				tasks.CreateTaskArgs{
					ClusterID:     cid,
					FolderID:      session.FolderCoords.FolderID,
					Auth:          session.Subject,
					TaskType:      esmodels.TaskTypeUserCreate,
					TaskArgs:      args,
					OperationType: esmodels.OperationTypeUserCreate,
					Metadata:      esmodels.MetadataCreateUser{UserName: spec.Name},
					Revision:      cluster.Revision,
				},
			)
			if err != nil {
				return operations.Operation{}, err
			}

			if err := es.toSearchQueue(ctx, session.FolderCoords, op); err != nil {
				return operations.Operation{}, err
			}

			return op, nil
		},
		clusterslogic.WithPermission(esmodels.PermClustersUpdate),
	)
}

func (es *ElasticSearch) UpdateUser(ctx context.Context, args elasticsearch.UpdateUserArgs) (operations.Operation, error) {
	return es.operator.ModifyOnCluster(ctx, args.ClusterID, clusters.TypeElasticSearch,
		func(ctx context.Context, session sessions.Session, _ clusterslogic.Reader, modifier clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {
			if err := esmodels.UserNameValidator.ValidateString(args.Name); err != nil {
				return operations.Operation{}, semerr.NotFoundf("user %q not found", args.Name)
			}

			pillar := espillars.NewCluster()
			if err := cluster.Pillar(pillar); err != nil {
				return operations.Operation{}, err
			}

			user, ok := pillar.GetUser(args.Name)
			if !ok {
				return operations.Operation{}, semerr.NotFoundf("user %q not found", args.Name)
			}

			if args.Password.Unmask() != "" {
				passwordEncrypted, err := es.cryptoProvider.Encrypt([]byte(args.Password.Unmask()))
				if err != nil {
					return operations.Operation{}, xerrors.Errorf("failed to encrypt user %q password: %w", args.Name, err)
				}
				user.Password = passwordEncrypted
			}

			pillar.SetUser(user)
			if err := modifier.UpdatePillar(ctx, cluster.ClusterID, cluster.Revision, pillar); err != nil {
				return operations.Operation{}, err
			}

			taskArgs := make(map[string]interface{})
			taskArgs["target-user"] = args.Name
			taskArgs["feature_flags"] = session.FeatureFlags

			op, err := es.tasks.CreateTask(
				ctx,
				session,
				tasks.CreateTaskArgs{
					ClusterID:     cluster.ClusterID,
					FolderID:      session.FolderCoords.FolderID,
					Auth:          session.Subject,
					TaskType:      esmodels.TaskTypeUserModify,
					TaskArgs:      taskArgs,
					OperationType: esmodels.OperationTypeUserModify,
					Metadata:      esmodels.MetadataModifyUser{UserName: args.Name},
					Revision:      cluster.Revision,
				},
			)
			if err != nil {
				return operations.Operation{}, err
			}
			return op, nil
		},
		clusterslogic.WithPermission(esmodels.PermClustersUpdate),
	)
}

func (es *ElasticSearch) DeleteUser(ctx context.Context, cid string, name string) (operations.Operation, error) {
	return es.operator.DeleteOnCluster(ctx, cid, clusters.TypeElasticSearch,
		func(ctx context.Context, session sessions.Session, _ clusterslogic.Reader, modifier clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {
			if err := esmodels.UserNameValidator.ValidateString(name); err != nil {
				return operations.Operation{}, semerr.NotFoundf("user %q not found", name)
			}

			pillar := espillars.NewCluster()
			if err := cluster.Pillar(pillar); err != nil {
				return operations.Operation{}, err
			}

			if err := pillar.DeleteUser(name); err != nil {
				return operations.Operation{}, err
			}

			if err := modifier.UpdatePillar(ctx, cluster.ClusterID, cluster.Revision, pillar); err != nil {
				return operations.Operation{}, err
			}

			args := make(map[string]interface{})
			args["target-user"] = name
			args["feature_flags"] = session.FeatureFlags

			op, err := es.tasks.CreateTask(
				ctx,
				session,
				tasks.CreateTaskArgs{
					ClusterID:     cid,
					FolderID:      session.FolderCoords.FolderID,
					Auth:          session.Subject,
					TaskType:      esmodels.TaskTypeUserDelete,
					TaskArgs:      args,
					OperationType: esmodels.OperationTypeUserDelete,
					Metadata:      esmodels.MetadataDeleteUser{UserName: name},
					Revision:      cluster.Revision,
				},
			)
			if err != nil {
				return operations.Operation{}, err
			}

			if err := es.toSearchQueue(ctx, session.FolderCoords, op); err != nil {
				return operations.Operation{}, err
			}

			return op, nil
		},
		clusterslogic.WithPermission(esmodels.PermClustersUpdate),
	)
}
