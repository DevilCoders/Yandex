package ydb

import (
	"context"
	"strings"
	"time"

	"github.com/jmoiron/sqlx"

	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/table"

	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"

	"a.yandex-team.ru/cloud/marketplace/pkg/errtools"
)

var (
	ErrConnection = xerrors.NewSentinel("connection error")
	ErrPing       = xerrors.NewSentinel("ping failed")
)

type Connector struct {
	directPool *table.SessionPool
	db         *sqlx.DB

	rootPath string

	logger log.Logger
}

func Connect(ctx context.Context, options ...Option) (connection *Connector, resultErr error) {
	configurator := defaultConfigurator

	for i := range options {
		options[i](&configurator)
	}

	if err := configurator.validate(); err != nil {
		return nil, err
	}

	logger := configurator.logger
	logger.Debug("establishing ydb connection...", log.String("endpoint", configurator.endpoint))

	driver, err := configurator.makeYDBDialer().Dial(ctx, configurator.endpoint)
	if err != nil {
		return nil, ErrConnection.Wrap(err)
	}

	defer func() {
		if resultErr != nil {
			logger.Debug("closing ydb connection because of failure")
			_ = driver.Close()
		}
	}()

	logger.Debug("preparing table client driver...")

	tableClient := &table.Client{
		Driver: driver,
	}

	driverDB := configurator.openDBWithTableClient(tableClient)

	defer func() {
		if resultErr != nil {
			logger.Debug("closing ydb driver because of failure")
			_ = driverDB.Close()
		}
	}()

	directPool := &table.SessionPool{
		Builder:       tableClient,
		SizeLimit:     configurator.maxDirectConnections,
		IdleThreshold: time.Second,
	}

	logger.Debug("preparing sql driver...")

	db := configurator.makeAndSetupSQLDriver(driverDB)
	err = db.PingContext(ctx)
	if err != nil {
		return nil, ErrPing.Wrap(err)
	}

	connection = &Connector{
		db:         db,
		directPool: directPool,

		rootPath: configurator.rootPath,

		logger: logger,
	}

	logger.Debug("successfully prepared connection")

	return connection, resultErr
}

type closeError struct {
	poolErr error
	dbErr   error
}

func (e closeError) Error() string {
	var builder strings.Builder
	if e.poolErr != nil {
		_, _ = builder.WriteString(e.poolErr.Error())
		_, _ = builder.WriteRune(';')
	}

	if e.dbErr != nil {
		_, _ = builder.WriteString(e.dbErr.Error())
	}

	return builder.String()
}

func (c *Connector) Close() error {
	const defaultCloseTimeout = 5 * time.Second

	closeCtx, cancel := context.WithTimeout(context.Background(), defaultCloseTimeout)
	defer cancel()

	poolErr := c.directPool.Close(closeCtx)
	dbErr := c.db.Close()

	if poolErr != nil || dbErr != nil {
		return &closeError{
			poolErr: poolErr,
			dbErr:   dbErr,
		}
	}

	return nil
}

func (c *Connector) Root() string {
	return c.rootPath
}

func (c *Connector) DB() *sqlx.DB {
	return c.db
}

func (c *Connector) DirectPool() *table.SessionPool {
	return c.directPool
}

func (c *Connector) MapDBError(ctx context.Context, err error) error {
	return errtools.WithPrevCaller(err)
}
