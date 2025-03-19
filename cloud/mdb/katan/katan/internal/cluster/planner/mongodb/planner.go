package mongodb

import (
	"fmt"
	"sort"

	"a.yandex-team.ru/cloud/mdb/katan/internal/tags"
	"a.yandex-team.ru/cloud/mdb/katan/katan/internal/cluster/planner"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
)

type hostRole string

const (
	hostRoleMongod     = hostRole("mongodb_cluster.mongod")
	hostRoleMongos     = hostRole("mongodb_cluster.mongos")
	hostRoleMongocfg   = hostRole("mongodb_cluster.mongocfg")
	hostRoleMongoinfra = hostRole("mongodb_cluster.mongoinfra")
	hostRoleUnknown    = hostRole("")
)

func parseHostRole(str string) hostRole {
	switch str {
	case string(hostRoleMongod):
		return hostRoleMongod
	case string(hostRoleMongos):
		return hostRoleMongos
	case string(hostRoleMongocfg):
		return hostRoleMongocfg
	case string(hostRoleMongoinfra):
		return hostRoleMongoinfra
	default:
		return hostRoleUnknown
	}
}

const (
	mongodHealthService   = "mongod"
	mongosHealthService   = "mongos"
	mongocfgHealthService = "mongocfg"

	minMongosCount     = 2
	minMongocfgCount   = 3
	minMongoinfraCount = 3
	planMaxSpeed       = 3
)

type replSet struct {
	Valid     bool
	Primary   string
	Secondary []string
	Count     int
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
	rs.Valid = true
	rs.Count++
	return nil
}

func (rs *replSet) Assert() error {
	if rs.Primary == "" {
		return fmt.Errorf("primary not found")
	}
	return nil
}

type primaryHosts struct {
	Valid bool
	Hosts []string
}

func (h *primaryHosts) AddHost(fqdn string, repl planner.Service) error {
	if repl.Role != types.ServiceRoleMaster {
		return fmt.Errorf("master role expected, but given %q. On %q", repl.Role, fqdn)
	}
	h.Hosts = append(h.Hosts, fqdn)
	h.Valid = true
	return nil
}

type mongoCluster struct {
	Mongod     map[string]replSet
	Mongos     primaryHosts
	Mongocfg   replSet
	Mongoinfra replSet
	Sharded    bool
}

func newMongoCluster() *mongoCluster {
	return &mongoCluster{
		Mongod: make(map[string]replSet),
	}
}

func (c *mongoCluster) Shards() []string {
	shards := make([]string, 0, len(c.Mongod))
	for k := range c.Mongod {
		shards = append(shards, k)
	}
	return shards
}

func (c *mongoCluster) Assert() error {
	if len(c.Mongod) < 1 {
		return fmt.Errorf("empty mongod host list")
	}

	for shardID, replSet := range c.Mongod {
		if err := replSet.Assert(); err != nil {
			return fmt.Errorf("mongod %q shard replset failure: %+v", shardID, err)
		}
	}
	if !c.Sharded {
		return nil
	}

	if c.Mongoinfra.Valid == (c.Mongocfg.Valid || c.Mongos.Valid) {
		return fmt.Errorf("expected or mongos+mongocfg or mongoinfra hosts")
	}

	if c.Mongoinfra.Valid {
		if err := c.Mongoinfra.Assert(); err != nil {
			return fmt.Errorf("mongoinfra replset failure: %+v", err)
		}
		if c.Mongoinfra.Count < minMongoinfraCount {
			return fmt.Errorf("expected at least %d mongoinfra hosts, but got %d", minMongoinfraCount, c.Mongoinfra.Count)
		}
	}

	if c.Mongocfg.Valid != c.Mongos.Valid {
		return fmt.Errorf("expected mongocfg with mongos hosts")
	}

	if c.Mongocfg.Valid {
		if err := c.Mongocfg.Assert(); err != nil {
			return fmt.Errorf("mongocfg replset failure: %+v", err)
		}
		if c.Mongocfg.Count < minMongocfgCount {
			return fmt.Errorf("expected at least %d mongocfg hosts, but got %d", minMongocfgCount, c.Mongocfg.Count)
		}
	}

	if c.Mongos.Valid {
		if len(c.Mongos.Hosts) < minMongosCount {
			return fmt.Errorf("expected at least %d mongos hosts, but got %d", minMongosCount, len(c.Mongos.Hosts))
		}
	}

	return nil
}

