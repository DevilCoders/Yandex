package clickhouseclient

import (
	"context"
	"database/sql"
	"fmt"
	"time"

	"github.com/jmoiron/sqlx"
	"golang.yandex/hasql"
)

type Cluster struct {
	shards []*hasql.Cluster
}

func NewCluster(runCtx context.Context, nodes [][]ClusterNode) (cluster *Cluster, resErr error) {
	shards := make([]*hasql.Cluster, len(nodes))
	defer func() {
		if resErr != nil {
			for _, s := range shards {
				if s != nil {
					_ = s.Close()
				}
			}
		}
	}()

	for i := range shards {
		haNodes := make([]hasql.Node, len(nodes[i]))
		for ni, n := range nodes[i] {
			haNodes[ni] = n
		}

		shard, err := hasql.NewCluster(haNodes, chCheck, hasql.WithUpdateInterval(time.Second*15))
		if err != nil {
			return nil, fmt.Errorf("creating shard %d: %w", i, err)
		}
		shards[i] = shard
	}
	return &Cluster{shards: shards}, nil
}

func (c *Cluster) DB(shardNo int) (*sqlx.DB, error) {
	if shardNo > len(c.shards) {
		return nil, ErrNoSuchPartition.Wrap(fmt.Errorf("%d of %d", shardNo, len(c.shards)))
	}
	shard := c.shards[shardNo]

	node := shard.Alive()
	if node == nil {
		return nil, ErrNoAliveNodes.Wrap(fmt.Errorf("shard %d", shardNo))
	}

	return node.(interface{ DBX() *sqlx.DB }).DBX(), nil
}

func (c *Cluster) Partitions() int {
	return len(c.shards)
}

func (c *Cluster) Close() (err error) {
	for _, s := range c.shards {
		if ce := s.Close(); ce != nil {
			err = ce
		}
	}
	return
}

var _ hasql.Node = ClusterNode{}

type ClusterNode struct {
	Name   string
	DBConn *sqlx.DB
}

func (n ClusterNode) String() string { return n.Name }
func (n ClusterNode) Addr() string   { return n.Name }

func (n ClusterNode) DB() *sql.DB {
	return n.DBConn.DB
}

func (n ClusterNode) DBX() *sqlx.DB {
	return n.DBConn
}

func chCheck(ctx context.Context, db *sql.DB) (bool, error) {
	err := db.PingContext(ctx)
	return err == nil, err
}
