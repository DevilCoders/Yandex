package provider

import (
	"context"
	"strconv"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	clusterslogic "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/sessions"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/mongodb/mongomodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/environment"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/tasks"
)

func (mg *MongoDB) ResetupHosts(ctx context.Context, cid string, hostNames []string) (operations.Operation, error) {
	// Check input first
	if len(hostNames) < 1 {
		return operations.Operation{}, semerr.InvalidInput("at least one host needed")
	}

	// TODO: Check that each host from unique shard && raise InvalidInput if not instead of this
	if len(hostNames) > 1 {
		return operations.Operation{}, semerr.InvalidInput("more than one host isn't supported yet")
	}

	return mg.operator.ModifyOnCluster(ctx, cid, clusters.TypeMongoDB,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, _ clusterslogic.Modifier, cl clusterslogic.Cluster) (operations.Operation, error) {
			currentHosts, err := clusterslogic.ListAllHosts(ctx, reader, cl.ClusterID)
			if err != nil {
				return operations.Operation{}, err
			}

			if len(currentHosts) == 1 {
				return operations.Operation{}, semerr.FailedPrecondition("can not resetup single host in replica set")
			}

			hostMap := make(map[string]hosts.HostExtended, len(currentHosts))
			for _, host := range currentHosts {
				hostMap[host.FQDN] = host
			}

			for _, hostName := range hostNames {
				_, ok := hostMap[hostName]
				if !ok {
					return operations.Operation{}, semerr.InvalidInputf("unknown host: %s", hostName)
				}
			}

			op, err := mg.tasks.CreateTask(
				ctx,
				session,
				tasks.CreateTaskArgs{
					ClusterID:     cid,
					FolderID:      session.FolderCoords.FolderID,
					Auth:          session.Subject,
					TaskType:      mongomodels.TaskTypeResetupHosts,
					OperationType: mongomodels.OperationTypeResetupHosts,
					Metadata: mongomodels.MetadataResetupHosts{
						HostNames: hostNames,
					},
					TaskArgs: map[string]interface{}{
						"host_names": hostNames,
						"resetup_id": strconv.FormatInt(time.Now().Unix(), 16),
					},

					Timeout:  optional.NewDuration(time.Hour * 24 * time.Duration(len(hostNames)) * 6), // TODO: 6 is for worker retries, need to fix someday
					Revision: cl.Revision,
				},
			)
			if err != nil {
				return operations.Operation{}, err
			}

			// TODO: Send event (?)
			return op, nil
		},
	)
}

func (mg *MongoDB) RestartHosts(ctx context.Context, cid string, hostNames []string) (operations.Operation, error) {
	// Check input first
	if mg.cfg.EnvironmentVType != environment.VTypeCompute {
		return operations.Operation{}, semerr.NotImplemented("host restart implemented only for compute envronment yet")
	}
	if len(hostNames) < 1 {
		return operations.Operation{}, semerr.InvalidInput("at least one host needed")
	}

	if len(hostNames) > 1 {
		return operations.Operation{}, semerr.InvalidInput("more than one host isn't supported yet")
	}

	return mg.operator.ModifyOnCluster(ctx, cid, clusters.TypeMongoDB,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, _ clusterslogic.Modifier, cl clusterslogic.Cluster) (operations.Operation, error) {
			currentHosts, err := clusterslogic.ListAllHosts(ctx, reader, cl.ClusterID)
			if err != nil {
				return operations.Operation{}, err
			}

			hostMap := make(map[string]hosts.HostExtended, len(currentHosts))
			for _, host := range currentHosts {
				hostMap[host.FQDN] = host
			}

			for _, hostName := range hostNames {
				_, ok := hostMap[hostName]
				if !ok {
					return operations.Operation{}, semerr.InvalidInputf("unknown host: %s", hostName)
				}
			}
			op, err := mg.tasks.CreateTask(
				ctx,
				session,
				tasks.CreateTaskArgs{
					ClusterID:     cid,
					FolderID:      session.FolderCoords.FolderID,
					Auth:          session.Subject,
					TaskType:      mongomodels.TaskTypeRestartHosts,
					OperationType: mongomodels.OperationTypeRestartHosts,
					Metadata: mongomodels.MetadataRestartHosts{
						HostNames: hostNames,
					},
					TaskArgs: map[string]interface{}{
						"host_names": hostNames,
						"restart_id": strconv.FormatInt(time.Now().Unix(), 16),
					},

					Timeout:  optional.NewDuration(time.Hour * time.Duration(len(hostNames)) * 6), // TODO: 6 is for worker retries, need to fix someday
					Revision: cl.Revision,
				},
			)
			if err != nil {
				return operations.Operation{}, err
			}

			// TODO: Send event (?)
			return op, nil
		},
	)
}

func (mg *MongoDB) StepdownHosts(ctx context.Context, cid string, hostNames []string) (operations.Operation, error) {
	// Check input first
	if len(hostNames) < 1 {
		return operations.Operation{}, semerr.InvalidInput("at least one host needed")
	}

	if len(hostNames) > 1 {
		return operations.Operation{}, semerr.InvalidInput("more than one host isn't supported yet")
	}

	return mg.operator.ModifyOnCluster(ctx, cid, clusters.TypeMongoDB,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, _ clusterslogic.Modifier, cl clusterslogic.Cluster) (operations.Operation, error) {
			currentHosts, err := clusterslogic.ListAllHosts(ctx, reader, cl.ClusterID)
			if err != nil {
				return operations.Operation{}, err
			}

			if len(currentHosts) == 1 {
				return operations.Operation{}, semerr.FailedPrecondition("can not stepdown single host in replica set")
			}

			hostMap := make(map[string]hosts.HostExtended, len(currentHosts))
			for _, host := range currentHosts {
				hostMap[host.FQDN] = host
			}

			for _, hostName := range hostNames {
				_, ok := hostMap[hostName]
				if !ok {
					return operations.Operation{}, semerr.InvalidInputf("unknown host: %s", hostName)
				}
			}
			op, err := mg.tasks.CreateTask(
				ctx,
				session,
				tasks.CreateTaskArgs{
					ClusterID:     cid,
					FolderID:      session.FolderCoords.FolderID,
					Auth:          session.Subject,
					TaskType:      mongomodels.TaskTypeStepdownHosts,
					OperationType: mongomodels.OperationTypeStepdownHosts,
					Metadata: mongomodels.MetadataStepdownHosts{
						HostNames: hostNames,
					},
					TaskArgs: map[string]interface{}{
						"host_names":  hostNames,
						"stepdown_id": strconv.FormatInt(time.Now().Unix(), 16),
					},

					Timeout:  optional.NewDuration(time.Hour * time.Duration(len(hostNames)) * 6), // TODO: 6 is for worker retries, need to fix someday
					Revision: cl.Revision,
				},
			)
			if err != nil {
				return operations.Operation{}, err
			}

			// TODO: Send event (?)
			return op, nil
		},
	)
}
