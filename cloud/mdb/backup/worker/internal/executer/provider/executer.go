package provider

import (
	"context"
	"fmt"
	"time"

	"a.yandex-team.ru/cloud/mdb/backup/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/backup/worker/internal/backupmanager"
	"a.yandex-team.ru/cloud/mdb/backup/worker/internal/executer"
	"a.yandex-team.ru/cloud/mdb/backup/worker/internal/hostpicker"
	"a.yandex-team.ru/cloud/mdb/backup/worker/internal/models"
	"a.yandex-team.ru/cloud/mdb/deploy/api/pkg/deployapi"
	deploymodels "a.yandex-team.ru/cloud/mdb/deploy/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/internal/encodingutil"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/requestid"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type Executer struct {
	mdb               metadb.MetaDB
	lg                log.Logger
	deploy            deployapi.Client
	hostPicker        hostpicker.HostPicker
	bm                backupmanager.BackupManager
	argsProvider      backupmanager.DeployArgsProvider
	artifactExctactor backupmanager.JobFactsExtractor
	bu                backupmanager.SizeMeasurer
	cfg               Config
	clusterType       metadb.ClusterType
}

func NewExecuter(
	mdb metadb.MetaDB,
	lg log.Logger,
	deploy deployapi.Client,
	hostPicker hostpicker.HostPicker,
	bm backupmanager.BackupManager,
	bu backupmanager.SizeMeasurer,
	argsProvider backupmanager.DeployArgsProvider,
	artifactExctactor backupmanager.JobFactsExtractor,
	clusterType metadb.ClusterType,
	cfg Config) (*Executer, error) {
	if (cfg.Creating.UpdateSizes || cfg.Deleting.UpdateSizes) && bu == nil {
		return nil, xerrors.Errorf("size updater expected with given config: %+v", cfg)
	}

	return &Executer{
		mdb:               mdb,
		lg:                lg,
		deploy:            deploy,
		hostPicker:        hostPicker,
		bm:                bm,
		bu:                bu,
		argsProvider:      argsProvider,
		artifactExctactor: artifactExctactor,
		clusterType:       clusterType,
		cfg:               cfg,
	}, nil
}

func (ex *Executer) StartCreation(ctx context.Context, job models.BackupJob) (results []executer.Resulter) {
	result := executer.BackupCreationStart{}

	// check that cluster is currently in the appropriate status to run a backup shipment
	ok, err := ex.mdb.ClusterStatusIsIn(job.Ctx, job.ClusterID, metadb.ApplicableClusterStatuses)
	if err != nil {
		result.Err = metadb.WrapTempError(err, "failed to check cluster status")
		return []executer.Resulter{result}
	}
	if !ok {
		result.Err = metadb.WrapTempError(err, "cluster is not in the correct state to run backup")
		return []executer.Resulter{result}
	}

	cfg := ex.cfg.Planned
	creationCmd := backupmanager.NewDeployCmd(cfg.DeployType, cfg.CmdConfig.Type, cfg.CmdConfig.Args, cfg.CmdConfig.Timeout)

	args, err := ex.argsProvider.CreationArgsFromJob(ctx, job, cfg.DeployType)
	if err != nil {
		result.Err = metadb.WrapPermError(err, "failed to get job args")
		return []executer.Resulter{result}
	}

	creationCmd.AddArgs(args...)
	result.ShipmentID, result.Err = ex.runShipmentOnPickedHost(ctx, job, creationCmd, cfg.SyncAllStateTimeout)

	return []executer.Resulter{result}
}

