package kikimr

import (
	"database/sql"
	"database/sql/driver"
	"fmt"
	"path"
	"regexp"
	"strings"
	"sync"
	"time"

	"golang.org/x/xerrors"

	"a.yandex-team.ru/cloud/compute/go-common/tracing"

	"go.uber.org/zap"
	"golang.org/x/net/context"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"

	"a.yandex-team.ru/cloud/compute/go-common/database/kikimr"
	log "a.yandex-team.ru/cloud/compute/go-common/logging/ctxlog"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/config"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/globalauth"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/misc"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/storage"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/scheme"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/table"
)

const (
	defaultDatabase = "default"

	// chunks and snapshotchunks are smaller than blobs
	blobsShardCapacity               = 100 * misc.GB
	chunksRowSize                    = 200
	snapshotChunksRowSize            = 100
	chunksToBlobsSplitFactor         = misc.GB / (blobsShardCapacity / storage.DefaultChunkSize * chunksRowSize)
	snapshotChunksToBlobsSplitFactor = misc.GB / (blobsShardCapacity / storage.DefaultChunkSize * snapshotChunksRowSize)

	createTablesRetryTimeout = 10 * time.Minute

	healthcheckID = "healthcheck-id"

	bigTableProfileName = "ExtBlobsOnHDD"
)

type columnDescription struct {
	columnName string
	columnType string
	external   bool
}

func (cd columnDescription) String() string {
	var external string
	if cd.external {
		external = " [EXTERNAL]"
	}
	return fmt.Sprintf("%s %s%s", cd.columnName, cd.columnType, external)
}

type tableDescription struct {
	name          string
	columns       []columnDescription
	keyColumns    []string
	autoSplitSize int64
	startSplit    splitter
}

func (td tableDescription) String(workingdir string) string {
	columns := make([]string, 0, len(td.columns))
	for _, c := range td.columns {
		columns = append(columns, c.String())
	}
	var autoSplit string
	if td.autoSplitSize != 0 {
		autoSplit = "[AUTOSPLIT] "
	}
	var startSplit string
	if td.startSplit != nil {
		startSplit = fmt.Sprintf("[START_SPLIT=%d] ", td.startSplit.GetCount())
	}

	return fmt.Sprintf("[%s] %s%s(%s, PRIMARY KEY (%s))",
		workingdir+"/"+td.name, autoSplit, startSplit,
		strings.Join(columns, ", "), strings.Join(td.keyColumns, ", "))
}

func (td tableDescription) IsExternal() bool {
	for _, c := range td.columns {
		if c.external {
			return true
		}
	}
	return false
}

func (td tableDescription) GetOptions(useBlobHDDprofile bool) (opts []table.CreateTableOption) {
	opts = append(opts, table.WithPrimaryKeyColumn(td.keyColumns...))
	typeMap := map[string]ydb.Type{
		"string": ydb.Optional(ydb.TypeString),
		"int64":  ydb.Optional(ydb.TypeInt64),
		"uint64": ydb.Optional(ydb.TypeUint64),
		"bool":   ydb.Optional(ydb.TypeBool),
		"json":   ydb.Optional(ydb.TypeJSON),
	}

	for _, col := range td.columns {
		column := table.Column{
			Name: col.columnName,
			Type: typeMap[col.columnType],
		}
		if col.external && useBlobHDDprofile {
			column.Family = bigTableProfileName
		}
		opts = append(opts, table.WithColumnMeta(column))
	}

	if td.IsExternal() && useBlobHDDprofile {
		// from https://st.yandex-team.ru/KIKIMR-6232#5c46b87038af440021ec421c
		opts = append(opts, table.WithColumnFamilies(table.ColumnFamily{
			Name:         bigTableProfileName,
			Data:         table.StoragePool{Media: "hdd"},
			Compression:  table.ColumnFamilyCompressionLZ4,
			KeepInMemory: ydb.FeatureDisabled,
		}))
	}

	var profileOpts []table.PartitioningPolicyOption

	if td.autoSplitSize != 0 {
		profileOpts = append(profileOpts,
			table.WithPartitioningPolicyMode(table.PartitioningAutoSplit))
	}
	if td.startSplit != nil {
		profileOpts = append(profileOpts, table.WithPartitioningPolicyExplicitPartitions(td.startSplit.GetNewDriverBoundary()...))
	}
	if len(profileOpts) > 0 {
		opts = append(opts, table.WithProfile(table.WithPartitioningPolicy(profileOpts...)))
	}

	return
}

