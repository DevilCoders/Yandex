package provider

import (
	"context"
	"reflect"
	"sort"

	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/crypto"
	clusterslogic "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/sessions"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/kafka/kfmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/kafka/provider/internal/kfpillars"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/pagination"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/tasks"
	"a.yandex-team.ru/library/go/core/xerrors"
)

const (
	dataCloudOwnerUserName = "admin"
)

func userFromPillar(cid string, user kfpillars.UserData) kfmodels.User {
	permissions := make([]kfmodels.Permission, 0, len(user.Permissions))
	for _, permission := range user.Permissions {
		permissions = append(permissions, kfmodels.Permission{
			TopicName:  permission.TopicName,
			AccessRole: kfmodels.AccessRoleType(permission.Role),
			Group:      permission.Group,
			Host:       permission.Host,
		})
	}
	return kfmodels.User{
		ClusterID:   cid,
		Name:        user.Name,
		Permissions: permissions,
	}
}

func (kf *Kafka) User(ctx context.Context, cid, name string) (kfmodels.User, error) {
	var res kfmodels.User
	if err := kf.operator.ReadOnCluster(ctx, cid, clusters.TypeKafka,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, cl clusterslogic.Cluster) error {
			var err error
			if name == "" {
				return semerr.InvalidInput("topic name must be specified")
			}

			pillar := kfpillars.NewCluster()
			if err = cl.Pillar(pillar); err != nil {
				return err
			}

			user, ok := pillar.Data.Kafka.Users[name]
			if !ok {
				return semerr.NotFoundf("user %q not found", name)
			}

			res = userFromPillar(cid, user)
			return nil
		},
		clusterslogic.WithPermission(kfmodels.PermClustersGet),
	); err != nil {
		return kfmodels.User{}, err
	}

	return res, nil
}

func (kf *Kafka) Users(ctx context.Context, cid string, limit, offset int64) ([]kfmodels.User, pagination.OffsetPageToken, error) {
	var userModels []kfmodels.User
	if err := kf.operator.ReadOnCluster(ctx, cid, clusters.TypeKafka,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, cl clusterslogic.Cluster) error {
			var err error

			pillar := kfpillars.NewCluster()
			if err = cl.Pillar(pillar); err != nil {
				return err
			}

			userModels = make([]kfmodels.User, 0, len(pillar.Data.Kafka.Users))
			for _, user := range pillar.Data.Kafka.Users {
				userModels = append(userModels, userFromPillar(cid, user))
			}

			sort.Slice(userModels, func(i, j int) bool {
				return userModels[i].Name < userModels[j].Name
			})

			return nil
		},
		clusterslogic.WithPermission(kfmodels.PermClustersGet),
	); err != nil {
		return nil, pagination.OffsetPageToken{}, err
	}

	page := pagination.NewPage(int64(len(userModels)), limit, offset)

	return userModels[page.LowerIndex:page.UpperIndex], pagination.OffsetPageToken{
		Offset: page.NextPageOffset,
		More:   page.HasMore,
	}, nil
}

func permissionsToPillar(perms []kfmodels.Permission) []kfpillars.PermissionsData {
	permissions := make([]kfpillars.PermissionsData, 0, len(perms))
	for _, perm := range perms {
		permissions = append(permissions, kfpillars.PermissionsData{
			TopicName: perm.TopicName,
			Role:      string(perm.AccessRole),
			Group:     perm.Group,
			Host:      perm.Host,
		})
	}
	return permissions
}

func addPermissionToPillar(pillarPerms []kfpillars.PermissionsData, perm kfmodels.Permission) ([]kfpillars.PermissionsData, bool) {
	permNew := kfpillars.PermissionsData{
		TopicName: perm.TopicName,
		Role:      string(perm.AccessRole),
		Group:     perm.Group,
		Host:      perm.Host,
	}
	result := make([]kfpillars.PermissionsData, len(pillarPerms))
	copy(result, pillarPerms)
	for _, permOld := range pillarPerms {
		if reflect.DeepEqual(permNew, permOld) {
			return result, false
		}
	}
	result = append(result, permNew)
	return result, true
}

func removePermissionFromPillar(pillarPerms []kfpillars.PermissionsData, perm kfmodels.Permission) ([]kfpillars.PermissionsData, bool) {
	permNew := kfpillars.PermissionsData{
		TopicName: perm.TopicName,
		Role:      string(perm.AccessRole),
		Group:     perm.Group,
		Host:      perm.Host,
	}
	changed := false
	result := make([]kfpillars.PermissionsData, 0, len(pillarPerms))
	for _, permOld := range pillarPerms {
		if reflect.DeepEqual(permNew, permOld) {
			changed = true
			continue
		}
		result = append(result, permOld)
	}
	return result, changed
}