func (ex *Executer) CompleteCreating(ctx context.Context, job models.BackupJob) []executer.Resulter {
	creationResult := executer.BackupCreationComplete{}
	cfg := ex.cfg.Creating

	if err := ex.hasDeployCompleted(ctx, job, cfg.CheckDeployTimeout.Duration); err != nil {
		creationResult.Err = err
		return []executer.Resulter{creationResult}
	}

	bucket, err := ex.mdb.ClusterBucket(job.Ctx, job.ClusterID)
	if err != nil {
		creationResult.Err = metadb.WrapTempError(err, "can not get cluster bucket")
		return []executer.Resulter{creationResult}
	}
	ctxlog.Debugf(ctx, ex.lg, "retrieved cluster bucket %q", bucket)

	storageBackups, err := ex.listStorageBackupsMetadata(ctx, job, bucket)
	if err != nil {
		creationResult.Err = err
		return []executer.Resulter{creationResult}
	}

	if len(storageBackups) < 1 {
		creationResult.Err = metadb.StorageBackupsNotFound
		return []executer.Resulter{creationResult}
	}
	backupMeta, ok := backupmanager.MetadataByID(storageBackups, job.BackupID)
	if !ok {
		creationResult.Err = metadb.StorageCreatedBackupNotFound
		return []executer.Resulter{creationResult}
	}
	creationResult.Metadata = backupMeta

	var sizes []backupmanager.BackupSize
	if ex.cfg.Creating.UpdateSizes {
		sizes, err = ex.bu.SizesAfterCreated(ctx, job, backupMeta, bucket)
		if err != nil {
			ctxlog.Error(ctx, ex.lg, "got error during calculation of backup sizes", log.Error(err))
			if ex.cfg.Creating.AssertUpdateSizesErrors {
				creationResult.Err = metadb.WrapTempError(err, "size update after creation")
			}
		}
		ctxlog.Debugf(ctx, ex.lg, "calculated new backup sizes: %+v", sizes)
	}

	results := []executer.Resulter{creationResult}
	for _, s := range sizes {
		results = append(results, executer.BackupResize(s))
	}
	return results
}

func (ex *Executer) StartDeletion(ctx context.Context, job models.BackupJob) []executer.Resulter {
	result := executer.BackupDeletionStart{}
	cfg := ex.cfg.Obsolete
	cmd := backupmanager.NewDeployCmd(cfg.DeployType, cfg.CmdConfig.Type, cfg.CmdConfig.Args,
		cfg.CmdConfig.Timeout)

	args, err := ex.argsProvider.DeletionArgsFromJob(ctx, job)
	if err != nil {
		result.Err = metadb.WrapPermError(err, "can not marshal deploy pillar")
		return []executer.Resulter{result}
	}

	cmd.AddArgs(args...)
	result.ShipmentID, result.Err = ex.runShipmentOnPickedHost(ctx, job, cmd, cfg.SyncAllStateTimeout)

	return []executer.Resulter{result}
}

func (ex *Executer) CompleteDeleting(ctx context.Context, job models.BackupJob) []executer.Resulter {
	deletionResult := executer.BackupDeletionComplete{}
	cfg := ex.cfg.Deleting

	if err := ex.hasDeployCompleted(ctx, job, cfg.CheckDeployTimeout.Duration); err != nil {
		deletionResult.Err = err
		return []executer.Resulter{deletionResult}
	}

	bucket, err := ex.mdb.ClusterBucket(job.Ctx, job.ClusterID)
	if err != nil {
		deletionResult.Err = metadb.WrapTempError(err, "cluster bucket")
		return []executer.Resulter{deletionResult}
	}
	ctxlog.Debugf(ctx, ex.lg, "retrieved cluster bucket %q", bucket)

	storageBackupsMetadata, err := ex.listStorageBackupsMetadata(ctx, job, bucket)
	if err != nil {
		deletionResult.Err = err
		return []executer.Resulter{deletionResult}
	}

	if _, ok := backupmanager.MetadataByID(storageBackupsMetadata, job.BackupID); ok {
		deletionResult.Err = metadb.StorageDeletedBackupFound
		return []executer.Resulter{deletionResult}
	}

	var sizes []backupmanager.BackupSize
	if ex.cfg.Deleting.UpdateSizes {
		sizes, err = ex.bu.SizesAfterDeleted(ctx, job, bucket)
		if err != nil {
			ctxlog.Error(ctx, ex.lg, "got error during calculation of backup sizes", log.Error(err))
			if ex.cfg.Deleting.AssertUpdateSizesErrors {
				deletionResult.Err = metadb.WrapTempError(err, "size update after deletion")
			}
		}
		ctxlog.Debugf(ctx, ex.lg, "calculated new backup sizes: %+v", sizes)
	}

	results := []executer.Resulter{deletionResult}
	for _, s := range sizes {
		results = append(results, executer.BackupResize(s))
	}
	return results
}

func (ex *Executer) listStorageBackupsMetadata(ctx context.Context, job models.BackupJob, bucket string) ([]metadb.BackupMetadata, error) {
	path, err := ex.artifactExctactor.BackupPathFromJob(ctx, job)
	if err != nil {
		return nil, metadb.WrapTempError(err, "can not get cluster bucket path")
	}
	backups, err := ex.bm.ListBackups(ctx, bucket, path)
	if err != nil {
		return nil, metadb.WrapTempError(err, "can not list s3")
	}
	ctxlog.Debugf(ctx, ex.lg, "listed %d backups", len(backups))

	return backups, nil
}

