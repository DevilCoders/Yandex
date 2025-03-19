package transfer

import (
	"context"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/transfer/protos"
)

////////////////////////////////////////////////////////////////////////////////

type Service interface {
	TransferFromImageToDisk(
		ctx context.Context,
		req *protos.TransferFromImageToDiskRequest,
	) (string, error)

	TransferFromSnapshotToDisk(
		ctx context.Context,
		req *protos.TransferFromSnapshotToDiskRequest,
	) (string, error)
}
