package persistence

import (
	"context"
	"fmt"
	"os"
	"path"
	"runtime/debug"
	"sort"
	"strings"
	"time"

	"github.com/ydb-platform/ydb-go-sdk/v3"
	ydb_credentials "github.com/ydb-platform/ydb-go-sdk/v3/credentials"
	"github.com/ydb-platform/ydb-go-sdk/v3/sugar"
	ydb_table "github.com/ydb-platform/ydb-go-sdk/v3/table"
	ydb_options "github.com/ydb-platform/ydb-go-sdk/v3/table/options"
	ydb_result "github.com/ydb-platform/ydb-go-sdk/v3/table/result"
	ydb_types "github.com/ydb-platform/ydb-go-sdk/v3/table/types"
	"github.com/ydb-platform/ydb-go-sdk/v3/trace"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/logging"
	persistence_config "a.yandex-team.ru/cloud/disk_manager/internal/pkg/persistence/config"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/errors"
	"a.yandex-team.ru/library/go/core/metrics"
	ydbMetrics "a.yandex-team.ru/library/go/yandex/ydb/metrics"
)

////////////////////////////////////////////////////////////////////////////////

func createTable(
	ctx context.Context,
	session ydb_table.Session,
	fullPath string,
	description CreateTableDescription,
) error {

	options := make([]ydb_options.CreateTableOption, 0, len(description.Columns)+1)
	for _, column := range description.Columns {
		options = append(options, ydb_options.WithColumn(column.Name, column.Type))
	}

	options = append(options, ydb_options.WithPrimaryKeyColumn(description.PrimaryKey...))

	for _, key := range description.SecondaryKeys {
		options = append(
			options,
			ydb_options.WithIndex(
				key+"_index",
				ydb_options.WithIndexType(ydb_options.GlobalIndex()),
				ydb_options.WithIndexColumns(key),
			),
		)
	}

	options = append(options, ydb_options.WithColumnFamilies(description.ColumnFamilies...))

	if description.UniformPartitions > 0 {
		options = append(options, ydb_options.WithProfile(
			ydb_options.WithPartitioningPolicy(
				ydb_options.WithPartitioningPolicyUniformPartitions(description.UniformPartitions),
			),
		))
	}

	if len(description.ExternalBlobsMediaKind) > 0 {
		options = append(options, ydb_options.WithStorageSettings(
			ydb_options.StorageSettings{
				External:           ydb_options.StoragePool{Media: description.ExternalBlobsMediaKind},
				StoreExternalBlobs: ydb_options.FeatureEnabled,
			},
		))
		options = append(options, ydb_options.WithProfile(
			ydb_options.WithPartitioningPolicy(
				ydb_options.WithPartitioningPolicyMode(ydb_options.PartitioningDisabled),
			),
		))
	}

	return session.CreateTable(ctx, fullPath, options...)
}

func primaryKeysMatch(
	currentKey []string,
	key []string,
) bool {
	sort.Strings(currentKey)
	sort.Strings(key)
	if len(key) != len(currentKey) {
		return false
	}

	for i := range currentKey {
		if key[i] != currentKey[i] {
			return false
		}
	}

	return true
}

func splitColumns(
	lhs []ydb_options.Column,
	rhs []ydb_options.Column,
) (lhsOnly []ydb_options.Column, rhsOnly []ydb_options.Column, err error) {

	for _, column := range rhs {
		i := sort.Search(len(lhs), func(i int) bool {
			return lhs[i].Name >= column.Name
		})
		if i >= len(lhs) || lhs[i].Name != column.Name {
			rhsOnly = append(rhsOnly, column)
		} else if !ydb_types.Equal(lhs[i].Type, column.Type) {
			err = fmt.Errorf("column %v type mismatch old=%v, new=%v", column.Name, lhs[i].Type, column.Type)
		}
	}

	for _, column := range lhs {
		i := sort.Search(len(rhs), func(i int) bool {
			return rhs[i].Name >= column.Name
		})
		if i >= len(rhs) || rhs[i].Name != column.Name {
			lhsOnly = append(lhsOnly, column)
		}
	}

	return
}

