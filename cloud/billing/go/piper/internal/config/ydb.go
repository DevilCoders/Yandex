package config

import (
	"fmt"

	"github.com/jmoiron/sqlx"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/adapters/ydb"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/config/cfgtypes"
	ydbclient "a.yandex-team.ru/cloud/billing/go/piper/internal/db/ydb/client"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling/logf"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/table"
)

type YDBProvider func(cfgtypes.YDBParams) (YDBConnections, error)

type YDBConnections interface {
	DirectPool() *table.SessionPool
	DB() *sqlx.DB
	Close() error
}

type ydbContainer struct {
	ydbInstContainer

	ydbProvider YDBProvider

	ydbMetaDBOnce  initSync
	ydbMetaAdapter *ydb.MetaAdapter

	ydbCumulative        ydbInstContainer
	ydbCumulativeAdapter *ydb.MetaAdapter

	ydbUniq        ydbInstContainer
	ydbUniqAdapter *ydb.DataAdapter

	ydbPresenter        ydbInstContainer
	ydbPresenterAdapter *ydb.DataAdapter
}

type ydbInstContainer struct {
	ydbAdapterOnce initSync
	ydbConnections YDBConnections
}

func (c *Container) GetYDB() (*sqlx.DB, error) {
	if err := c.initYDB(); err != nil {
		return nil, err
	}
	return c.ydbConnections.DB(), nil
}

func (c *Container) GetYDBAdapter() (*ydb.MetaAdapter, error) {
	err := c.initYDBAdapter()
	return c.ydbMetaAdapter, err
}

func (c *Container) GetYDBCumulativeAdapter() (*ydb.MetaAdapter, error) {
	err := c.initYDBCumulativeAdapter()
	return c.ydbCumulativeAdapter, err
}

func (c *Container) GetYDBUniqAdapter() (*ydb.DataAdapter, error) {
	err := c.initYDBUniqAdapter()
	return c.ydbUniqAdapter, err
}

func (c *Container) GetYDBPresenterAdapter() (*ydb.DataAdapter, error) {
	err := c.initYDBPresenterAdapter()
	return c.ydbPresenterAdapter, err
}

func (c *Container) initYDB() error {
	return c.ydbMetaDBOnce.Do(c.connectYDB)
}

func (c *Container) initYDBAdapter() error {
	return c.ydbAdapterOnce.Do(c.makeYDBAdapter)
}

func (c *Container) initYDBCumulativeAdapter() error {
	return c.ydbCumulative.ydbAdapterOnce.Do(c.makeYDBCumulativeAdapter)
}

func (c *Container) initYDBUniqAdapter() error {
	return c.ydbUniq.ydbAdapterOnce.Do(c.makeYDBUniqAdapter)
}

func (c *Container) initYDBPresenterAdapter() error {
	return c.ydbPresenter.ydbAdapterOnce.Do(c.makeYDBPresenterAdapter)
}

func (c *Container) connectYDB() error {
	c.initializedTooling()
	if c.initFailed() {
		return c.initError
	}

	cfg, err := c.GetYDBConfig()
	if err != nil {
		return err
	}

	prov := c.ydbProvider
	if prov == nil {
		prov = c.ydbBuildProvider
	}
	c.ydbConnections, err = prov(cfg.YDBParams)
	if err != nil {
		return c.failInitF("YDB init: %w", err)
	}
	return nil
}

func (c *Container) makeYDBAdapter() error {
	c.initializedTooling()
	if c.initFailed() {
		return c.initError
	}
	err := c.initYDB()
	if err != nil {
		return err
	}
	cfg, err := c.GetYDBConfig()
	if err != nil {
		return err
	}

	c.debug("ydb adapter database root", logf.Value(cfg.Root))
	c.ydbMetaAdapter, err = ydb.NewMetaAdapter(c.mainCtx,
		c.ydbConnections.DB(),
		ydb.YDBScheme(c.ydbConnections.DirectPool()),
		cfg.Database,
		cfg.Root,
	)
	if err != nil {
		return c.failInitF("YDB adapter init: %w", err)
	}
	c.debug("ydb adapter constructed")

	c.statesMgr.Add("ydb-adapter", c.ydbMetaAdapter)
	c.debug("ydb adapter state registered")

	return nil
}

func (c *Container) makeYDBCumulativeAdapter() error {
	c.initializedTooling()
	if c.initFailed() {
		return c.initError
	}
	fullCfg, err := c.GetYDBConfig()
	if err != nil {
		return err
	}
	cfg := fullCfg.Installations.Cumulative

	prov := c.ydbProvider
	if prov == nil {
		prov = c.ydbBuildCumulativeProvider
	}
	c.ydbCumulative.ydbConnections, err = prov(cfg)
	if err != nil {
		return c.failInitF("YDB cumulative init: %w", err)
	}

	c.debug("ydb cumulative adapter database root", logf.Value(cfg.Root))
	c.ydbCumulativeAdapter, err = ydb.NewMetaAdapter(c.mainCtx,
		c.ydbCumulative.ydbConnections.DB(),
		ydb.YDBScheme(c.ydbCumulative.ydbConnections.DirectPool()),
		cfg.Database,
		cfg.Root,
	)
	if err != nil {
		return c.failInitF("YDB cumulative adapter init: %w", err)
	}
	c.debug("ydb cumulative adapter constructed")

	c.statesMgr.Add("ydb-cumulative-adapter", c.ydbUniqAdapter)
	c.debug("ydb cumulative adapter state registered")

	return nil
}

