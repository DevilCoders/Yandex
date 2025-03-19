package redis

import (
	"fmt"
	"sort"

	"a.yandex-team.ru/cloud/mdb/katan/katan/internal/cluster/planner"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
)

const (
	redisSentinelHealthService = "redis"
	redisShardedHealthService  = "redis_cluster"
	planMaxSpeed               = 3
)

type replSet struct {
	Primary   string
	Secondary []string
}

func (rs *replSet) AddHost(fqdn string, repl planner.Service) error {
	switch repl.Role {
	case types.ServiceRoleMaster:
		if rs.Primary != "" {
			return fmt.Errorf("found 2 masters: %q and %q", rs.Primary, fqdn)
		}
		rs.Primary = fqdn
	case types.ServiceRoleReplica:
		rs.Secondary = append(rs.Secondary, fqdn)
	default:
		return fmt.Errorf("unsupported service role: %q. On %q", repl.Role, fqdn)
	}
	return nil
}

func (rs *replSet) Assert() error {
	if rs.Primary == "" {
		return fmt.Errorf("primary not found")
	}
	return nil
}

type redisCluster struct {
	Shards   map[string]replSet
	Sharded  bool
	Sentinel bool
}

func newRedisCluster() *redisCluster {
	return &redisCluster{
		Shards: make(map[string]replSet),
	}
}

func (c *redisCluster) GetShards() []string {
	shards := make([]string, 0, len(c.Shards))
	for k := range c.Shards {
		shards = append(shards, k)
	}
	return shards
}

func (c *redisCluster) Assert(hosts map[string]planner.Host) error {
	if len(c.Shards) < 1 {
		return fmt.Errorf("empty shards list")
	}
	if c.Sharded && c.Sentinel {
		return fmt.Errorf("unexpected sentinel\\sharded hosts mix: %+v", hosts)
	}
	if c.Sharded && len(c.Shards) == 1 || c.Sentinel && len(c.Shards) > 1 {
		return fmt.Errorf("unexpected shards number for cluster type: %+v", hosts)
	}

	for shardID, replSet := range c.Shards {
		if err := replSet.Assert(); err != nil {
			return fmt.Errorf("redis %q shard replset failure: %w", shardID, err)
		}
	}
	return nil
}

func (c *redisCluster) AddHost(fqdn string, host planner.Host) error {
	shardID := host.Tags.Meta.ShardID
	if shardID == "" {
		return fmt.Errorf("empty shardID given: %+v", host)
	}

	hg := c.Shards[shardID]
	var err error
	if repl, ok := host.Services[redisSentinelHealthService]; ok {
		c.Sentinel = true
		err = hg.AddHost(fqdn, repl)
	} else if repl, ok := host.Services[redisShardedHealthService]; ok {
		c.Sharded = true
		err = hg.AddHost(fqdn, repl)
	} else {
		return fmt.Errorf("no %q, neither %q in %q host services: %+v",
			redisSentinelHealthService, redisShardedHealthService, fqdn, host.Services)
	}
	c.Shards[shardID] = hg

	if err != nil {
		return fmt.Errorf("redis %q host failure: %w", shardID, err)
	}
	return nil
}

func planFromHostGroup(hg replSet) [][]string {
	sort.Strings(hg.Secondary)
	plan := planner.LinearPlan(hg.Secondary, planMaxSpeed)
	plan = append(plan, []string{hg.Primary})
	return plan
}

// Planner plan rollout on redis cluster
func Planner(cluster planner.Cluster) ([][]string, error) {
	if len(cluster.Hosts) == 0 {
		return nil, fmt.Errorf("got empty cluster: %+v", cluster)
	}

	redis := newRedisCluster()
	for fqdn, host := range cluster.Hosts {
		if err := redis.AddHost(fqdn, host); err != nil {
			return nil, err
		}
	}

	if err := redis.Assert(cluster.Hosts); err != nil {
		return nil, err
	}

	var plan [][]string

	shards := redis.GetShards()
	sort.Strings(shards)

	for _, shard := range shards {
		plan = append(plan, planFromHostGroup(redis.Shards[shard])...)
	}

	return plan, nil
}