func alterTable(
	ctx context.Context,
	session ydb_table.Session,
	fullPath string,
	currentDescription ydb_options.Description,
	description CreateTableDescription,
) error {

	currentPrimaryKey := currentDescription.PrimaryKey
	primaryKey := description.PrimaryKey
	if !primaryKeysMatch(currentPrimaryKey, primaryKey) {
		return fmt.Errorf("cannot change primary key. Current=%v, Requested=%v", currentPrimaryKey, primaryKey)
	}

	currentColumns := currentDescription.Columns
	columns := description.Columns
	sort.Slice(currentColumns, func(i, j int) bool {
		return currentColumns[i].Name < currentColumns[j].Name
	})
	sort.Slice(columns, func(i, j int) bool {
		return columns[i].Name < columns[j].Name
	})

	removedColumns, addedColumns, err := splitColumns(currentColumns, columns)
	if err != nil {
		return err
	}

	if len(removedColumns) != 0 {
		return fmt.Errorf("cannot remove columns. Requests to remove %v", removedColumns)
	}

	if len(addedColumns) == 0 {
		return nil
	}

	alterTableOptions := make([]ydb_options.AlterTableOption, 0, len(addedColumns))

	for _, column := range addedColumns {
		alterTableOptions = append(alterTableOptions, ydb_options.WithAddColumn(column.Name, column.Type))
	}

	return session.AlterTable(ctx, fullPath, alterTableOptions...)
}

func createOrAlterTable(
	ctx context.Context,
	session ydb_table.Session,
	fullPath string,
	description CreateTableDescription,
) error {

	currentDescription, err := session.DescribeTable(ctx, fullPath)

	switch {
	case err == nil:
		// Table exists, altering.
		err = alterTable(ctx, session, fullPath, currentDescription, description)
		if err != nil {
			return fmt.Errorf("failed to alter table %v: %w", fullPath, err)
		}

	case ydb.IsOperationErrorSchemeError(err):
		// Table not found, creating new.
		err = createTable(ctx, session, fullPath, description)
		if err != nil {
			return fmt.Errorf("failed to create table %v: %w", fullPath, err)
		}

	default:
		return fmt.Errorf("failed to describe table %v: %w", fullPath, err)
	}

	return nil
}

func dropTable(
	ctx context.Context,
	session ydb_table.Session,
	fullPath string,
) error {

	_, err := session.DescribeTable(ctx, fullPath)

	switch {
	case err == nil:
		// Table exists, dropping.
		err = session.DropTable(ctx, fullPath)
		if err != nil {
			return fmt.Errorf("failed to drop table %v: %w", fullPath, err)
		}

	case ydb.IsOperationErrorSchemeError(err):
		// Table not found, nothing to do.
	default:
		return fmt.Errorf("failed to describe table %vi before dropping: %w", fullPath, err)
	}

	return nil
}

////////////////////////////////////////////////////////////////////////////////

type CreateTableDescription struct {
	Columns                []ydb_options.Column
	PrimaryKey             []string
	SecondaryKeys          []string
	ColumnFamilies         []ydb_options.ColumnFamily
	UniformPartitions      uint64
	ExternalBlobsMediaKind string
}

type CreateTableOption func(*CreateTableDescription)

func WithColumn(name string, typ ydb_types.Type) CreateTableOption {
	return func(d *CreateTableDescription) {
		d.Columns = append(d.Columns, ydb_options.Column{
			Name: name,
			Type: typ,
		})
	}
}

func WithColumnAndFamily(name string, typ ydb_types.Type, family string) CreateTableOption {
	return func(d *CreateTableDescription) {
		d.Columns = append(d.Columns, ydb_options.Column{
			Name:   name,
			Type:   typ,
			Family: family,
		})
	}
}

func WithPrimaryKeyColumn(columns ...string) CreateTableOption {
	return func(d *CreateTableDescription) {
		d.PrimaryKey = append(d.PrimaryKey, columns...)
	}
}

func WithSecondaryKeyColumn(columns ...string) CreateTableOption {
	return func(d *CreateTableDescription) {
		d.SecondaryKeys = append(d.SecondaryKeys, columns...)
	}
}

func WithColumnFamilies(cf ...ydb_options.ColumnFamily) CreateTableOption {
	return func(d *CreateTableDescription) {
		d.ColumnFamilies = append(d.ColumnFamilies, cf...)
	}
}

func WithUniformPartitions(n uint64) CreateTableOption {
	return func(d *CreateTableDescription) {
		d.UniformPartitions = n
	}
}

func WithExternalBlobs(mediaKind string) CreateTableOption {
	return func(d *CreateTableDescription) {
		d.ExternalBlobsMediaKind = mediaKind
	}
}

func MakeCreateTableDescription(opts ...CreateTableOption) CreateTableDescription {
	var result CreateTableDescription
	for _, opt := range opts {
		opt(&result)
	}
	return result
}

////////////////////////////////////////////////////////////////////////////////

type Transaction struct {
	transaction ydb_table.Transaction
}

func (t *Transaction) Execute(
	ctx context.Context,
	query string,
	params *ydb_table.QueryParameters,
) (ydb_result.Result, error) {

	res, err := t.transaction.Execute(
		ctx,
		query,
		params,
	)
	if err != nil {
		logging.Debug(
			ctx,
			"Transaction.Execute query failed, query=%v: %v",
			query,
			err,
		)

		// TODO: some errors should not be retriable.
		return nil, &errors.RetriableError{
			Err: fmt.Errorf(
				"query failed, query=%v: %w",
				query,
				err,
			),
		}
	}

	return res, nil
}