func (ex *Executer) hasDeployCompleted(ctx context.Context, job models.BackupJob, checkDeployTimeout time.Duration) error {
	shipID, err := models.ShipmentIDFromJob(job)
	if err != nil {
		return metadb.WrapPermError(err, "unexpected shipmentID")
	}

	deployCtx, cancel := context.WithTimeout(ctx, checkDeployTimeout)
	defer cancel()

	return AssertShipmentCompleted(deployCtx, shipID, ex.deploy)
}

func (ex *Executer) runShipmentOnPickedHost(ctx context.Context, job models.BackupJob, deployCmd backupmanager.DeployCmd,
	syncAllTimeout encodingutil.Duration) (string, error) {
	hosts, err := ex.mdb.ListHosts(ctx, job.ClusterID, optional.NewString(job.SubClusterID), job.ShardID)
	if err != nil {
		return "", metadb.WrapTempError(err, "can not list cluster hosts")
	}
	if len(hosts) < 1 {
		return "", metadb.NewError("no hosts were found to run shipment", false)
	}

	fqdn, err := ex.hostPicker.PickHost(ctx, metadb.FqdnsFromHosts(hosts))
	if err != nil {
		return "", metadb.WrapTempError(err, "can not pick host to deploy")
	}
	cluster, err := ex.mdb.Cluster(ctx, job.ClusterID)
	if err != nil {
		return "", metadb.WrapTempError(err, "can not load cluster")
	}
	if deployCmd.DeployType == backupmanager.StateDeployType {
		deployCmd.AddArgs("saltenv=" + string(cluster.Environment))
	}
	shipmentID, requestID, err := CreateShipmentWithSyncAll(
		job.Ctx,
		deployCmd,
		[]string{fqdn},
		syncAllTimeout,
		ex.deploy)

	if err != nil {
		return "", metadb.WrapTempError(err, "can not start deploy")
	}
	ctxlog.Infof(job.Ctx, ex.lg, "created shipment %d on %+v (Request-ID: %s)", shipmentID, fqdn, requestID)

	return shipmentID.String(), nil
}

func CreateShipment(ctx context.Context, args []string, hosts []string, timeout encodingutil.Duration, deploy deployapi.Client) (deploymodels.ShipmentID, string, error) {
	commands := []deploymodels.CommandDef{{
		Type:    "state.sls",
		Args:    args,
		Timeout: timeout,
	}}

	return createShipment(ctx, commands, hosts, deploy)
}

func CreateShipmentWithSyncAll(ctx context.Context, deployCmd backupmanager.DeployCmd, hosts []string,
	syncAllTimeout encodingutil.Duration, deploy deployapi.Client) (deploymodels.ShipmentID, string, error) {
	syncAllCommand := deploymodels.CommandDef{
		Type:    "saltutil.sync_all",
		Args:    []string{},
		Timeout: syncAllTimeout,
	}

	return createShipment(ctx, []deploymodels.CommandDef{syncAllCommand, deployCmd.CommandDef}, hosts, deploy)
}

func createShipment(ctx context.Context, commands []deploymodels.CommandDef, hosts []string, deploy deployapi.Client) (deploymodels.ShipmentID, string, error) {
	if len(hosts) < 1 {
		return 0, "", metadb.NewError("empty host list", true)
	}

	requestID := requestid.New()

	totalTimeout := time.Duration(0)

	for _, cmd := range commands {
		totalTimeout += cmd.Timeout.Duration
	}

	shipment, err := deploy.CreateShipment(
		requestid.WithRequestID(ctx, requestID),
		hosts,
		commands,
		1,
		1,
		totalTimeout,
	)

	if err != nil {
		return 0, "", metadb.WrapTempError(err, "can not start deploy: %w")
	}

	return shipment.ID, requestID, nil
}

func AssertShipmentCompleted(ctx context.Context, shipmentID deploymodels.ShipmentID, deploy deployapi.Client) error {
	shipment, err := deploy.GetShipment(ctx, shipmentID)
	if err != nil {
		return metadb.WrapTempError(err, "unable to get shipment")
	}

	if shipment.Status == deploymodels.ShipmentStatusInProgress {
		return metadb.DeployInProgressError
	}

	if shipment.Status != deploymodels.ShipmentStatusDone {
		return metadb.NewError(fmt.Sprintf("shipment %d is in unexpected status: %s", shipment.ID, shipment.Status), false)
	}

	return nil
}
