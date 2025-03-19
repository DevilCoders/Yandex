package filesystem

import (
	"context"

	"a.yandex-team.ru/cloud/disk_manager/api/yandex/cloud/priv/disk_manager/v1"
)

////////////////////////////////////////////////////////////////////////////////

type Service interface {
	CreateFilesystem(
		ctx context.Context,
		req *disk_manager.CreateFilesystemRequest,
	) (string, error)

	DeleteFilesystem(
		ctx context.Context,
		req *disk_manager.DeleteFilesystemRequest,
	) (string, error)

	ResizeFilesystem(
		ctx context.Context,
		req *disk_manager.ResizeFilesystemRequest,
	) (string, error)

	DescribeFilesystemModel(
		ctx context.Context,
		req *disk_manager.DescribeFilesystemModelRequest,
	) (*disk_manager.FilesystemModel, error)
}