func (t *Transaction) Commit(ctx context.Context) error {
	_, err := t.transaction.CommitTx(ctx)
	if err != nil {
		// TODO: some errors should not be retriable.
		return &errors.RetriableError{
			Err: fmt.Errorf("failed to commit transaction: %w", err),
		}
	}

	return nil
}

func (t *Transaction) Rollback(ctx context.Context) error {
	return t.transaction.Rollback(ctx)
}

////////////////////////////////////////////////////////////////////////////////

type Session struct {
	session ydb_table.Session
}

func (s *Session) BeginRWTransaction(
	ctx context.Context,
) (*Transaction, error) {

	settings := ydb_table.TxSettings(ydb_table.WithSerializableReadWrite())
	transaction, err := s.session.BeginTransaction(ctx, settings)
	if err != nil {
		// TODO: some errors should not be retriable.
		return nil, &errors.RetriableError{
			Err: fmt.Errorf("failed to begin transaction: %w", err),
		}
	}

	return &Transaction{transaction: transaction}, nil
}

func (s *Session) execute(
	ctx context.Context,
	control *ydb_table.TransactionControl,
	query string,
	params *ydb_table.QueryParameters,
) (ydb_table.Transaction, ydb_result.Result, error) {

	transaction, res, err := s.session.Execute(
		ctx,
		control,
		query,
		params,
	)
	if err != nil {
		logging.Debug(
			ctx,
			"Session.execute query failed, query=%v: %v",
			query,
			err,
		)

		// TODO: some errors should not be retriable.
		return nil, nil, &errors.RetriableError{
			Err: fmt.Errorf(
				"query failed, query=%v: %w",
				query,
				err,
			),
		}
	}

	return transaction, res, nil
}

func (s *Session) ExecuteRO(
	ctx context.Context,
	query string,
	params *ydb_table.QueryParameters,
) (ydb_table.Transaction, ydb_result.Result, error) {

	tx := ydb_table.TxControl(
		ydb_table.BeginTx(
			ydb_table.WithOnlineReadOnly(),
		),
		ydb_table.CommitTx(),
	)
	return s.execute(ctx, tx, query, params)
}

func (s *Session) ExecuteRW(
	ctx context.Context,
	query string,
	params *ydb_table.QueryParameters,
) (ydb_table.Transaction, ydb_result.Result, error) {

	tx := ydb_table.TxControl(
		ydb_table.BeginTx(
			ydb_table.WithSerializableReadWrite(),
		),
		ydb_table.CommitTx(),
	)
	return s.execute(ctx, tx, query, params)
}

func (s *Session) StreamReadTable(
	ctx context.Context,
	path string,
	opts ...ydb_options.ReadTableOption,
) (ydb_result.StreamResult, error) {

	res, err := s.session.StreamReadTable(ctx, path, opts...)
	if err != nil {
		// TODO: some errors should not be retriable.
		return nil, &errors.RetriableError{
			Err: fmt.Errorf(
				"read table failed, path=%v: %w",
				path,
				err,
			),
		}
	}

	return res, nil
}

////////////////////////////////////////////////////////////////////////////////

type YDBClient struct {
	db       ydb.Connection
	database string
	rootPath string
}

func (c *YDBClient) Close(ctx context.Context) error {
	return c.db.Close(ctx)
}

func (c *YDBClient) AbsolutePath(elem ...string) string {
	return path.Join(c.database, c.rootPath, path.Join(elem...))
}

func (c *YDBClient) makeDirs(ctx context.Context, absolutePath string) error {
	if !strings.HasPrefix(absolutePath, c.database) {
		return fmt.Errorf("'%v' is expected to be rooted at '%v'", absolutePath, c.database)
	}
	relativePath := strings.TrimPrefix(absolutePath, c.database)
	parts := strings.Split(relativePath, "/")

	dirPath := c.database
	for _, part := range parts {
		if len(part) == 0 {
			continue
		}
		dirPath = path.Join(dirPath, part)
		err := c.db.Scheme().MakeDirectory(ctx, dirPath)
		if err != nil {
			return fmt.Errorf("cannot make directory %v: %w", dirPath, err)
		}
	}

	return nil
}

func (c *YDBClient) CreateOrAlterTable(
	ctx context.Context,
	folder string,
	name string,
	description CreateTableDescription,
) error {

	folderFullPath := c.AbsolutePath(folder)
	fullPath := path.Join(folderFullPath, name)

	return c.Execute(
		ctx,
		func(ctx context.Context, s *Session) error {
			err := c.makeDirs(ctx, folderFullPath)
			if err != nil {
				return err
			}

			return createOrAlterTable(ctx, s.session, fullPath, description)
		},
	)
}

