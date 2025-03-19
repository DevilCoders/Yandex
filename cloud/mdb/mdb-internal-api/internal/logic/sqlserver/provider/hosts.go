package provider

import (
	"context"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	clusterslogic "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/sessions"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/sqlserver/ssmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/tasks"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (ss *SQLServer) UpdateHosts(ctx context.Context, cid string, specs []ssmodels.UpdateHostSpec) (operations.Operation, error) {
	if len(specs) > 1 {
		return operations.Operation{}, semerr.NotImplemented("updating multiple hosts at once is not supported yet")
	}

	if len(specs) < 1 {
		return operations.Operation{}, semerr.InvalidInput("no hosts to update are specified")
	}

	spec := specs[0]
	return ss.UpdateSQLServerHost(ctx, cid, spec)
}

func (ss *SQLServer) UpdateSQLServerHost(ctx context.Context, cid string, spec ssmodels.UpdateHostSpec) (operations.Operation, error) {
	return ss.operator.ModifyOnNotStoppedCluster(ctx, cid, clusters.TypeSQLServer,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, modifier clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {
			currentHosts, err := clusterslogic.ListAllHosts(ctx, reader, cluster.ClusterID)
			if err != nil {
				return operations.Operation{}, err
			}

			deHosts := clusterslogic.GetHostsWithRole(currentHosts, hosts.RoleSQLServer)

			var host hosts.HostExtended
			var found = false
			for _, h := range deHosts {
				if h.FQDN == spec.HostName {
					host = h
					found = true
					break
				}
			}

			if !found {
				return operations.Operation{}, semerr.InvalidInputf("there is no host with such FQDN: %s", spec.HostName)
			}

			needModifyAssignPublicIP := spec.AssignPublicIP != host.AssignPublicIP

			if needModifyAssignPublicIP {
				err = modifier.ModifyHostPublicIP(ctx, cluster.ClusterID, spec.HostName, cluster.Revision, spec.AssignPublicIP)
				if err != nil {
					return operations.Operation{}, err
				}
			} else {
				return operations.Operation{}, semerr.FailedPrecondition("no changes detected")
			}

			timeout := time.Minute * 3

			hostArgs := make(map[string]interface{})
			taskArgs := make(map[string]interface{})
			hostArgs["fqdn"] = spec.HostName
			taskArgs["host"] = hostArgs
			if needModifyAssignPublicIP {
				taskArgs["include-metadata"] = spec.AssignPublicIP
			}

			op, err := ss.tasks.CreateTask(
				ctx,
				session,
				tasks.CreateTaskArgs{
					ClusterID:     cid,
					FolderID:      session.FolderCoords.FolderID,
					Auth:          session.Subject,
					TaskType:      ssmodels.TaskTypeHostUpdate,
					OperationType: ssmodels.OperationTypeHostUpdate,
					Revision:      cluster.Revision,
					Timeout:       optional.NewDuration(timeout),
					Metadata: ssmodels.MetadataUpdateHosts{
						HostNames: []string{spec.HostName},
					},
					TaskArgs: taskArgs,
				})
			if err != nil {
				return operations.Operation{}, xerrors.Errorf("failed to create operation: %w", err)
			}
			return op, nil
		})
}
