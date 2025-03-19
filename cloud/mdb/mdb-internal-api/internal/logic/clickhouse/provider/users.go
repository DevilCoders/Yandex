package provider

import (
	"context"
	"crypto/sha256"
	"encoding/hex"
	"sort"

	cheventspub "a.yandex-team.ru/cloud/bitbucket/public-api/yandex/cloud/events/mdb/clickhouse"
	"a.yandex-team.ru/cloud/mdb/internal/secret"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/crypto"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/clickhouse/chmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/clickhouse/provider/internal/chpillars"
	clusterslogic "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/sessions"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/pagination"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/pillars"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/tasks"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (ch *ClickHouse) User(ctx context.Context, cid, name string) (chmodels.User, error) {
	var userModel chmodels.User
	if err := ch.operator.ReadOnCluster(ctx, cid, clusters.TypeClickHouse,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, _ clusterslogic.Cluster) error {
			sc, err := ch.chSubCluster(ctx, reader, cid)
			if err != nil {
				return err
			}

			if sc.Pillar.SQLUserManagement() {
				return semerr.FailedPrecondition("operation not permitted when SQL user management is enabled.")
			}

			for userName, user := range sc.Pillar.Data.ClickHouse.Users {
				if userName == name {
					var err error
					userModel, err = chpillars.UserFromPillar(cid, userName, user)
					return err
				}
			}

			return semerr.NotFoundf("user %q not found", name)
		},
	); err != nil {
		return chmodels.User{}, err
	}

	return userModel, nil
}

func (ch *ClickHouse) Users(ctx context.Context, cid string, pageSize, offset int64) ([]chmodels.User, pagination.OffsetPageToken, error) {
	var userModels []chmodels.User
	if err := ch.operator.ReadOnCluster(ctx, cid, clusters.TypeClickHouse,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, _ clusterslogic.Cluster) error {
			sc, err := ch.chSubCluster(ctx, reader, cid)
			if err != nil {
				return err
			}

			if sc.Pillar.SQLUserManagement() {
				return nil
			}

			userModels = make([]chmodels.User, 0, len(sc.Pillar.Data.ClickHouse.Users))
			for userName, user := range sc.Pillar.Data.ClickHouse.Users {
				user, err := chpillars.UserFromPillar(cid, userName, user)
				if err != nil {
					return err
				}
				userModels = append(userModels, user)
			}

			sort.Slice(userModels, func(i, j int) bool {
				return userModels[i].Name < userModels[j].Name
			})

			return nil
		},
	); err != nil {
		return nil, pagination.OffsetPageToken{}, err
	}

	page := pagination.NewPage(int64(len(userModels)), pageSize, offset)

	return userModels[page.LowerIndex:page.UpperIndex], pagination.OffsetPageToken{
		Offset: page.NextPageOffset,
		More:   page.HasMore,
	}, nil
}

func (ch *ClickHouse) CreateUser(ctx context.Context, cid string, spec chmodels.UserSpec) (operations.Operation, error) {
	if err := spec.Validate(); err != nil {
		return operations.Operation{}, err
	}

	return ch.operator.CreateOnCluster(ctx, cid, clusters.TypeClickHouse,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, modifier clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {
			sc, err := ch.chSubCluster(ctx, reader, cid)
			if err != nil {
				return operations.Operation{}, err
			}

			if sc.Pillar.SQLUserManagement() {
				return operations.Operation{}, semerr.FailedPrecondition("operation not permitted when SQL user management is enabled.")
			}

			password, hash, err := ch.encryptUserPassword(spec.Password)
			if err != nil {
				return operations.Operation{}, err
			}

			if err = sc.Pillar.AddUser(spec, password, hash); err != nil {
				return operations.Operation{}, err
			}

			if err = modifier.UpdateSubClusterPillar(ctx, sc.ClusterID, sc.SubClusterID, cluster.Revision, sc.Pillar); err != nil {
				return operations.Operation{}, err
			}

			op, err := ch.tasks.CreateTask(
				ctx,
				session,
				tasks.CreateTaskArgs{
					ClusterID:     cid,
					FolderID:      session.FolderCoords.FolderID,
					Auth:          session.Subject,
					OperationType: chmodels.OperationTypeUserCreate,
					TaskType:      chmodels.TaskTypeUserCreate,
					TaskArgs:      map[string]interface{}{"target-user": spec.Name},
					Metadata:      chmodels.MetadataCreateUser{UserName: spec.Name},
					Revision:      cluster.Revision,
				},
			)
			if err != nil {
				return operations.Operation{}, xerrors.Errorf("failed to create operation: %w", err)
			}

			event := &cheventspub.CreateUser{
				Authentication:  ch.events.NewAuthentication(session.Subject),
				Authorization:   ch.events.NewAuthorization(session.Subject),
				RequestMetadata: ch.events.NewRequestMetadata(ctx),
				EventStatus:     cheventspub.CreateUser_STARTED,
				Details: &cheventspub.CreateUser_EventDetails{
					ClusterId: cid,
					UserName:  spec.Name,
				},
			}
			em, err := ch.events.NewEventMetadata(event, op, session.FolderCoords.CloudExtID, session.FolderCoords.FolderExtID)
			if err != nil {
				return operations.Operation{}, err
			}
			event.EventMetadata = em

			if err = ch.events.Store(ctx, event, op); err != nil {
				return operations.Operation{}, xerrors.Errorf("failed to save event for op %+v: %w", op, err)
			}

			return op, nil
		},
	)
}

