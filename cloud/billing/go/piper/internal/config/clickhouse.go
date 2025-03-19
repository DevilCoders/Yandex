package config

import (
	"context"
	"errors"
	"fmt"

	"github.com/jmoiron/sqlx"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/adapters/clickhouse"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/config/cfgtypes"
	clickhouseclient "a.yandex-team.ru/cloud/billing/go/piper/internal/db/clickhouse/client"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling/logf"
)

type ClickhouseProvider func(context.Context, string, string, cfgtypes.ClickhouseCommonConfig) (ClickhouseConnections, error)

type ClickhouseConnections interface {
	DB() *sqlx.DB
	Close() error
}

type chContainer struct {
	chOnce        initSync
	chAdapterOnce initSync
	chCluster     DBCluster
	chProvider    ClickhouseProvider
	chAdapter     *clickhouse.Adapter
}

func (c *Container) GetClickhouse() (DBCluster, error) {
	if err := c.initCH(); err != nil {
		return nil, err
	}
	return c.chCluster, nil
}

func (c *Container) GetClickhouseAdapter() (*clickhouse.Adapter, error) {
	err := c.initChAdapter()
	return c.chAdapter, err
}

func (c *Container) initCH() error {
	return c.chOnce.Do(c.connectCH)
}

func (c *Container) initChAdapter() error {
	return c.chAdapterOnce.Do(c.makeChAdapter)
}

func (c *Container) connectCH() error {
	c.initializedTooling()
	if c.initFailed() {
		return c.initError
	}

	cfg, err := c.GetClickhouseConfig()
	if err != nil {
		return err
	}
	if len(cfg.Shards) == 0 {
		return c.failInit(errors.New("clickhouse is not configured"))
	}

	nodes := make([][]clickhouseclient.ClusterNode, len(cfg.Shards))
	for i := range cfg.Shards {
		if len(cfg.Shards[i].Hosts) == 0 {
			return c.failInitF("clickhouse: shard %d has no hosts", i)
		}
		nodes[i] = make([]clickhouseclient.ClusterNode, len(cfg.Shards[i].Hosts))
	}

	prov := c.chProvider
	if prov == nil {
		prov = c.chBuildProvider
	}

	if !cfg.DisableTLS.Bool() {
		tls, err := c.GetTLS()
		if err != nil {
			return err
		}
		clickhouseclient.SetTLSConfig(tls)
	} else {
		c.debug("clickhouse tsl disabled")
	}

	for shardNum, shard := range cfg.Shards {
		for hostNum := range shard.Hosts {
			host := cfg.Shards[shardNum].Hosts[hostNum]
			name := fmt.Sprintf("%d-%d", shardNum, hostNum)
			node, err := prov(c.mainCtx, host, name, cfg.ClickhouseCommonConfig)
			if err != nil {
				return fmt.Errorf("connection to shard %d host %s error: %w", shardNum, host, err)
			}
			nodes[shardNum][hostNum] = clickhouseclient.ClusterNode{
				Name:   host, // May be use some abstract name here after some ops experience
				DBConn: node.DB(),
			}
		}
	}

	cluster, err := clickhouseclient.NewCluster(c.runCtx, nodes)
	if err != nil {
		return c.failInitF("clickhouse: %w", err)
	}
	c.statesMgr.Add("ch-cluster", cluster)

	c.chCluster = cluster
	return nil
}

func (c *Container) makeChAdapter() error {
	c.initializedTooling()
	if c.initFailed() {
		return c.initError
	}
	err := c.initCH()
	if err != nil {
		return err
	}
	c.chAdapter, err = clickhouse.New(c.runCtx, c.chCluster)
	if err != nil {
		return c.failInitF("clickhouse adapter init: %w", err)
	}
	c.debug("clickhouse adapter constructed")

	c.statesMgr.Add("ch-adapter", c.chAdapter)
	c.debug("clickhouse adapter state registered")

	return nil
}

const clickhouseCompression = 6

func (c *Container) chBuildProvider(ctx context.Context, host string, name string, config cfgtypes.ClickhouseCommonConfig) (ClickhouseConnections, error) {
	if ctx.Err() != nil {
		return nil, ctx.Err()
	}
	c.debug("clickhouse address", logf.Value(host))
	c.debug("clickhouse database", logf.Value(config.Database))
	c.debug("clickhouse user", logf.Value(config.Auth.User))

	cfg := clickhouseclient.NewConfig().
		// Alive().Debug(true).
		Address(host).
		Port(config.Port).
		Username(config.Auth.User).
		Password(config.Auth.Password).
		Database(config.Database).
		Secure(!config.DisableTLS.Bool()).
		Compress(clickhouseCompression)

	con, err := cfg.Build(ctx)
	if err != nil {
		return nil, err
	}
	c.debug("clickhouse connected")

	c.statesMgr.Add(fmt.Sprintf("ch-%s", name), con)
	c.debug("clickhouse state registered")
	return con, nil
}
