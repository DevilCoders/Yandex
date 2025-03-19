package provider

import (
	"context"
	"encoding/json"
	"fmt"
	"io/ioutil"
	"path"
	"path/filepath"
	"sort"
	"strconv"
	"strings"

	"golang.org/x/exp/slices"

	"a.yandex-team.ru/cloud/mdb/backup/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/backup/internal/tools/walg"
	"a.yandex-team.ru/cloud/mdb/backup/internal/tools/walg/postgresql"
	"a.yandex-team.ru/cloud/mdb/backup/worker/internal/backupmanager"
	"a.yandex-team.ru/cloud/mdb/backup/worker/internal/models"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/s3"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/ptr"
)

const (
	pgMetadataFilename = "metadata.json"
	s3Scheme           = "s3://"
	bigMetaFileSize    = 16 * 1024 * 1024
)

type PostgreSQLS3BackupManager struct {
	s3 s3.Client
	lg log.Logger
}

func NewPostgreSQLBackupManager(s3 s3.Client, lg log.Logger) *PostgreSQLS3BackupManager {
	return &PostgreSQLS3BackupManager{s3: s3, lg: lg}
}

func getPostgreSQLWalgRootPath(cid string, majorVersion int) string {
	return fmt.Sprintf("wal-e/%s/%d/", cid, majorVersion*100)
}

func GetPostgreSQLWalgBackupsPath(cid string) string {
	return fmt.Sprintf("wal-e/%s/", cid)
}

func metadataPathFromSentinelPath(sPath string) string {
	return path.Join(strings.TrimSuffix(sPath, walg.WalgSentinelSuffix), pgMetadataFilename)
}

func (pgbm *PostgreSQLS3BackupManager) ListBackups(ctx context.Context, bucket, prefixPath string) ([]metadb.BackupMetadata, error) {
	delimiter := "/"
	objects, _, err := pgbm.s3.ListObjects(ctx, bucket, s3.ListObjectsOpts{Prefix: ptr.String(prefixPath), Delimiter: &delimiter})
	if err != nil {
		return nil, err
	}
	backups := make([]metadb.BackupMetadata, 0, len(objects))

	for _, obj := range objects {
		// if backup does not have a sentinel file, it is considered invalid,
		// we should ignore such backups
		if !strings.HasSuffix(obj.Key, walg.WalgSentinelSuffix) {
			continue
		}
		if obj.Size > bigMetaFileSize {
			ctxlog.Debugf(ctx, pgbm.lg, "big sentinel file found %s (%d bytes)", obj.Key, obj.Size)
		}
		backup, err := pgbm.metadataFromS3Object(ctx, bucket, obj)
		if err != nil {
			return nil, err
		}

		backups = append(backups, backup)
	}
	return backups, nil
}

func (pgbm *PostgreSQLS3BackupManager) ListBackupsAllVersions(ctx context.Context, bucket, prefixPath string) ([]metadb.BackupMetadata, error) {
	delimiter := "/"
	_, folders, err := pgbm.s3.ListObjects(ctx, bucket, s3.ListObjectsOpts{Prefix: ptr.String(prefixPath), Delimiter: &delimiter})
	if err != nil {
		return nil, err
	}

	if len(folders) > 1 {
		pgbm.lg.Debugf("Found %d possible root paths at prefix %s: %v", len(folders), prefixPath, folders)
	}
	result := make([]metadb.BackupMetadata, 0)
	for i := range folders {
		backups, err := pgbm.ListBackups(ctx, bucket, walg.WalgBackupsPathFromRootPath(folders[i].Prefix))
		if err != nil {
			return nil, err
		}

		result = append(result, backups...)
	}

	return result, nil
}

func (pgbm *PostgreSQLS3BackupManager) ListWals(ctx context.Context, bucket, prefixPath string) ([]postgresql.WalSegment, error) {
	objects, _, err := pgbm.s3.ListObjects(ctx, bucket, s3.ListObjectsOpts{Prefix: ptr.String(prefixPath), Delimiter: ptr.String("/")})
	if err != nil {
		return nil, err
	}
	wals := make([]postgresql.WalSegment, 0, len(objects))
	for _, obj := range objects {
		basename := strings.TrimSuffix(filepath.Base(obj.Key), filepath.Ext(obj.Key))
		if len(basename) != postgresql.WalSegmentNameSize {
			continue
		}
		wal, err := postgresql.NewWalSegmentFromFilename(basename, obj.Size)
		if err != nil {
			pgbm.lg.Warn("bad wal filename", log.Error(err))
			continue
		}
		wals = append(wals, wal)
	}
	return wals, nil
}