func (c *mongoCluster) AddHost(fqdn string, host planner.Host) error {
	role, err := hostRoleFromMeta(host.Tags.Meta)
	if err != nil {
		return err
	}

	switch role {
	case hostRoleMongod:
		shardID := host.Tags.Meta.ShardID
		if shardID == "" {
			return fmt.Errorf("empty shardID given: %+v", host)
		}
		repl, ok := host.Services[mongodHealthService]
		if !ok {
			return fmt.Errorf("no %q in %q host services: %+v", mongodHealthService, fqdn, host.Services)
		}

		hg := c.Mongod[shardID]
		err = hg.AddHost(fqdn, repl)
		c.Mongod[shardID] = hg
		if len(c.Mongod) > 1 {
			c.Sharded = true
		}

		if err != nil {
			return fmt.Errorf("mongod %q host failure: %+v", shardID, err)
		}
		return nil

	case hostRoleMongos:
		repl, ok := host.Services[mongosHealthService]
		if !ok {
			return fmt.Errorf("no %q in %q host services: %+v", mongosHealthService, fqdn, host.Services)
		}
		if err := c.Mongos.AddHost(fqdn, repl); err != nil {
			return fmt.Errorf("mongos host failure: %+v", err)
		}
		c.Sharded = true
		return nil

	case hostRoleMongocfg:
		repl, ok := host.Services[mongocfgHealthService]
		if !ok {
			return fmt.Errorf("no %q in %q host services: %+v", mongocfgHealthService, fqdn, host.Services)
		}
		if err := c.Mongocfg.AddHost(fqdn, repl); err != nil {
			return fmt.Errorf("mongocfg host failure: %+v", err)
		}
		c.Sharded = true
		return nil

	case hostRoleMongoinfra:
		repl, ok := host.Services[mongocfgHealthService]
		if !ok {
			return fmt.Errorf("no %q in %q host services: %+v", mongocfgHealthService, fqdn, host.Services)
		}
		if err := c.Mongoinfra.AddHost(fqdn, repl); err != nil {
			return fmt.Errorf("mongoinfra host failure: %+v", err)
		}
		c.Sharded = true
		return nil
	}

	return fmt.Errorf("unknown host type %q for host %+v", role, host)
}

func hostRoleFromMeta(meta tags.HostMeta) (hostRole, error) {
	if len(meta.Roles) != 1 {
		return "", fmt.Errorf("unexpected roles count: %+v", meta.Roles)
	}
	role := parseHostRole(meta.Roles[0])
	if role == hostRoleUnknown {
		return "", fmt.Errorf("unknown host role: %+v", meta.Roles[0])
	}
	return role, nil
}

func planFromHostGroup(hg replSet) [][]string {
	sort.Strings(hg.Secondary)
	plan := planner.LinearPlan(hg.Secondary, planMaxSpeed)
	plan = append(plan, []string{hg.Primary})
	return plan
}

// Planner plan rollout on mongodb cluster
func Planner(cluster planner.Cluster) ([][]string, error) {
	if len(cluster.Hosts) == 0 {
		return nil, fmt.Errorf("got empty cluster: %+v", cluster)
	}

	mongo := newMongoCluster()
	for fqdn, host := range cluster.Hosts {
		if err := mongo.AddHost(fqdn, host); err != nil {
			return nil, err
		}
	}

	if err := mongo.Assert(); err != nil {
		return nil, err
	}

	var plan [][]string

	shards := mongo.Shards()
	sort.Strings(shards)

	for _, shard := range shards {
		plan = append(plan, planFromHostGroup(mongo.Mongod[shard])...)
	}
	if mongo.Mongocfg.Valid {
		plan = append(plan, planFromHostGroup(mongo.Mongocfg)...)
	}
	if mongo.Mongoinfra.Valid {
		plan = append(plan, planFromHostGroup(mongo.Mongoinfra)...)
	}
	if mongo.Mongos.Valid {
		sort.Strings(mongo.Mongos.Hosts)
		plan = append(plan, planner.LinearPlan(mongo.Mongos.Hosts, planMaxSpeed)...)
	}

	return plan, nil
}