func containsExternalProfile(descr table.Description) bool {
	for _, c := range descr.ColumnFamilies {
		if c.Name == bigTableProfileName {
			return true
		}
	}
	return false
}

var (
	queryRegexp = compileQueryRegexp(buildTables(0))
)

func compileQueryRegexp(tables []tableDescription) *regexp.Regexp {
	tableNames := make([]string, 0, len(tables))
	for _, td := range tables {
		tableNames = append(tableNames, td.name)
	}
	// NB: There is a dummy symbol after table name.
	// it is to distinguish 'snapshots' and 'snapshotsext'
	// Must be stripped before matching to table.
	re := "\\$(" + strings.Join(tableNames, "|") + ")([^a-zA-Z_]|$)|\\s{2,}"
	return regexp.MustCompile(re)
}

func IsKikimrRetryableErr(err error) bool {
	// new ydb driver internal error checker (abort/rollback -> Retrible() == true)
	checker := ydb.RetryChecker{}
	return checker.Check(err).Retriable()
}

func convertRetryableError(err error) error {
	if IsKikimrRetryableErr(err) {
		return misc.ErrInternalRetry
	}

	// Temporary, may remove after fix https://st.yandex-team.ru/KIKIMR-11336
	if xerrors.Is(err, driver.ErrBadConn) {
		return misc.ErrInternalRetry
	}

	// context canceled or something simular errors
	if ydb.DefaultRetryChecker.Check(err).MustCheckSession() {
		return context.Canceled
	}

	if s, ok := status.FromError(err); ok && s.Code() == codes.Canceled {
		return context.Canceled
	}

	if s, ok := status.FromError(err); ok && s.Code() == codes.DeadlineExceeded {
		return context.DeadlineExceeded
	}

	return err
}

func kikimrError(err error) error {
	err = convertRetryableError(err)
	return err
}