type ListWalsFunc func(ctx context.Context, bucket, prefixPath string) ([]postgresql.WalSegment, error)

func FilterWalsByRange(wals []postgresql.WalSegment, r Range, exclude []Range) []postgresql.WalSegment {
	var filtered []postgresql.WalSegment
	var w postgresql.WalSegment
	for i := range wals {
		w = wals[i]
		if IsLSNInRanges(w.StartLSN, exclude) || !r.In(w.StartLSN) {
			continue
		}
		filtered = append(filtered, w)
	}
	return filtered
}

func ListWalsByRange(ctx context.Context, listFunc ListWalsFunc, path, bucket string, r Range, exclude []Range, lg log.Logger) ([]postgresql.WalSegment, error) {
	prefixes := postgresql.CommonPrefixesFromTimelineLsn(r.Timelines(), r.Begin.LSN, r.End.LSN)
	ctxlog.Debugf(ctx, lg, "generated wal prefixes from {timelines: %+v, beginLSN: %d, endLSN: %d}: %+v", r.Timelines(), r.Begin.LSN, r.End.LSN, prefixes)
	wals, err := ListWalsByPrefixes(ctx, listFunc, prefixes, path, bucket)
	if err != nil {
		return nil, xerrors.Errorf("list wals by prefixes: %w", err)
	}
	filteredWals := FilterWalsByRange(wals, r, exclude)
	ctxlog.Debugf(ctx, lg, "fetched %d wals by range (%+v) and exclude (%+v)", len(filteredWals), r, exclude)
	return filteredWals, nil
}

func ListWalsByPrefixes(ctx context.Context, listFunc ListWalsFunc, prefixes []string, rootPath, bucket string) ([]postgresql.WalSegment, error) {
	var wals []postgresql.WalSegment
	for _, prefix := range prefixes {
		batch, err := listFunc(ctx, bucket, path.Join(rootPath, prefix))
		if err != nil {
			return nil, xerrors.Errorf("list journals: %w", err)
		}
		wals = append(wals, batch...)
	}
	return wals, nil
}

func UnmarshalBackupSentinelDto(raw []byte) (postgresql.BackupSentinelDto, error) {
	var sentinel postgresql.BackupSentinelDto
	if err := json.Unmarshal(raw, &sentinel); err != nil {
		return postgresql.BackupSentinelDto{}, err
	}
	return sentinel, nil
}

func (pgbm *PostgreSQLS3BackupManager) metadataFromS3Object(ctx context.Context, bucket string, obj s3.Object) (postgresql.PostgreSQLMetadata, error) {
	// In order to init the PostgreSQL metadata from the existing S3 backup we need two files from S3:
	// 1. (Metadata) metadata.json (generic info about the backup)
	// 2. (Sentinel) ****_backup_stop_sentinel.json (currently we need this to obtain the incremental backup info,
	// because it is not stored in metadata.json)

	metaPath := metadataPathFromSentinelPath(obj.Key)
	metadata, rawMeta, err := fetchExtendedMetadataDtoFromS3(ctx, pgbm.s3, bucket, metaPath)
	if err != nil {
		return postgresql.PostgreSQLMetadata{}, xerrors.Errorf("fetch metadata.json from S3: %w", err)
	}

	sentinel, err := fetchBackupSentinelDtoFromS3(ctx, pgbm.s3, bucket, obj.Key)
	if err != nil {
		return postgresql.PostgreSQLMetadata{}, xerrors.Errorf("fetch backup_stop_sentinel.json from S3: %w", err)
	}

	rootPath, backupName, err := walg.ParseWalgSentinelPath(obj.Key)
	if err != nil {
		return postgresql.PostgreSQLMetadata{}, err
	}

	return postgresql.NewPostgreSQLMetadata(backupName, rootPath, metadata, rawMeta, sentinel)
}