func (ch *ClickHouse) UpdateUser(ctx context.Context, cid string, name string, spec chmodels.UpdateUserArgs) (operations.Operation, error) {
	if err := spec.Validate(); err != nil {
		return operations.Operation{}, err
	}

	return ch.operator.ModifyOnNotStoppedCluster(ctx, cid, clusters.TypeClickHouse,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, modifier clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {
			sc, err := ch.chSubCluster(ctx, reader, cid)
			if err != nil {
				return operations.Operation{}, err
			}

			if sc.Pillar.SQLUserManagement() {
				return operations.Operation{}, semerr.FailedPrecondition("operation not permitted when SQL user management is enabled.")
			}

			if spec.Password != nil {
				password, hash, err := ch.encryptUserPassword(*spec.Password)
				if err != nil {
					return operations.Operation{}, err
				}

				if err := sc.Pillar.UpdateUserPassword(name, password, hash); err != nil {
					return operations.Operation{}, err
				}
			}

			if err = sc.Pillar.ModifyUser(name, spec); err != nil {
				return operations.Operation{}, err
			}

			if err = modifier.UpdateSubClusterPillar(ctx, sc.ClusterID, sc.SubClusterID, cluster.Revision, sc.Pillar); err != nil {
				return operations.Operation{}, err
			}

			op, err := ch.tasks.CreateTask(
				ctx,
				session,
				tasks.CreateTaskArgs{
					ClusterID:     cid,
					FolderID:      session.FolderCoords.FolderID,
					Auth:          session.Subject,
					OperationType: chmodels.OperationTypeUserModify,
					TaskType:      chmodels.TaskTypeUserModify,
					TaskArgs:      map[string]interface{}{"target-user": name},
					Metadata:      chmodels.MetadataModifyUser{UserName: name},
					Revision:      cluster.Revision,
				},
			)
			if err != nil {
				return operations.Operation{}, xerrors.Errorf("failed to create operation: %w", err)
			}

			event := &cheventspub.UpdateUser{
				Authentication:  ch.events.NewAuthentication(session.Subject),
				Authorization:   ch.events.NewAuthorization(session.Subject),
				RequestMetadata: ch.events.NewRequestMetadata(ctx),
				EventStatus:     cheventspub.UpdateUser_STARTED,
				Details: &cheventspub.UpdateUser_EventDetails{
					ClusterId: cid,
					UserName:  name,
				},
			}

			em, err := ch.events.NewEventMetadata(event, op, session.FolderCoords.CloudExtID, session.FolderCoords.FolderExtID)
			if err != nil {
				return operations.Operation{}, err
			}
			event.EventMetadata = em

			if err = ch.events.Store(ctx, event, op); err != nil {
				return operations.Operation{}, xerrors.Errorf("failed to save event for op %+v: %w", op, err)
			}

			return op, nil
		},
	)
}