func buildTables(startSplit int) []tableDescription {
	var (
		blobsPartitions          = startSplit
		chunksPartitions         = startSplit / chunksToBlobsSplitFactor
		snapshotchunksPartitions = startSplit / snapshotChunksToBlobsSplitFactor
	)
	return []tableDescription{
		{
			"snapshotsext",
			[]columnDescription{
				{"id", "string", false},
				{"base", "string", false},
				{"tree", "string", false},
				{"state", "string", false},
				{"statedescription", "string", false},
				{"created", "uint64", false},
				{"deleted", "uint64", false},
				{"size", "int64", false},
				{"realsize", "int64", false},
				{"organization", "string", false},
				{"project", "string", false},
				{"public", "bool", false},
				{"metadata", "string", false},
				{"name", "string", false},
				{"description", "string", false},
				{"disk", "string", false},
				{"productid", "string", false},
				{"imageid", "string", false},
				{"chunksize", "int64", false},
				{"checksum", "string", false},
			}, []string{"id"}, misc.GB, nil,
		},
		// To find children
		{
			"snapshotsext_base",
			[]columnDescription{
				{"base", "string", false},
				{"id", "string", false},
			}, []string{"base", "id"}, misc.GB, nil,
		},
		// To find all snapshots in tree
		{
			"snapshotsext_tree",
			[]columnDescription{
				{"tree", "string", false},
				{"id", "string", false},
			}, []string{"tree", "id"}, misc.GB, nil,
		},
		// To list by project
		{
			"snapshotsext_project",
			[]columnDescription{
				{"project", "string", false},
				{"id", "string", false},
			}, []string{"project", "id"}, misc.GB, nil,
		},
		// For names unique checking
		{
			"snapshotsext_name",
			[]columnDescription{
				{"project", "string", false},
				{"name", "string", false},
				{"id", "string", false},
			}, []string{"project", "name"}, misc.GB, nil,
		},
		// Public snapshots index
		{
			"snapshotsext_public",
			[]columnDescription{
				{"id", "string", false},
			}, []string{"id"}, misc.GB, nil,
		},
		// We separate statedescription to prevent lock invalidation on progress update
		{
			"snapshotsext_statedescription",
			[]columnDescription{
				{"id", "string", false},
				{"statedescription", "string", false},
			}, []string{"id"}, misc.GB, nil,
		},
		// Snapshots ready for GC
		// Number of rows in this table is much less than index by state.
		// Chain ID is used for correct dependency processing
		// on realsize change calculation
		{
			"snapshotsext_gc",
			[]columnDescription{
				{"id", "string", false},
				{"chain", "string", false},
			}, []string{"id"}, misc.GB, nil,
		},
		{
			"snapshots",
			[]columnDescription{
				{"id", "string", false},
				{"base", "string", false},
				{"tree", "string", false},
				{"chunksize", "int64", false},
				{"realsize", "int64", false},
			}, []string{"id"}, misc.GB, nil,
		},
		{
			"snapshots_base",
			[]columnDescription{
				{"base", "string", false},
				{"id", "string", false},
			}, []string{"base", "id"}, misc.GB, nil,
		},
		{
			"snapshots_tree",
			[]columnDescription{
				{"tree", "string", false},
				{"id", "string", false},
			}, []string{"tree", "id"}, misc.GB, nil,
		},
		// To protect snapshotchunks on concurrent deletion of subsequent snapshots
		{
			"snapshots_locks",
			[]columnDescription{
				{"id", "string", false},
				{"timestamp", "uint64", false},
				{"lock_state", "json", false},
			}, []string{"id"}, misc.GB, nil,
		},
		{
			"snapshotchunks",
			[]columnDescription{
				{"tree", "string", false},
				{"snapshotid", "string", false},
				{"chunkoffset", "int64", false},
				{"chunkid", "string", false},
			}, []string{"tree", "snapshotid", "chunkoffset"}, misc.GB,
			uuidSplitter{&snapshotchunksPartitions},
		},
		{
			"chunks",
			[]columnDescription{
				{"id", "string", false},
				{"sum", "string", false},
				{"format", "string", false},
				{"size", "int64", false},
				{"refcnt", "int64", false},
				{"zero", "bool", false},
			}, []string{"id"}, misc.GB,
			uuidSplitter{&chunksPartitions},
		},
		{
			"blobs_on_hdd",
			[]columnDescription{
				{"id", "string", false},
				{"data", "string", true},
			}, []string{"id"}, 0,
			uuidSplitter{&blobsPartitions},
		},
		{
			"sizechanges",
			[]columnDescription{
				{"id", "string", false},
				{"timestamp", "uint64", false},
				{"realsize", "int64", false},
			}, []string{"id", "timestamp"}, misc.GB, nil,
		},
		{
			"changed_children",
			[]columnDescription{
				{"base", "string", false},
				{"id", "string", false},
				{"timestamp", "uint64", false},
				{"realsize", "int64", false},
			}, []string{"base", "id"}, misc.GB, nil,
		},
	}
}

type splitter interface {
	GetCount() int
	GetNewDriverBoundary() []ydb.Value
}

type uuidSplitter struct {
	count *int
}

func (us uuidSplitter) GetCount() int {
	if us.count == nil || *us.count <= 1 {
		return 1
	}

	return *us.count
}

func (us uuidSplitter) GetNewDriverBoundary() []ydb.Value {
	if us.count == nil || *us.count <= 1 {
		return nil
	}

	// We work with 3-byte prefixes
	prefixCount := 1 << 24
	step := prefixCount / (*us.count - 1)
	boundary := make([]ydb.Value, 0, prefixCount/step)
	for i := step; i < prefixCount; i += step {
		boundary = append(boundary, ydb.TupleValue(ydb.OptionalValue(ydb.StringValue([]byte(fmt.Sprintf("%06x", i))))))
	}
	return boundary
}