func fetchExtendedMetadataDtoFromS3(ctx context.Context, s3 s3.Client, bucket, metaPath string) (postgresql.ExtendedMetadataDto, []byte, error) {
	rMeta, err := s3.GetObject(ctx, bucket, metaPath)
	if err != nil {
		return postgresql.ExtendedMetadataDto{}, nil, err
	}
	defer rMeta.Close()

	rawMeta, err := ioutil.ReadAll(rMeta)
	if err != nil {
		return postgresql.ExtendedMetadataDto{}, nil, err
	}

	metadata, err := postgresql.UnmarshalExtendedMetadataDto(rawMeta)
	if err != nil {
		return postgresql.ExtendedMetadataDto{}, nil, err
	}
	return metadata, rawMeta, nil
}

func fetchBackupSentinelDtoFromS3(ctx context.Context, s3 s3.Client, bucket, sentinelPath string) (postgresql.BackupSentinelDto, error) {
	rSentinel, err := s3.GetObject(ctx, bucket, sentinelPath)
	if err != nil {
		return postgresql.BackupSentinelDto{}, err
	}
	defer rSentinel.Close()

	rawSentinel, err := ioutil.ReadAll(rSentinel)
	if err != nil {
		return postgresql.BackupSentinelDto{}, err
	}

	sentinel, err := UnmarshalBackupSentinelDto(rawSentinel)
	if err != nil {
		return postgresql.BackupSentinelDto{}, err
	}

	return sentinel, nil
}

func PostgresMajorVersion(ctx context.Context, mdb metadb.MetaDB, clusterID string) (int, error) {
	versionByComponent, err := mdb.ClusterVersions(ctx, clusterID)
	if err != nil {
		return 0, xerrors.Errorf("Failed to get cluster versions: %w", err)
	}
	postgresVersions, ok := versionByComponent["postgres"]
	if !ok {
		return 0, xerrors.Errorf("There is no entry for postgres component version in cluster versions")
	}
	majorVersionNum, err := strconv.Atoi(postgresVersions.MajorVersion)
	if err != nil {
		return 0, xerrors.Errorf("Failed to parse postgres major version number: %w", err)
	}
	return majorVersionNum, nil
}

type PostgreSQLJobFactsExtractor struct {
	mdb metadb.MetaDB
}

func NewPostgreSQLJobFactsExtractor(mdb metadb.MetaDB) *PostgreSQLJobFactsExtractor {
	return &PostgreSQLJobFactsExtractor{
		mdb: mdb,
	}
}

func (pgde *PostgreSQLJobFactsExtractor) BackupPathFromJob(ctx context.Context, job models.BackupJob) (string, error) {
	rootPath, err := pgde.rootPathFromJob(ctx, job)
	if err != nil {
		return "", err
	}

	return walg.WalgBackupsPathFromRootPath(rootPath), nil
}

func (pgde *PostgreSQLJobFactsExtractor) rootPathFromJob(ctx context.Context, job models.BackupJob) (string, error) {
	meta, err := postgresql.UnmarshalPostgreSQLMetadata(job.Metadata)
	if err == nil {
		return meta.RootPath, nil
	}

	pgVersion, err := PostgresMajorVersion(ctx, pgde.mdb, job.ClusterID)
	if err != nil {
		return "", metadb.WrapTempError(err, "can not get Postgres major version")
	}
	return getPostgreSQLWalgRootPath(job.ClusterID, pgVersion), nil
}

func (pgde *PostgreSQLJobFactsExtractor) BackupNameRootFromJob(ctx context.Context, job models.BackupJob) (string, string, error) {
	meta, err := postgresql.UnmarshalPostgreSQLMetadata(job.Metadata)
	if err != nil {
		return "", "", xerrors.Errorf("can not unmarshal postgresql metadata with error: %w", err)
	}
	return meta.BackupName, meta.RootPath, nil
}

type PostgreSQLDeployArgsProvider struct {
	mdb       metadb.MetaDB
	lg        log.Logger
	factsExtr *PostgreSQLJobFactsExtractor
}

func NewPostgreSQLDeployArgsProvider(mdb metadb.MetaDB, lg log.Logger,
	factsExtr *PostgreSQLJobFactsExtractor) *PostgreSQLDeployArgsProvider {
	return &PostgreSQLDeployArgsProvider{mdb: mdb, lg: lg, factsExtr: factsExtr}
}