func (c *Container) makeYDBUniqAdapter() error {
	c.initializedTooling()
	if c.initFailed() {
		return c.initError
	}
	fullCfg, err := c.GetYDBConfig()
	if err != nil {
		return err
	}
	cfg := fullCfg.Installations.Uniq

	prov := c.ydbProvider
	if prov == nil {
		prov = c.ydbBuildUniqProvider
	}
	c.ydbUniq.ydbConnections, err = prov(cfg)
	if err != nil {
		return c.failInitF("YDB uniq init: %w", err)
	}

	c.debug("ydb uniq adapter database root", logf.Value(cfg.Root))
	c.ydbUniqAdapter, err = ydb.NewDataAdapter(c.mainCtx,
		ydb.DataAdapterConfig{EnableUniq: true},
		c.ydbUniq.ydbConnections.DB(),
		ydb.YDBScheme(c.ydbUniq.ydbConnections.DirectPool()),
		cfg.Database,
		cfg.Root,
	)
	if err != nil {
		return c.failInitF("YDB uniq adapter init: %w", err)
	}
	c.debug("ydb uniq adapter constructed")

	c.statesMgr.Add("ydb-uniq-adapter", c.ydbUniqAdapter)
	c.debug("ydb uniq adapter state registered")

	return nil
}

func (c *Container) makeYDBPresenterAdapter() error {
	c.initializedTooling()
	if c.initFailed() {
		return c.initError
	}
	fullCfg, err := c.GetYDBConfig()
	if err != nil {
		return err
	}
	cfg := fullCfg.Installations.Presenter

	prov := c.ydbProvider
	if prov == nil {
		prov = c.ydbBuildPresenterProvider
	}
	c.ydbPresenter.ydbConnections, err = prov(cfg)
	if err != nil {
		return c.failInitF("YDB presenter init: %w", err)
	}

	c.debug("ydb presenter adapter database root", logf.Value(cfg.Root))
	c.ydbPresenterAdapter, err = ydb.NewDataAdapter(c.mainCtx,
		ydb.DataAdapterConfig{EnablePresenter: true},
		c.ydbPresenter.ydbConnections.DB(),
		ydb.YDBScheme(c.ydbPresenter.ydbConnections.DirectPool()),
		cfg.Database,
		cfg.Root,
	)
	if err != nil {
		return c.failInitF("YDB presenter adapter init: %w", err)
	}
	c.debug("ydb presenter adapter constructed")

	c.statesMgr.Add("ydb-presenter-adapter", c.ydbPresenterAdapter)
	c.debug("ydb presenter adapter state registered")

	return nil
}

func (c *Container) ydbBuildProvider(config cfgtypes.YDBParams) (YDBConnections, error) {
	con, err := c.ydbBuild(config)
	if err != nil {
		return nil, err
	}

	c.statesMgr.AddCritical("ydb", con)
	c.debug("ydb state registered")
	return con, nil
}

func (c *Container) ydbBuildUniqProvider(config cfgtypes.YDBParams) (YDBConnections, error) {
	con, err := c.ydbBuild(config)
	if err != nil {
		return nil, err
	}

	c.statesMgr.AddCritical("ydb-uniq", con)
	c.debug("ydb state registered")
	return con, nil
}

func (c *Container) ydbBuildCumulativeProvider(config cfgtypes.YDBParams) (YDBConnections, error) {
	con, err := c.ydbBuild(config)
	if err != nil {
		return nil, err
	}

	c.statesMgr.AddCritical("ydb-cumulative", con)
	c.debug("ydb state registered")
	return con, nil
}

func (c *Container) ydbBuildPresenterProvider(config cfgtypes.YDBParams) (YDBConnections, error) {
	con, err := c.ydbBuild(config)
	if err != nil {
		return nil, err
	}

	c.statesMgr.AddCritical("ydb-presenter", con)
	c.debug("ydb state registered")
	return con, nil
}

func (c *Container) ydbBuild(config cfgtypes.YDBParams) (*ydbclient.Connections, error) {
	c.debug("ydb auth type", logf.Value(config.Auth.Type.String()))
	auth, err := c.GetYDBAuth(config.Auth)
	if err != nil {
		return nil, err
	}
	c.debug("ydb address", logf.Value(config.Address))
	c.debug("ydb database", logf.Value(config.Database))

	cfg := ydbclient.NewConfig().
		DB(config.Address, config.Database).
		Credentials(auth).
		ConnectTimeout(config.ConnectTimeout.Duration()).
		RequestTimeout(config.RequestTimeout.Duration()).
		DiscoveryInterval(config.DiscoveryInterval.Duration()).
		MaxConnections(config.MaxConnections).
		MaxIdleConnections(config.MaxIdleConnections).
		MaxDirectConnections(config.MaxDirectConnections).
		ConnMaxLifetime(config.ConnMaxLifetime.Duration())

	if !config.DisableTLS.Bool() {
		c.debug("ydb tls enabled")
		tls, err := c.GetTLS()
		if err != nil {
			return nil, err
		}
		cfg = cfg.TLS(tls)
	}

	con, err := cfg.Build(c.mainCtx)
	if err != nil {
		return nil, fmt.Errorf("ydb: %w", err)
	}
	c.debug("ydb connected")
	return con, nil
}