type kikimrstorage struct {
	cache               storage.SnapshotChunksCache
	wg                  sync.WaitGroup
	p                   *kikimr.DatabaseProvider
	db                  kikimrDB
	kikimrGlobalTimeout time.Duration
	fs                  *storage.FsWarehouse // CLOUD-1825: mock for speedup
	clearChunkBatchSize int
	clearChunkWorkers   int

	lockPollInterval time.Duration
	timeSkewInterval time.Duration
	lockDuration     time.Duration
}

///////////////////////////////////////////////////////////////////////////////
// Initialization methods
///////////////////////////////////////////////////////////////////////////////

// NewKikimr returns new kikimr-based storage
func NewKikimr(ctx context.Context, conf *config.Config, tableOps int) (storage.Storage, error) {
	cache, err := storage.NewSnapshotChunksCache(conf.General.CacheSize)
	if err != nil {
		log.G(ctx).Error("Unable to create ARCCache", zap.Int("cache_size", conf.General.CacheSize), zap.Error(err))
		return nil, xerrors.Errorf("failed to create ARCCache: %w", err)
	}

	// Hack for vagrant
	newConf := kikimr.Config{}
	for k, v := range conf.Kikimr {
		var vnew kikimr.DatabaseConfig
		vnew, err = kikimr.SubstituteVariables(v)
		if err != nil {
			log.G(ctx).Error("Failed to substitute vagrant", zap.Error(err), zap.Any("value", v))
			return nil, xerrors.Errorf("failed to substitute vagrant: %w", err)
		}
		newConf[k] = vnew
	}
	conf.Kikimr = newConf

	p, err := kikimr.NewDatabaseProvider(conf.Kikimr)
	if err != nil {
		log.G(ctx).Error("Failed to parse kikimr config", zap.Error(err))
		return nil, err
	}

	kdb := p.GetDatabase(defaultDatabase)
	if kdb == nil {
		log.G(ctx).Error("Kikimr database not in config", zap.String("database", defaultDatabase))
		return nil, fmt.Errorf("kikimr database not in config")
	}

	dbConfig := conf.Kikimr[defaultDatabase]
	_ = dbConfig.Validate() // set defaults
	st := &kikimrstorage{
		cache:               cache,
		p:                   p,
		clearChunkBatchSize: conf.General.ClearChunkBatchSize,
		clearChunkWorkers:   conf.Performance.ClearChunkWorkers,

		lockPollInterval:    time.Second * time.Duration(dbConfig.LockPollIntervalSec),
		timeSkewInterval:    time.Second * time.Duration(dbConfig.TimeSkewSec),
		kikimrGlobalTimeout: time.Duration(dbConfig.OperationTimeoutMs) * time.Millisecond,
	}
	st.lockDuration = st.lockPollInterval + st.timeSkewInterval +
		time.Second*time.Duration(dbConfig.UpdateGraceTimeSec) + time.Second

	// CLOUD-1825: mock for speedup
	if conf.General.DummyFsRoot != "" {
		st.fs = &storage.FsWarehouse{BaseDir: conf.General.DummyFsRoot}
	}

	cr := globalauth.GetCredentials()
	db, err := p.GetDatabase(defaultDatabase).Open(ctx, cr)
	if err != nil {
		log.G(ctx).Error("Failed to open database", zap.String("database", defaultDatabase), zap.Error(err))
		return nil, err
	}
	st.db = sqlDB{db, st.replacer()}

	if err = st.Init(ctx, tableOps, buildTables(conf.General.StartSplit)); err != nil {
		return nil, err
	}
	return st, nil
}

func (st *kikimrstorage) Init(ctx context.Context, tableOps int, tables []tableDescription) (resErr error) {
	span, ctx := tracing.StartSpanFromContextFunc(ctx)
	defer func() { tracing.Finish(span, resErr) }()

	for _, td := range tables {
		if err := st.p.CreateFakeTables(defaultDatabase + "/" + td.name); err != nil {
			log.G(ctx).Error("CreateFakeTables failed", zap.String("database", defaultDatabase),
				zap.String("table", td.name), zap.Error(err))
			return err
		}
	}
	if tableOps&misc.TableOpDrop != 0 {
		_ = st.dropTables(ctx, tables)
	}
	if tableOps&misc.TableOpCreate != 0 {
		err := st.retryWithTx(ctx, "createTables", func(tx Querier) error {
			return st.createTables(ctx, tx, tables)
		})
		if err != nil {
			return err
		}
	}
	return nil
}