type PostgreSQLCreationArgs struct {
	BackupID          string `json:"backup_id"`
	Full              bool   `json:"full_backup,omitempty"`
	BackupIsPermanent bool   `json:"user_backup,omitempty"`
	DeltaFromID       string `json:"from_backup_id,omitempty"`
	DeltaFromName     string `json:"from_backup_name,omitempty"`
}

func (a *PostgreSQLCreationArgs) Marshal() []string {
	args := []string{
		fmt.Sprintf("full_backup=%t", a.Full),
		fmt.Sprintf("user_backup=%t", a.BackupIsPermanent),
	}
	if a.BackupID != "" {
		args = append(args, fmt.Sprintf("backup_id=%s", a.BackupID))
	}
	if a.DeltaFromID != "" {
		args = append(args, fmt.Sprintf("from_backup_id=%s", a.DeltaFromID))
	}
	if a.DeltaFromName != "" {
		args = append(args, fmt.Sprintf("from_backup_name=%s", a.DeltaFromName))
	}
	return args
}

type PostgreSQLDeletionPillar struct {
	BackupID   string `json:"backup_id"`
	BackupName string `json:"backup_name"`
	S3Prefix   string `json:"s3_prefix"`
}

func (pgdp *PostgreSQLDeployArgsProvider) CreationArgsFromJob(ctx context.Context, job models.BackupJob, deployType backupmanager.DeployType) ([]string, error) {
	bs, err := pgdp.mdb.ListParentBackups(ctx, job.BackupID)
	if err != nil {
		return nil, err
	}

	if len(bs) > 1 {
		return nil, xerrors.New("can not create delta backup from multiple base backups")
	}
	args := PostgreSQLCreationArgs{
		BackupID:          job.BackupID,
		BackupIsPermanent: job.Initiator == metadb.BackupInitiatorUser,
	}

	if len(bs) == 1 {
		parentBackup := bs[0]

		parsedMeta, err := postgresql.UnmarshalPostgreSQLMetadata(parentBackup.Metadata)
		if err != nil {
			return nil, xerrors.Errorf("can not unmarshal postgresql metadata with error: %w", err)
		}

		args.DeltaFromID = parsedMeta.BackupID
		args.DeltaFromName = parsedMeta.BackupName

	} else {
		args.Full = true
	}

	if deployType == backupmanager.StateDeployType {
		return backupmanager.PillarToArgs(args)
	}

	return args.Marshal(), nil
}

func (pgdp *PostgreSQLDeployArgsProvider) DeletionArgsFromJob(ctx context.Context, job models.BackupJob) ([]string, error) {
	name, rootPath, err := pgdp.factsExtr.BackupNameRootFromJob(ctx, job)
	if err != nil {
		return nil, err
	}

	bucket, err := pgdp.mdb.ClusterBucket(ctx, job.ClusterID)
	if err != nil {
		return nil, xerrors.Errorf("can not get cluster bucket: %w", err)
	}

	pillar := PostgreSQLDeletionPillar{
		BackupID:   job.BackupID,
		BackupName: name,
		S3Prefix:   s3Scheme + path.Join(bucket, rootPath),
	}
	return backupmanager.PillarToArgs(pillar)
}

func IsLSNInRanges(lsn uint64, ranges []Range) bool {
	for _, r := range ranges {
		if r.In(lsn) {
			return true
		}
	}
	return false
}

func FullPGBackupsFromBackups(backups []metadb.Backup) ([]FullPostgreSQLBackup, error) {
	pgbackups := make([]FullPostgreSQLBackup, 0, len(backups))
	for _, b := range backups {
		pgb, err := NewFullPostgresqlBackup(b)
		if err != nil {
			return nil, xerrors.Errorf("unmarshal full postgres metadata: %w", err)
		}
		pgbackups = append(pgbackups, pgb)
	}
	return pgbackups, nil
}

func RangesFromPGMetas(metas []FullPostgreSQLBackup) ([]Range, error) {
	ranges := make([]Range, 0, len(metas))
	for _, m := range metas {
		r, err := RangeFromBackupMeta(m.ParsedMetadata.BackupName, m.ParsedMetadata.ParsedDtoMeta)
		if err != nil {
			return nil, xerrors.Errorf("range from pg meta: %w", err)
		}
		ranges = append(ranges, r)
	}
	return ranges, nil
}

