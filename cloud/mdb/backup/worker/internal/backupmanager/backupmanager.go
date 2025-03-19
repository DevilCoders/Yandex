package backupmanager

import (
	"context"
	"encoding/json"
	"time"

	"a.yandex-team.ru/cloud/mdb/backup/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/backup/worker/internal/models"
	deploymodels "a.yandex-team.ru/cloud/mdb/deploy/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/internal/encodingutil"
)

type DeployType string

const (
	ModuleDeployType DeployType = "module"
	StateDeployType  DeployType = "state"
)

type BackupManager interface {
	ListBackups(ctx context.Context, bucket, path string) ([]metadb.BackupMetadata, error)
}

func MetadataByID(ml []metadb.BackupMetadata, backupID string) (metadb.BackupMetadata, bool) {
	for i := range ml {
		if ml[i].ID() == backupID {
			return ml[i], true
		}
	}
	return nil, false
}

type StorageBackup struct {
	cid        string
	subcluster string
	shard      string
	startedAt  time.Time
	finishedAt time.Time
	initiator  metadb.BackupInitiator
	method     metadb.BackupMethod
	meta       metadb.BackupMetadata
}

// DeployCmd represents the deployment command with deploy type
type DeployCmd struct {
	DeployType DeployType
	deploymodels.CommandDef
}

func (cmd *DeployCmd) AddArgs(args ...string) {
	cmd.Args = append(cmd.Args, args...)
}

func NewDeployCmd(deployType DeployType, cmdType string, args []string, timeout encodingutil.Duration) DeployCmd {
	return DeployCmd{
		DeployType: deployType,
		CommandDef: deploymodels.CommandDef{
			Type:    cmdType,
			Args:    args,
			Timeout: timeout,
		},
	}
}

// DeployArgsProvider returns deploy job arguments in db specific form
type DeployArgsProvider interface {
	CreationArgsFromJob(ctx context.Context, job models.BackupJob, deployType DeployType) ([]string, error)
	DeletionArgsFromJob(ctx context.Context, job models.BackupJob) ([]string, error)
}

// PillarToArgs converts pillar to deploy job arguments
func PillarToArgs(pillar interface{}) ([]string, error) {
	pillarBytes, err := json.Marshal(pillar)
	if err != nil {
		return nil, err
	}

	return []string{"pillar=" + string(pillarBytes)}, nil
}

// JobFactsExtractor provides functions for obtaining artifacts of completed backups
type JobFactsExtractor interface {
	BackupPathFromJob(ctx context.Context, job models.BackupJob) (string, error)
	BackupNameRootFromJob(ctx context.Context, job models.BackupJob) (string, string, error)
}

type BackupSize struct {
	ID          string
	DataSize    int64
	JournalSize int64
}

type SizeMeasurer interface {
	SizesAfterCreated(ctx context.Context, job models.BackupJob, createdBackupMeta metadb.BackupMetadata, bucket string) ([]BackupSize, error)
	SizesAfterDeleted(ctx context.Context, job models.BackupJob, bucket string) ([]BackupSize, error)
}