func (c *YDBClient) DropTable(
	ctx context.Context,
	folder string,
	name string,
) error {

	folderFullPath := c.AbsolutePath(folder)
	fullPath := path.Join(folderFullPath, name)

	return c.Execute(
		ctx,
		func(ctx context.Context, s *Session) error {
			return dropTable(ctx, s.session, fullPath)
		},
	)
}

func (c *YDBClient) Execute(
	ctx context.Context,
	op func(context.Context, *Session) error,
) error {

	adapter := func(ctx context.Context, session ydb_table.Session) error {
		return op(ctx, &Session{session: session})
	}

	err := c.db.Table().Do(ctx, adapter, ydb_table.WithIdempotent())
	if err != nil {
		// TODO: some errors should not be retriable.
		return &errors.RetriableError{Err: err}
	}

	return nil
}

func (c *YDBClient) ExecuteRO(
	ctx context.Context,
	query string,
	params *ydb_table.QueryParameters,
) (ydb_result.Result, error) {

	var res ydb_result.Result

	err := c.Execute(ctx, func(ctx context.Context, session *Session) error {
		var err error
		_, res, err = session.ExecuteRO(ctx, query, params)
		return err
	})

	return res, err
}

func (c *YDBClient) ExecuteRW(
	ctx context.Context,
	query string,
	params *ydb_table.QueryParameters,
) (ydb_result.Result, error) {

	var res ydb_result.Result

	err := c.Execute(ctx, func(ctx context.Context, session *Session) error {
		var err error
		_, res, err = session.ExecuteRW(ctx, query, params)
		return err
	})

	return res, err
}

////////////////////////////////////////////////////////////////////////////////

type optionsHolder struct {
	registry metrics.Registry
	creds    ydb_credentials.Credentials
	details  trace.Details
}

type option func(h *optionsHolder)

func WithRegistry(registry metrics.Registry) option {
	return func(h *optionsHolder) {
		h.registry = registry
	}
}

func WithCredentials(creds ydb_credentials.Credentials) option {
	return func(h *optionsHolder) {
		h.creds = creds
	}
}

func WithDetails(details trace.Details) option {
	return func(h *optionsHolder) {
		h.details = details
	}
}

////////////////////////////////////////////////////////////////////////////////

func CreateYDBClient(
	ctx context.Context,
	config *persistence_config.PersistenceConfig,
	opts ...option,
) (*YDBClient, error) {

	h := &optionsHolder{
		details: trace.DriverRepeaterEvents |
			trace.DriverConnEvents |
			trace.TablePoolEvents |
			trace.RetryEvents |
			trace.DiscoveryEvents |
			trace.SchemeEvents,
	}
	for _, o := range opts {
		o(h)
	}

	options := []ydb.Option{
		ydb.WithPanicCallback(func(e interface{}) {
			fmt.Fprintf(os.Stdout, "panic from ydb driver: %v\n%s", e, debug.Stack())
			os.Exit(1)
		}),
		ydb.WithLogger(
			h.details,
			ydb.WithExternalLogger(
				&logger{
					ctx: ctx,
				},
			),
		),
	}

	if !config.GetDisableAuthentication() {
		options = append(options, ydb.WithCredentials(h.creds))
	}

	if h.registry != nil {
		options = append(
			options,
			ydbMetrics.WithTraces(
				h.registry,
				ydbMetrics.WithDetails(h.details),
			),
		)
	}

	if config.GetSecure() && len(config.GetRootCertsFile()) != 0 {
		options = append(
			options,
			ydb.WithCertificatesFromFile(config.GetRootCertsFile()),
		)
	}

	connectionTimeout, err := time.ParseDuration(config.GetConnectionTimeout())
	if err != nil {
		return nil, fmt.Errorf("failed to parse ConnectionTimeout: %w", err)
	}

	options = append(
		options,
		ydb.WithDialTimeout(connectionTimeout),
	)

	driver, err := ydb.Open(
		ctx,
		sugar.DSN(config.GetEndpoint(), config.GetDatabase(), config.GetSecure()),
		options...,
	)
	if err != nil {
		return nil, fmt.Errorf("failed to dial: %w", err)
	}

	return &YDBClient{
		db:       driver,
		database: config.GetDatabase(),
		rootPath: config.GetRootPath(),
	}, nil
}

////////////////////////////////////////////////////////////////////////////////

func TimestampValue(t time.Time) ydb_types.Value {
	if t.IsZero() {
		return ydb_types.ZeroValue(ydb_types.TypeTimestamp)
	}

	return ydb_types.TimestampValueFromTime(t)
}