func RangeFromBackupMeta(backupName string, dtoMeta postgresql.ExtendedMetadataDto) (Range, error) {
	timelineID, startLSN, finishLSN, err := PgInfoFromBackupMeta(backupName, dtoMeta)
	if err != nil {
		return Range{}, err
	}
	return Range{Begin: Point{timelineID, startLSN}, End: Point{timelineID, finishLSN}}, nil
}

func PgInfoFromBackupMeta(backupName string, dtoMeta postgresql.ExtendedMetadataDto) (timelineID uint32, startLSN, finishLSN uint64, err error) {
	timelineID, err = postgresql.ParseTimelineFromBackupName(backupName)
	if err != nil {
		return 0, 0, 0, xerrors.Errorf("parse backup filename: %w", err)
	}

	return timelineID, dtoMeta.StartLsn, dtoMeta.FinishLsn, nil
}

type Point struct {
	Timeline uint32
	LSN      uint64
}

type Range struct {
	Begin Point
	End   Point
}

func (r Range) Timelines() []uint32 {
	timelines := make([]uint32, 0, r.End.Timeline-r.Begin.Timeline+1)
	for i := r.Begin.Timeline; i <= r.End.Timeline; i++ {
		timelines = append(timelines, i)
	}
	return timelines
}

func (r Range) In(lsn uint64) bool {
	return lsn >= r.Begin.LSN && lsn <= r.End.LSN
}

func RangeForAutoBackup(sortedBackups []FullPostgreSQLBackup, backupID string) (Range, []Range, error) {
	reqAutoIdx, intermManualIdx, nextAutoIdx, err := NextAutoBackupIndexes(sortedBackups, backupID)
	if err != nil {
		return Range{}, nil, xerrors.Errorf("find update range: %w", err)
	}

	intermManual := make([]FullPostgreSQLBackup, len(intermManualIdx))
	for i, j := range intermManualIdx {
		intermManual[i] = sortedBackups[j]
	}
	reqAuto := sortedBackups[reqAutoIdx]
	reqAutoRange, err := RangeFromBackupMeta(reqAuto.ParsedMetadata.BackupName, reqAuto.ParsedMetadata.ParsedDtoMeta)
	if err != nil {
		return Range{}, nil, err
	}
	if nextAutoIdx == -1 {
		return reqAutoRange, nil, nil
	}

	nextAuto := sortedBackups[nextAutoIdx]

	nextAutoRange, err := RangeFromBackupMeta(nextAuto.ParsedMetadata.BackupName, nextAuto.ParsedMetadata.ParsedDtoMeta)
	if err != nil {
		return Range{}, nil, err
	}

	excludeRanges, err := RangesFromPGMetas(intermManual)
	if err != nil {
		return Range{}, nil, err
	}

	return Range{Begin: reqAutoRange.Begin, End: nextAutoRange.Begin}, excludeRanges, nil

}

type FullPostgreSQLBackup struct {
	metadb.Backup
	ParsedMetadata FullPostgreSQLMetadata
}

func NewFullPostgresqlBackup(b metadb.Backup) (FullPostgreSQLBackup, error) {
	meta, err := UnmarshalFullPostgreSQLMetadata(b.Metadata)
	if err != nil {
		return FullPostgreSQLBackup{}, err
	}
	return FullPostgreSQLBackup{b, meta}, nil
}

func (b FullPostgreSQLBackup) ID() string {
	return b.Backup.BackupID
}

func (b FullPostgreSQLBackup) Initiator() metadb.BackupInitiator {
	return b.Backup.Initiator
}

type FullPostgreSQLMetadata struct {
	postgresql.PostgreSQLMetadata
	ParsedDtoMeta postgresql.ExtendedMetadataDto
}

func UnmarshalFullPostgreSQLMetadata(raw []byte) (FullPostgreSQLMetadata, error) {
	meta, err := postgresql.UnmarshalPostgreSQLMetadata(raw)
	if err != nil {
		return FullPostgreSQLMetadata{}, xerrors.Errorf("unmarshal postgres metadata: %w", err)
	}
	metaDto, err := postgresql.UnmarshalExtendedMetadataDto(meta.RawMeta)
	if err != nil {
		return FullPostgreSQLMetadata{}, xerrors.Errorf("unmarshal postgres metadataDto: %w", err)
	}
	return FullPostgreSQLMetadata{meta, metaDto}, nil
}

type PostgresSizeMeasurer struct {
	pbm *PostgreSQLS3BackupManager
	mdb metadb.MetaDB
	lg  log.Logger
}