func (ch *ClickHouse) DeleteUser(ctx context.Context, cid string, name string) (operations.Operation, error) {
	return ch.operator.DeleteOnCluster(ctx, cid, clusters.TypeClickHouse,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, modifier clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {
			sc, err := ch.chSubCluster(ctx, reader, cid)
			if err != nil {
				return operations.Operation{}, err
			}

			if sc.Pillar.SQLUserManagement() {
				return operations.Operation{}, semerr.FailedPrecondition("operation not permitted when SQL user management is enabled.")
			}

			if err = sc.Pillar.DeleteUser(name); err != nil {
				return operations.Operation{}, err
			}

			if err = modifier.UpdateSubClusterPillar(ctx, sc.ClusterID, sc.SubClusterID, cluster.Revision, sc.Pillar); err != nil {
				return operations.Operation{}, err
			}

			op, err := ch.tasks.CreateTask(
				ctx,
				session,
				tasks.CreateTaskArgs{
					ClusterID:     cid,
					FolderID:      session.FolderCoords.FolderID,
					Auth:          session.Subject,
					OperationType: chmodels.OperationTypeUserDelete,
					TaskType:      chmodels.TaskTypeUserDelete,
					TaskArgs:      map[string]interface{}{"target-user": name},
					Metadata:      chmodels.MetadataDeleteUser{UserName: name},
					Revision:      cluster.Revision,
				},
			)
			if err != nil {
				return operations.Operation{}, xerrors.Errorf("failed to create operation: %w", err)
			}

			event := &cheventspub.DeleteUser{
				Authentication:  ch.events.NewAuthentication(session.Subject),
				Authorization:   ch.events.NewAuthorization(session.Subject),
				RequestMetadata: ch.events.NewRequestMetadata(ctx),
				EventStatus:     cheventspub.DeleteUser_STARTED,
				Details: &cheventspub.DeleteUser_EventDetails{
					ClusterId: cid,
					UserName:  name,
				},
			}
			em, err := ch.events.NewEventMetadata(event, op, session.FolderCoords.CloudExtID, session.FolderCoords.FolderExtID)
			if err != nil {
				return operations.Operation{}, err
			}
			event.EventMetadata = em

			if err = ch.events.Store(ctx, event, op); err != nil {
				return operations.Operation{}, xerrors.Errorf("failed to save event for op %+v: %w", op, err)
			}

			return op, nil
		},
	)
}

func (ch *ClickHouse) GrantPermission(ctx context.Context, cid, name string, permission chmodels.Permission) (operations.Operation, error) {
	return ch.operator.ModifyOnNotStoppedCluster(ctx, cid, clusters.TypeClickHouse,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, modifier clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {
			sc, err := ch.chSubCluster(ctx, reader, cid)
			if err != nil {
				return operations.Operation{}, err
			}

			if sc.Pillar.SQLUserManagement() {
				return operations.Operation{}, semerr.FailedPrecondition("operation not permitted when SQL user management is enabled.")
			}

			if err = sc.Pillar.GrantPermission(name, permission); err != nil {
				return operations.Operation{}, err
			}

			if err = modifier.UpdateSubClusterPillar(ctx, sc.ClusterID, sc.SubClusterID, cluster.Revision, sc.Pillar); err != nil {
				return operations.Operation{}, err
			}

			op, err := ch.tasks.CreateTask(
				ctx,
				session,
				tasks.CreateTaskArgs{
					ClusterID:     cid,
					FolderID:      session.FolderCoords.FolderID,
					Auth:          session.Subject,
					OperationType: chmodels.OperationTypeUserGrantPermission,
					TaskType:      chmodels.TaskTypeUserModify,
					TaskArgs:      map[string]interface{}{"target-user": name},
					Metadata:      chmodels.MetadataGrantUserPermission{UserName: name},
					Revision:      cluster.Revision,
				},
			)
			if err != nil {
				return operations.Operation{}, xerrors.Errorf("failed to create operation: %w", err)
			}

			event := &cheventspub.GrantUserPermission{
				Authentication:  ch.events.NewAuthentication(session.Subject),
				Authorization:   ch.events.NewAuthorization(session.Subject),
				RequestMetadata: ch.events.NewRequestMetadata(ctx),
				EventStatus:     cheventspub.GrantUserPermission_STARTED,
				Details: &cheventspub.GrantUserPermission_EventDetails{
					ClusterId: cid,
					UserName:  name,
				},
			}
			em, err := ch.events.NewEventMetadata(event, op, session.FolderCoords.CloudExtID, session.FolderCoords.FolderExtID)
			if err != nil {
				return operations.Operation{}, err
			}
			event.EventMetadata = em

			if err = ch.events.Store(ctx, event, op); err != nil {
				return operations.Operation{}, xerrors.Errorf("failed to save event for op %+v: %w", op, err)
			}

			return op, nil
		},
	)
}