func (st *kikimrstorage) dropTables(ctx context.Context, tables []tableDescription) error {
	cl, err := st.p.GetDatabase(defaultDatabase).Dial(ctx, globalauth.GetCredentials())
	log.DebugErrorCtx(ctx, err, "dropTablesNewDriver: connect to driver")
	if err != nil {
		return xerrors.Errorf("droptables connect to db: %w", err)
	}

	session, err := cl.CreateSession(ctx)
	log.DebugErrorCtx(ctx, err, "dropTablesNewDriver: session creation")
	if err != nil {
		return err
	}
	defer func() {
		err := session.Close(ctx)
		log.DebugErrorCtx(ctx, err, "dropTablesNewDriver: session close")
	}()
	var errs []interface{}
	for _, td := range tables {
		tableString := path.Join(st.p.GetDatabase(defaultDatabase).GetConfig().Root, td.name)
		log.G(ctx).Info("dropTablesNewDriver: start drop", zap.String("table", tableString))
		err = session.DropTable(ctx, tableString)
		switch {
		case err == nil:
			log.G(ctx).Info("table drop successful", zap.String("table", tableString))
		case ydb.IsOpError(err, ydb.StatusNotFound):
			errs = append(errs, err)
			log.G(ctx).Warn("table not found", zap.String("table", tableString))
		default:
			errs = append(errs, err)
			log.G(ctx).Error("dropTablesNewDriver: failed to drop table '%s'",
				zap.String("table", tableString), zap.Error(err))
		}
	}
	if len(errs) > 0 {
		return fmt.Errorf(fmt.Sprintf("dropTablesError:%s", strings.Repeat(" %w", len(errs))), errs...)
	}
	return nil
}

func (st *kikimrstorage) createTables(ctx context.Context, _ Querier, tables []tableDescription) error {
	workDir := st.p.GetDatabase(defaultDatabase).GetConfig().Root
	dbName := st.p.GetDatabase(defaultDatabase).GetConfig().DBName

	cl, err := st.p.GetDatabase(defaultDatabase).Dial(ctx, globalauth.GetCredentials())
	log.DebugErrorCtx(ctx, err, "createTablesNewDriver: connect to driver")
	if err != nil {
		return xerrors.Errorf("createTablesNewDriver connect to db: %w", err)
	}
	session, err := cl.CreateSession(ctx)
	log.DebugErrorCtx(ctx, err, "createTablesNewDriver: session creation")
	if err != nil {
		return err
	}
	defer func() {
		err := session.Close(ctx)
		log.DebugErrorCtx(ctx, err, "createTablesNewDriver: session close")
	}()
	pathClient := scheme.Client{Driver: cl.Driver}

	parts := strings.Split(strings.TrimPrefix(workDir, dbName), "/")
	if len(parts) > 0 && parts[0] == "" {
		parts = parts[1:]
	}
	for i := 1; i <= len(parts); i++ {
		// Create dbName/first_part then dbName/first_part/second_part etc
		partPath := fmt.Sprintf("%s/%s", dbName, strings.Join(parts[:i], "/"))
		err = pathClient.MakeDirectory(ctx, partPath)
		if err != nil && !ydb.IsOpError(err, ydb.StatusAlreadyExists) {
			log.G(ctx).Error("createTablesNewDriver: failed to create directory", zap.Error(err),
				zap.String("work_dir", partPath))
			return err
		}
	}

	for _, td := range tables {
		tableString := path.Join(workDir, td.name)
		log.G(ctx).Info("Creating table (new driver)", zap.String("table", tableString))
		err = session.CreateTable(ctx, tableString, td.GetOptions(st.p.GetDatabase(defaultDatabase).GetConfig().UseBigTablesProfile)...)
		switch {
		case err == nil:
			log.G(ctx).Info("Table created successfully", zap.String("table", tableString))
		case ydb.IsOpError(err, ydb.StatusAlreadyExists):
			log.G(ctx).Warn("createTablesNewDriver: table already exists.", zap.String("table", tableString))
		default:
			log.G(ctx).Error("createTablesNewDriver: failed to create table '%s'",
				zap.String("table", tableString), zap.Error(err))
			return err
		}

		err = misc.RetryExtended(ctx, fmt.Sprintf("wait for %s", td.name), misc.RetryParams{RetryTimeout: createTablesRetryTimeout}, func() error {
			descr, err := session.DescribeTable(ctx, tableString)
			switch {
			case err == nil:
				log.G(ctx).Info("Table found successfully", zap.String("table", tableString))
				if td.IsExternal() && !containsExternalProfile(descr) {
					log.G(ctx).Error("table created without supposed profile", zap.String("table", td.name), zap.Any("description", descr))
				}
			case ydb.IsOpError(err, ydb.StatusNotFound):
				return misc.ErrInternalRetry
			default:
				return err
			}
			return nil
		})
		log.DebugErrorCtx(ctx, err, "createTablesNewDriver: wait for table", zap.String("table", tableString))
		if err != nil {
			return err
		}
	}
	return nil
}