func NewPostgresSizeMeasurer(pbm *PostgreSQLS3BackupManager, mdb metadb.MetaDB, lg log.Logger) *PostgresSizeMeasurer {
	return &PostgresSizeMeasurer{pbm, mdb, lg}
}

// SizesAfterDeleted calculates sizes of backups which can changed during deletion:
// 1) deleted backup wals new size is zero
// 2) previous automated backup (if it exists) wals size is size of wals
//    from it's beginning until beginning of next automated backup
//    *exclude manual backups between them
func (pu *PostgresSizeMeasurer) SizesAfterDeleted(ctx context.Context, job models.BackupJob, bucket string) ([]backupmanager.BackupSize, error) {
	sizes := []backupmanager.BackupSize{{ID: job.BackupID, DataSize: 0, JournalSize: 0}}

	lockedBackups, err := pu.mdb.LockBackups(
		ctx,
		job.ClusterID,
		optional.NewString(job.SubClusterID),
		job.ShardID,
		nil,
		nil,
		[]string{job.BackupID},
	)
	if err != nil {
		return nil, err
	}

	parsedBackups, err := FullPGBackupsFromBackups(metadb.FilterBackupsByStatus(lockedBackups, metadb.StatusesUpdateSize))
	if err != nil {
		return nil, err
	}

	deletedBackup, err := NewFullPostgresqlBackup(job.Backup)
	if err != nil {
		return nil, err
	}

	parsedBackups = append(parsedBackups, deletedBackup)

	sort.Slice(parsedBackups, func(i int, j int) bool {
		return parsedBackups[i].ParsedMetadata.ParsedDtoMeta.FinishLsn < parsedBackups[j].ParsedMetadata.ParsedDtoMeta.FinishLsn
	})

	deletedIdx := slices.IndexFunc(parsedBackups, func(b FullPostgreSQLBackup) bool { return b.BackupID == job.BackupID })
	if deletedIdx == -1 {
		return nil, xerrors.Errorf("deleted backup was not found in metadb")
	}

	prevBackupIdx := PrevAutoBackup(parsedBackups, job.BackupID)
	if prevBackupIdx == -1 {
		return sizes, nil
	}

	parsedBackupsWithoutDeleted := append(parsedBackups[:deletedIdx], parsedBackups[deletedIdx+1:]...)

	prevBackup := parsedBackups[prevBackupIdx]
	prevAutoBackupRange, prevExcludeRanges, err := RangeForAutoBackup(parsedBackupsWithoutDeleted, prevBackup.BackupID)
	if err != nil {
		return nil, err
	}
	prevAutoBackupWals, err := ListWalsByRange(
		ctx,
		pu.pbm.ListWals,
		walg.WalgWalPathFromRootPath(prevBackup.ParsedMetadata.RootPath),
		bucket,
		prevAutoBackupRange,
		prevExcludeRanges,
		pu.lg,
	)
	if err != nil {
		return nil, err
	}

	sizes = append(sizes, backupmanager.BackupSize{ID: prevBackup.BackupID, DataSize: prevBackup.ParsedMetadata.DataSize, JournalSize: WalsSize(prevAutoBackupWals)})

	return sizes, nil
}

