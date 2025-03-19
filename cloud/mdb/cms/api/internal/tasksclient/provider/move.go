package provider

import (
	"context"
	"fmt"
	"time"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/clusterdiscovery/metadbdiscovery"
	"a.yandex-team.ru/cloud/mdb/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type ResetupAction string

const (
	resetupTaskTypeTemplate = "%s_%s_resetup"

	ResetupActionRestore ResetupAction = "restore"
	ResetupActionReadd   ResetupAction = "readd"
)

func (c *TasksClient) CreateMoveInstanceTask(ctx context.Context, fqdn string, from string) (string, error) {
	txCtx, err := c.internalMeta.Begin(ctx, sqlutil.Primary)
	if err != nil {
		return "", xerrors.Errorf("begin tx: %w", err)
	}
	defer func() { _ = c.internalMeta.Rollback(txCtx) }()

	host, err := c.meta.GetHostByFQDN(txCtx, fqdn)
	if err != nil {
		return "", xerrors.Errorf("host by fqdn: %w", err)
	}

	cluster, err := c.meta.ClusterInfo(txCtx, host.ClusterID)
	if err != nil {
		return "", xerrors.Errorf("cluster by id: %w", err)
	}

	rev, err := c.internalMeta.LockCluster(txCtx, host.ClusterID, "")
	if err != nil {
		return "", xerrors.Errorf("lock cluster: %w", err)
	}

	args, err := c.createDefaultCreateTaskArgs(host.ClusterID, cluster.FolderID, rev)
	if err != nil {
		return "", xerrors.Errorf("create default CreateTaskArgs: %w", err)
	}

	taskType, err := generateResetupTaskType(cluster)
	if err != nil {
		return "", xerrors.Errorf("generate resetup task type: %w", err)
	}

	resetupAction, err := c.generateResetupAction(ctx, host)
	if err != nil {
		return "", xerrors.Errorf("generate resetup action: %w", err)
	}

	args.OperationType = taskType
	args.TaskType = taskType
	args.TaskArgs = map[string]interface{}{
		"fqdn":                  host.FQDN,
		"resetup_action":        resetupAction,
		"preserve_if_possible":  false,
		"ignore_hosts":          make([]string, 0),
		"lock_is_already_taken": true,
		"try_save_disks":        false,
		"cid":                   host.ClusterID,
		"resetup_from":          from,
	}
	args.Timeout = optional.NewDuration(4 * 24 * time.Hour)

	res, err := c.internalMeta.CreateTask(txCtx, args)
	if err != nil {
		return "", xerrors.Errorf("create task: %w", err)
	}

	err = c.internalMeta.CompleteClusterChange(txCtx, host.ClusterID, rev)
	if err != nil {
		return "", xerrors.Errorf("complete cluster change: %w", err)
	}

	err = c.internalMeta.Commit(txCtx)
	if err != nil {
		return "", xerrors.Errorf("commit tx: %w", err)
	}

	return res.OperationID, nil
}

func generateResetupTaskType(info metadb.ClusterInfo) (string, error) {
	var status string

	switch info.Status {
	case metadb.ClusterStatusRunning:
		status = "online"
	case metadb.ClusterStatusStopped:
		status = "offline"
	default:
		knownStatuses := []metadb.ClusterStatus{
			metadb.ClusterStatusRunning,
			metadb.ClusterStatusStopped,
		}
		return "", xerrors.Errorf(
			"can resetup clusters only in statuses %+q, but cluster is %q",
			knownStatuses,
			info.Status,
		)
	}

	return fmt.Sprintf(resetupTaskTypeTemplate, info.CType, status), nil
}

func (c *TasksClient) generateResetupAction(ctx context.Context, host metadb.Host) (ResetupAction, error) {
	discovery := metadbdiscovery.NewMetaDBBasedDiscovery(c.meta)
	hosts, err := discovery.FindInShardOrSubcidByFQDN(ctx, host.FQDN)
	if err != nil {
		return "", xerrors.Errorf("can not discovery hosts: %w", err)
	}

	if len(hosts.Others) == 0 {
		return ResetupActionRestore, nil
	} else {
		return ResetupActionReadd, nil
	}
}
