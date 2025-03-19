package test

import (
	"context"
	"fmt"
	"os"

	snapshot_config "a.yandex-team.ru/cloud/disk_manager/internal/pkg/dataplane/snapshot/config"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/logging"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/persistence"
)

////////////////////////////////////////////////////////////////////////////////

func NewS3Client() (*persistence.S3Client, error) {
	endpoint := fmt.Sprintf("http://localhost:%s", os.Getenv("S3MDS_PORT"))
	credentials := persistence.NewS3Credentials("test", "test")
	return persistence.NewS3Client(endpoint, "test", credentials)
}

func NewContext() context.Context {
	return logging.SetLogger(
		context.Background(),
		logging.CreateStderrLogger(logging.DebugLevel),
	)
}

func NewS3Key(config *snapshot_config.SnapshotConfig, chunkID string) string {
	return fmt.Sprintf(
		"%v/%v",
		config.GetChunkBlobsS3KeyPrefix(),
		chunkID,
	)
}
