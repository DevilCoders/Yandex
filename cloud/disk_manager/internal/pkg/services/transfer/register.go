package transfer

import (
	"context"
	"time"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/clients/nbs"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/clients/snapshot"
	transfer_config "a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/transfer/config"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks"
)

////////////////////////////////////////////////////////////////////////////////

func Register(
	ctx context.Context,
	config *transfer_config.TransferConfig,
	taskRegistry *tasks.Registry,
	snapshotFactory snapshot.Factory,
	nbsFactory nbs.Factory,
) error {

	pingPeriod, err := time.ParseDuration(config.GetSnapshotTaskPingPeriod())
	if err != nil {
		return err
	}

	heartbeatTimeout, err := time.ParseDuration(config.GetSnapshotTaskHeartbeatTimeout())
	if err != nil {
		return err
	}

	backoffTimeout, err := time.ParseDuration(config.GetSnapshotTaskBackoffTimeout())
	if err != nil {
		return err
	}

	err = taskRegistry.Register(ctx, "transfer.TransferFromImageToDisk", func() tasks.Task {
		return &transferFromImageToDiskTask{
			snapshotFactory:          snapshotFactory,
			snapshotPingPeriod:       pingPeriod,
			snapshotHeartbeatTimeout: heartbeatTimeout,
			snapshotBackoffTimeout:   backoffTimeout,
		}
	})
	if err != nil {
		return err
	}

	return taskRegistry.Register(ctx, "transfer.TransferFromSnapshotToDisk", func() tasks.Task {
		return &transferFromSnapshotToDiskTask{
			snapshotFactory:          snapshotFactory,
			snapshotPingPeriod:       pingPeriod,
			snapshotHeartbeatTimeout: heartbeatTimeout,
			snapshotBackoffTimeout:   backoffTimeout,
		}
	})
}