// SizesAfterCreated calculates sizes of backups which can be changed during creation:
// 1) created backup wals size is size of wals from beginning until end
// 2) previous automated backup (if it exists) wals size is size of wals
//    from it's beginning until beginning of created backup
//    *exclude manual backups between them
func (pu *PostgresSizeMeasurer) SizesAfterCreated(ctx context.Context, job models.BackupJob, backupMeta metadb.BackupMetadata, bucket string) ([]backupmanager.BackupSize, error) {
	var sizes []backupmanager.BackupSize

	createdMetaPG, ok := backupMeta.(postgresql.PostgreSQLMetadata)
	if !ok {
		return nil, xerrors.Errorf("postgresql metadata expected")
	}

	createdDto, err := postgresql.UnmarshalExtendedMetadataDto(createdMetaPG.RawMeta)
	if err != nil {
		return nil, xerrors.Errorf("range from backup meta: %w", err)
	}

	lockedBackups, err := pu.mdb.LockBackups(
		ctx,
		job.ClusterID,
		optional.NewString(job.SubClusterID),
		job.ShardID,
		nil,
		nil,
		[]string{job.BackupID},
	)
	if err != nil {
		return nil, err
	}

	if job.Initiator == metadb.BackupInitiatorUser {
		ctxlog.Debugf(ctx, pu.lg, "manual (%s) backup created, updating its own size only", job.Initiator)

		createdManualBackupRange, err := RangeFromBackupMeta(createdMetaPG.BackupName, createdDto)
		if err != nil {
			return nil, xerrors.Errorf("range from backup meta: %w", err)
		}

		createdManualBackupWals, err := ListWalsByRange(
			ctx,
			pu.pbm.ListWals,
			walg.WalgWalPathFromRootPath(createdMetaPG.RootPath),
			bucket,
			createdManualBackupRange,
			nil,
			pu.lg,
		)
		if err != nil {
			return nil, xerrors.Errorf("wal listing for created backup %s: %w", job.BackupID, err)
		}

		return []backupmanager.BackupSize{{ID: job.BackupID, DataSize: createdMetaPG.DataSize, JournalSize: WalsSize(createdManualBackupWals)}}, nil
	}

	ctxlog.Debugf(ctx, pu.lg, "automated (initiator = %s) backup created, updating sizes of created now and previous automated backups (if it exists)", job.Initiator)

	parsedBackups, err := FullPGBackupsFromBackups(metadb.FilterBackupsByStatus(lockedBackups, metadb.StatusesUpdateSize))
	if err != nil {
		return nil, err
	}
	parsedBackups = append(parsedBackups, FullPostgreSQLBackup{Backup: job.Backup, ParsedMetadata: FullPostgreSQLMetadata{PostgreSQLMetadata: createdMetaPG, ParsedDtoMeta: createdDto}})

	sort.Slice(parsedBackups, func(i int, j int) bool {
		return parsedBackups[i].ParsedMetadata.ParsedDtoMeta.FinishLsn < parsedBackups[j].ParsedMetadata.ParsedDtoMeta.FinishLsn
	})

	createdAutoBackupRange, excludeRanges, err := RangeForAutoBackup(parsedBackups, job.BackupID)
	if err != nil {
		return nil, err
	}

	ctxlog.Debugf(ctx, pu.lg, "calculated wal-ranges for created backup %+v, except %+v", createdAutoBackupRange, excludeRanges)

	createdAutoBackupWals, err := ListWalsByRange(
		ctx,
		pu.pbm.ListWals,
		walg.WalgWalPathFromRootPath(createdMetaPG.RootPath),
		bucket,
		createdAutoBackupRange,
		excludeRanges,
		pu.lg,
	)
	if err != nil {
		return nil, err
	}

	createdAutoBackupSizes := backupmanager.BackupSize{ID: job.BackupID, DataSize: createdMetaPG.DataSize, JournalSize: WalsSize(createdAutoBackupWals)}
	sizes = append(sizes, createdAutoBackupSizes)

	// If previous automated backup exists - recalculate it's wal size
	prevBackupIdx := PrevAutoBackup(parsedBackups, job.BackupID)
	if prevBackupIdx == -1 {
		return sizes, nil
	}

	prevBackup := parsedBackups[prevBackupIdx]
	prevAutoBackupRange, prevExcludeRanges, err := RangeForAutoBackup(parsedBackups, prevBackup.BackupID)
	if err != nil {
		return nil, err
	}
	ctxlog.Debugf(ctx, pu.lg, "calculated wal-ranges for previous backup (%s): %+v, except %+v", prevBackup.BackupID, prevAutoBackupRange, prevExcludeRanges)

	prevAutoBackupWals, err := ListWalsByRange(
		ctx,
		pu.pbm.ListWals,
		walg.WalgWalPathFromRootPath(prevBackup.ParsedMetadata.RootPath),
		bucket,
		prevAutoBackupRange,
		prevExcludeRanges,
		pu.lg,
	)
	if err != nil {
		return nil, err
	}

	sizes = append(sizes, backupmanager.BackupSize{ID: prevBackup.BackupID, DataSize: prevBackup.ParsedMetadata.DataSize, JournalSize: WalsSize(prevAutoBackupWals)})

	return sizes, nil
}

func WalsSize(wals []postgresql.WalSegment) int64 {
	var size int64
	for i := range wals {
		size += wals[i].Size
	}
	return size
}
