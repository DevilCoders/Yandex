package tasks

import (
	"context"

	grpc_metadata "google.golang.org/grpc/metadata"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/headers"
)

////////////////////////////////////////////////////////////////////////////////

// TODO: NBS-1858: Avoid collisions with user defined keys (maybe do some kind
// of user keys filtering).
const storageFolderForTasksPinningKey = "storage-folder-for-tasks-pinning"

func getStorageFolderForTasksPinning(ctx context.Context) string {
	metadata, ok := grpc_metadata.FromIncomingContext(ctx)
	if !ok {
		return ""
	}

	vals := metadata.Get(storageFolderForTasksPinningKey)
	if len(vals) == 0 {
		return ""
	}

	return vals[0]
}

func setStorageFolderForTasksPinning(
	ctx context.Context,
	storageFolder string,
) context.Context {

	return headers.Append(
		ctx,
		map[string]string{storageFolderForTasksPinningKey: storageFolder},
	)
}