func (ch *ClickHouse) RevokePermission(ctx context.Context, cid, name, database string) (operations.Operation, error) {
	return ch.operator.ModifyOnNotStoppedCluster(ctx, cid, clusters.TypeClickHouse,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, modifier clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {
			sc, err := ch.chSubCluster(ctx, reader, cid)
			if err != nil {
				return operations.Operation{}, err
			}

			if sc.Pillar.SQLUserManagement() {
				return operations.Operation{}, semerr.FailedPrecondition("operation not permitted when SQL user management is enabled.")
			}

			if err = sc.Pillar.RevokePermission(name, database); err != nil {
				return operations.Operation{}, err
			}

			if err = modifier.UpdateSubClusterPillar(ctx, sc.ClusterID, sc.SubClusterID, cluster.Revision, sc.Pillar); err != nil {
				return operations.Operation{}, err
			}

			op, err := ch.tasks.CreateTask(
				ctx,
				session,
				tasks.CreateTaskArgs{
					ClusterID:     cid,
					FolderID:      session.FolderCoords.FolderID,
					Auth:          session.Subject,
					OperationType: chmodels.OperationTypeUserRevokePermission,
					TaskType:      chmodels.TaskTypeUserModify,
					TaskArgs:      map[string]interface{}{"target-user": name},
					Metadata:      chmodels.MetadataRevokeUserPermission{UserName: name},
					Revision:      cluster.Revision,
				},
			)
			if err != nil {
				return operations.Operation{}, xerrors.Errorf("failed to create operation: %w", err)
			}

			event := &cheventspub.RevokeUserPermission{
				Authentication:  ch.events.NewAuthentication(session.Subject),
				Authorization:   ch.events.NewAuthorization(session.Subject),
				RequestMetadata: ch.events.NewRequestMetadata(ctx),
				EventStatus:     cheventspub.RevokeUserPermission_STARTED,
				Details: &cheventspub.RevokeUserPermission_EventDetails{
					ClusterId: cid,
					UserName:  name,
				},
			}
			em, err := ch.events.NewEventMetadata(event, op, session.FolderCoords.CloudExtID, session.FolderCoords.FolderExtID)
			if err != nil {
				return operations.Operation{}, err
			}
			event.EventMetadata = em

			if err = ch.events.Store(ctx, event, op); err != nil {
				return operations.Operation{}, xerrors.Errorf("failed to save event for op %+v: %w", op, err)
			}

			return op, nil
		},
	)
}

func (ch *ClickHouse) encryptUserPassword(password secret.String) (pillars.CryptoKey, pillars.CryptoKey, error) {
	pass, err := ch.cryptoProvider.Encrypt([]byte(password.Unmask()))
	if err != nil {
		return pillars.CryptoKey{}, pillars.CryptoKey{}, err
	}

	hash := sha256.Sum256([]byte(password.Unmask()))
	passwordHash, err := ch.cryptoProvider.Encrypt([]byte(hex.EncodeToString(hash[:])))
	if err != nil {
		return pillars.CryptoKey{}, pillars.CryptoKey{}, err
	}
	return pass, passwordHash, nil
}

func (ch *ClickHouse) ResetCredentials(ctx context.Context, cid string) (operations.Operation, error) {
	return ch.operator.ModifyOnNotStoppedCluster(ctx, cid, clusters.TypeClickHouse, func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, modifier clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {
		newPass, err := ch.cryptoProvider.GenerateRandomString(chmodels.PasswordLen, []rune(crypto.PasswordValidRunes))
		if err != nil {
			return operations.Operation{}, err
		}

		chSubCluster, err := ch.chSubCluster(ctx, reader, cid)
		if err != nil {
			return operations.Operation{}, err
		}

		pass, hash, err := ch.encryptUserPassword(newPass)
		if err != nil {
			return operations.Operation{}, nil
		}
		chSubCluster.Pillar.Data.ClickHouse.AdminPassword = &chpillars.AdminPassword{
			Hash:     hash,
			Password: pass,
		}

		if err := modifier.UpdateSubClusterPillar(ctx, cid, chSubCluster.SubClusterID, cluster.Revision, chSubCluster.Pillar); err != nil {
			return operations.Operation{}, err
		}

		op, err := ch.tasks.CreateTask(
			ctx,
			session,
			tasks.CreateTaskArgs{
				ClusterID:     cid,
				FolderID:      session.FolderCoords.FolderID,
				Auth:          session.Subject,
				OperationType: chmodels.OperationTypeUserModify,
				TaskType:      chmodels.TaskTypeUserModify,
				Metadata:      chmodels.MetadataModifyUser{UserName: ""},
				TaskArgs:      map[string]interface{}{"target-user": ""},
				Revision:      cluster.Revision,
			},
		)
		if err != nil {
			return operations.Operation{}, xerrors.Errorf("failed to create operation: %w", err)
		}

		event := &cheventspub.UpdateUser{
			Authentication:  ch.events.NewAuthentication(session.Subject),
			Authorization:   ch.events.NewAuthorization(session.Subject),
			RequestMetadata: ch.events.NewRequestMetadata(ctx),
			EventStatus:     cheventspub.UpdateUser_STARTED,
			Details: &cheventspub.UpdateUser_EventDetails{
				ClusterId: cid,
			},
		}

		em, err := ch.events.NewEventMetadata(event, op, session.FolderCoords.CloudExtID, session.FolderCoords.FolderExtID)
		if err != nil {
			return operations.Operation{}, err
		}
		event.EventMetadata = em

		if err = ch.events.Store(ctx, event, op); err != nil {
			return operations.Operation{}, xerrors.Errorf("failed to save event for op %+v: %w", op, err)
		}

		return op, nil
	})
}