func (kf *Kafka) CreateUser(ctx context.Context, cid string, spec kfmodels.UserSpec) (operations.Operation, error) {
	if err := spec.Validate(); err != nil {
		return operations.Operation{}, err
	}

	return kf.operator.CreateOnCluster(ctx, cid, clusters.TypeKafka,
		func(ctx context.Context, session sessions.Session, _ clusterslogic.Reader, modifier clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {
			pillar := kfpillars.NewCluster()
			if err := cluster.Pillar(pillar); err != nil {
				return operations.Operation{}, err
			}

			err := kfmodels.ValidateNonEmptyPassword(spec.Password)
			if err != nil {
				return operations.Operation{}, err
			}
			passwordEncrypted, err := kf.cryptoProvider.Encrypt([]byte(spec.Password.Unmask()))
			if err != nil {
				return operations.Operation{}, xerrors.Errorf("failed to encrypt user %q password: %w", spec.Name, err)
			}

			err = pillar.AddUser(kfpillars.UserData{
				Name:        spec.Name,
				Password:    passwordEncrypted,
				Permissions: permissionsToPillar(spec.Permissions),
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

			op, err := kf.tasks.CreateTask(
				ctx,
				session,
				tasks.CreateTaskArgs{
					ClusterID:     cid,
					FolderID:      session.FolderCoords.FolderID,
					Auth:          session.Subject,
					TaskType:      kfmodels.TaskTypeUserCreate,
					TaskArgs:      args,
					OperationType: kfmodels.OperationTypeUserCreate,
					Metadata:      kfmodels.MetadataCreateUser{UserName: spec.Name},
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

func (kf *Kafka) UpdateUser(ctx context.Context, args kfmodels.UpdateUserArgs) (operations.Operation, error) {
	if err := args.Validate(); err != nil {
		return operations.Operation{}, err
	}

	return kf.operator.ModifyOnCluster(ctx, args.ClusterID, clusters.TypeKafka,
		func(ctx context.Context, session sessions.Session, _ clusterslogic.Reader, modifier clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {
			pillar := kfpillars.NewCluster()
			if err := cluster.Pillar(pillar); err != nil {
				return operations.Operation{}, err
			}

			user, ok := pillar.GetUser(args.Name)
			if !ok {
				return operations.Operation{}, semerr.NotFoundf("user %q not found", args.Name)
			}

			if args.Password.Unmask() != "" {
				passwordEncrypted, err := kf.cryptoProvider.Encrypt([]byte(args.Password.Unmask()))
				if err != nil {
					return operations.Operation{}, xerrors.Errorf("failed to encrypt user %q password: %w", args.Name, err)
				}
				user.Password = passwordEncrypted
			}

			if args.PermissionsChanged {
				user.Permissions = permissionsToPillar(args.Permissions)
			}

			pillar.SetUser(user)
			if err := modifier.UpdatePillar(ctx, cluster.ClusterID, cluster.Revision, pillar); err != nil {
				return operations.Operation{}, err
			}

			taskArgs := make(map[string]interface{})
			taskArgs["target-user"] = args.Name
			taskArgs["feature_flags"] = session.FeatureFlags

			op, err := kf.tasks.CreateTask(
				ctx,
				session,
				tasks.CreateTaskArgs{
					ClusterID:     cluster.ClusterID,
					FolderID:      session.FolderCoords.FolderID,
					Auth:          session.Subject,
					TaskType:      kfmodels.TaskTypeUserModify,
					TaskArgs:      taskArgs,
					OperationType: kfmodels.OperationTypeUserModify,
					Metadata:      kfmodels.MetadataModifyUser{UserName: args.Name},
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

func (kf *Kafka) GrantUserPermission(ctx context.Context, cid string, name string, permission kfmodels.Permission) (operations.Operation, error) {
	if err := permission.Validate(); err != nil {
		return operations.Operation{}, err
	}

	return kf.operator.ModifyOnCluster(ctx, cid, clusters.TypeKafka,
		func(ctx context.Context, session sessions.Session, _ clusterslogic.Reader, modifier clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {
			pillar := kfpillars.NewCluster()
			if err := cluster.Pillar(pillar); err != nil {
				return operations.Operation{}, err
			}

			user, ok := pillar.GetUser(name)
			if !ok {
				return operations.Operation{}, semerr.NotFoundf("user %q not found", name)
			}

			permissionsNew, changed := addPermissionToPillar(user.Permissions, permission)

			if !changed {
				return operations.Operation{}, semerr.InvalidInput("no changes found in request")
			}

			user.Permissions = permissionsNew
			pillar.SetUser(user)
			if err := modifier.UpdatePillar(ctx, cluster.ClusterID, cluster.Revision, pillar); err != nil {
				return operations.Operation{}, err
			}

			taskArgs := make(map[string]interface{})
			taskArgs["target-user"] = name
			taskArgs["feature_flags"] = session.FeatureFlags

			op, err := kf.tasks.CreateTask(
				ctx,
				session,
				tasks.CreateTaskArgs{
					ClusterID:     cluster.ClusterID,
					FolderID:      session.FolderCoords.FolderID,
					Auth:          session.Subject,
					TaskType:      kfmodels.TaskTypeUserModify,
					TaskArgs:      taskArgs,
					OperationType: kfmodels.OperationTypeUserGrantPermission,
					Metadata:      kfmodels.MetadataModifyUser{UserName: name},
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

func (kf *Kafka) RevokeUserPermission(ctx context.Context, cid string, name string, permission kfmodels.Permission) (operations.Operation, error) {
	if err := permission.Validate(); err != nil {
		return operations.Operation{}, err
	}

	return kf.operator.ModifyOnCluster(ctx, cid, clusters.TypeKafka,
		func(ctx context.Context, session sessions.Session, _ clusterslogic.Reader, modifier clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {
			pillar := kfpillars.NewCluster()
			if err := cluster.Pillar(pillar); err != nil {
				return operations.Operation{}, err
			}

			user, ok := pillar.GetUser(name)
			if !ok {
				return operations.Operation{}, semerr.NotFoundf("user %q not found", name)
			}

			permissionsNew, changed := removePermissionFromPillar(user.Permissions, permission)

			if !changed {
				return operations.Operation{}, semerr.InvalidInput("no changes found in request")
			}

			user.Permissions = permissionsNew
			pillar.SetUser(user)
			if err := modifier.UpdatePillar(ctx, cluster.ClusterID, cluster.Revision, pillar); err != nil {
				return operations.Operation{}, err
			}

			taskArgs := make(map[string]interface{})
			taskArgs["target-user"] = name
			taskArgs["feature_flags"] = session.FeatureFlags

			op, err := kf.tasks.CreateTask(
				ctx,
				session,
				tasks.CreateTaskArgs{
					ClusterID:     cluster.ClusterID,
					FolderID:      session.FolderCoords.FolderID,
					Auth:          session.Subject,
					TaskType:      kfmodels.TaskTypeUserModify,
					TaskArgs:      taskArgs,
					OperationType: kfmodels.OperationTypeUserRevokePermission,
					Metadata:      kfmodels.MetadataModifyUser{UserName: name},
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

func (kf *Kafka) DeleteUser(ctx context.Context, cid string, name string) (operations.Operation, error) {
	return kf.operator.DeleteOnCluster(ctx, cid, clusters.TypeKafka,
		func(ctx context.Context, session sessions.Session, _ clusterslogic.Reader, modifier clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {
			pillar := kfpillars.NewCluster()
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

			op, err := kf.tasks.CreateTask(
				ctx,
				session,
				tasks.CreateTaskArgs{
					ClusterID:     cid,
					FolderID:      session.FolderCoords.FolderID,
					Auth:          session.Subject,
					TaskType:      kfmodels.TaskTypeUserDelete,
					TaskArgs:      args,
					OperationType: kfmodels.OperationTypeUserDelete,
					Metadata:      kfmodels.MetadataDeleteUser{UserName: name},
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

func (kf *Kafka) ResetCredentials(ctx context.Context, cid string) (operations.Operation, error) {
	return kf.operator.ModifyOnNotStoppedCluster(ctx, cid, clusters.TypeKafka, func(ctx context.Context,
		session sessions.Session, reader clusterslogic.Reader, modifier clusterslogic.Modifier,
		cluster clusterslogic.Cluster) (operations.Operation, error) {

		pillar, err := getPillarFromCluster(cluster)
		if err != nil {
			return operations.Operation{}, err
		}

		adminUserName := pillar.GetAdminUserName()
		err = kf.resetPasswordForUserInPillar(pillar, adminUserName)
		if err != nil {
			return operations.Operation{}, err
		}

		if err := modifier.UpdatePillar(ctx, cid, cluster.Revision, pillar); err != nil {
			return operations.Operation{}, err
		}

		op, err := kf.tasks.CreateTask(
			ctx,
			session,
			tasks.CreateTaskArgs{
				ClusterID:     cid,
				FolderID:      session.FolderCoords.FolderID,
				Auth:          session.Subject,
				OperationType: kfmodels.OperationTypeUserModify,
				TaskType:      kfmodels.TaskTypeUserModify,
				Metadata:      kfmodels.MetadataModifyUser{UserName: adminUserName},
				TaskArgs:      map[string]interface{}{"target-user": adminUserName},
				Revision:      cluster.Revision,
			},
		)
		if err != nil {
			return operations.Operation{}, xerrors.Errorf("failed to create operation: %w", err)
		}

		return op, nil
	},
		clusterslogic.WithPermission(kfmodels.PermClustersUpdate),
	)
}

func (kf *Kafka) resetPasswordForUserInPillar(pillar *kfpillars.Cluster, userName string) error {
	user, found := pillar.Data.Kafka.Users[userName]
	if !found {
		return xerrors.Errorf("can't find user: %s", userName)
	}
	newPassword, err := crypto.GenerateEncryptedPassword(kf.cryptoProvider, passwordGenLen, nil)
	if err != nil {
		return err
	}
	user.Password = newPassword
	pillar.Data.Kafka.Users[userName] = user

	return nil
}
