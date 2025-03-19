package helpers

import (
	"context"
	"fmt"
	"os"
	"time"

	"golang.org/x/xerrors"

	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/pgutil"
	"a.yandex-team.ru/library/go/core/log/nop"
)

const (
	envNameHost = "METADB_POSTGRESQL_RECIPE_HOST"
	envNamePort = "METADB_POSTGRESQL_RECIPE_PORT"
)

func Host() (string, error) {
	host, ok := os.LookupEnv(envNameHost)
	if !ok {
		return "", xerrors.Errorf("metadb recipe host %q missing in env", envNameHost)
	}

	return host, nil
}

func MustHost() string {
	host, err := Host()
	if err != nil {
		panic(err)
	}

	return host
}

func Port() (string, error) {
	port, ok := os.LookupEnv(envNamePort)
	if !ok {
		return "", xerrors.Errorf("metadb recipe port %q missing in env", envNamePort)
	}

	return port, nil
}

func MustPort() string {
	port, err := Port()
	if err != nil {
		panic(err)
	}

	return port
}

func pgConfig() pgutil.Config {
	return pgutil.Config{
		Addrs:   []string{fmt.Sprintf("%s:%s", MustHost(), MustPort())},
		DB:      "dbaas_metadb",
		SSLMode: pgutil.AllowSSLMode,
		User:    "dbaas_api",
	}
}

func newClient(ctx context.Context) (*sqlutil.Cluster, sqlutil.Node, error) {
	c, err := pgutil.NewCluster(pgConfig())
	if err != nil {
		return nil, nil, xerrors.Errorf("failed to create cluster: %w", err)
	}

	node, err := waitForDB(ctx, c)
	if err != nil {
		return nil, nil, xerrors.Errorf("failed to wait for db: %w", err)
	}

	return c, node, nil
}

func waitForDB(ctx context.Context, db *sqlutil.Cluster) (sqlutil.Node, error) {
	ctx, cancel := context.WithTimeout(ctx, time.Second*10)
	defer cancel()
	return db.WaitForPrimary(ctx)
}

type Client struct {
	Cluster *sqlutil.Cluster
	Node    sqlutil.Node
}

func NewClient() (*Client, error) {
	c := &Client{}
	return c, c.Reset()
}

// Reset closes old and creates new client to database. This is required at least after each 'drop schema'
// because OIDs are cached and will be invalid after schema recreation.
// TODO: update to pgx v4.1.x and rework this crap
func (c *Client) Reset() error {
	if c.Cluster != nil {
		// Remove old cluster so that we won't reuse it accidentally
		old := c.Cluster
		c.Cluster = nil
		if err := old.Close(); err != nil {
			return xerrors.Errorf("failed to close cluster: %w", err)
		}
	}

	cluster, node, err := newClient(context.Background())
	if err != nil {
		return xerrors.Errorf("failed to create client to metadb: %w", err)
	}

	c.Cluster = cluster
	c.Node = node
	return nil
}

var (
	queryCleanup = sqlutil.Stmt{
		Name:  "Cleanup",
		Query: "DELETE FROM dbaas.%s",
	}
)

func CleanupMetaDB(ctx context.Context, master sqlutil.Node) error {
	binding, err := sqlutil.BeginOnNode(ctx, master, nil)
	if err != nil {
		panic(err)
	}
	defer binding.Rollback(ctx)

	for _, name := range []string{
		"default_feature_flags",
		"cloud_feature_flags",
		"idempotence",
		"worker_queue",
		"target_pillar",
		"pillar_revs",
		"versions_revs",
		"hosts_revs",
		"shards_revs",
		"instance_groups_revs",
		"kubernetes_node_groups_revs",
		"subclusters_revs",
		"alert",
		"alert_revs",
		"alert_group",
		"alert_group_revs",
		"clusters_revs",
		"backup_schedule_revs",
		"clusters_changes",
		"pillar",
		"versions",
		"disks",
		"hosts",
		"kubernetes_node_groups",
		"placement_groups",
		"disk_placement_groups",
		"shards",
		"instance_groups",
		"subclusters",
		"hadoop_jobs",
		"backup_schedule",
		"backups_dependencies",
		"clusters",
		"folders",
		"clouds",
		"search_queue",
		"backups",
	} {
		_, err := sqlutil.QueryTxBinding(ctx, binding, queryCleanup.Format(name), nil, sqlutil.NopParser, &nop.Logger{})
		if err != nil {
			return xerrors.Errorf("failed to cleanup table %s: %w", name, err)
		}
	}

	if err := binding.Commit(ctx); err != nil {
		return xerrors.Errorf("failed to commit metadb cleanup: %w", err)
	}
	return nil
}