///////////////////////////////////////////////////////////////////////////////
// Interface methods
///////////////////////////////////////////////////////////////////////////////

func (st *kikimrstorage) Close() error {
	st.wg.Wait()
	return st.db.Close()
}

///////////////////////////////////////////////////////////////////////////////
// Utility methods
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Implementation methods
///////////////////////////////////////////////////////////////////////////////

func (st *kikimrstorage) PingContext(ctx context.Context) (resErr error) {
	span, ctx := tracing.StartSpanFromContextFunc(ctx)
	defer func() { tracing.Finish(span, resErr) }()

	return st.db.PingContext(ctx)
}

func (st *kikimrstorage) Health(ctx context.Context) (resErr error) {
	span, ctx := tracing.StartSpanFromContextFunc(ctx)
	defer func() { tracing.Finish(span, resErr) }()

	if err := st.PingContext(ctx); err != nil {
		return err
	}

	var foundID string
	if err := st.retryWithTx(ctx, "healthcheck", func(tx Querier) error {
		// not prepared statement
		res, err := tx.QueryContext(ctx, "SELECT id FROM $snapshotsext_public WHERE id = $1", healthcheckID)
		if err != nil {
			return err
		}
		defer func() {
			_ = res.Close()
		}()
		if !res.Next() {
			return nil
		}
		return res.Scan(&foundID)
	}); err != nil {
		if xerrors.Is(err, table.ErrSessionPoolOverflow) {
			// XXX: mitigate session leak
			panic("session pool overflow")
		}
		return err
	}

	if err := st.retryWithTx(ctx, "healthcheck-prepared", func(tx Querier) error {
		// prepared statement
		_, err := tx.QueryContext(ctx,
			"#PREPARE "+
				"DECLARE $id AS Utf8; "+
				"SELECT id FROM $snapshotsext_public WHERE id = $id",
			sql.Named("id", healthcheckID))
		return err
	}); err != nil {
		if xerrors.Is(err, table.ErrSessionPoolOverflow) {
			// XXX: mitigate session leak
			panic("session pool overflow")
		}
		return err
	}

	switch foundID {
	case healthcheckID:
		return nil
	case "":
		if err := st.retryWithTx(ctx, "healthcheck-insert", func(tx Querier) error {
			_, err := tx.ExecContext(ctx, "UPSERT INTO $snapshotsext_public (id) VALUES ($1)", healthcheckID)
			return err
		}); err == nil {
			return st.Health(ctx)
		} else {
			if xerrors.Is(err, table.ErrSessionPoolOverflow) {
				// XXX: mitigate session leak
				panic("session pool overflow")
			}
			return err
		}
	default:
		return fmt.Errorf("id: found != expected; %s != %s", foundID, healthcheckID)
	}
}
