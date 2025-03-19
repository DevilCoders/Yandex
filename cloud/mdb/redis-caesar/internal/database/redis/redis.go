package redis

import (
	"context"
	"fmt"
	"strconv"
	"strings"
	"sync"

	goredis "github.com/go-redis/redis/v8"

	"a.yandex-team.ru/cloud/mdb/redis-caesar/internal"
	"a.yandex-team.ru/cloud/mdb/redis-caesar/internal/database"
	"a.yandex-team.ru/cloud/mdb/redis-caesar/pkg/config"
	"a.yandex-team.ru/library/go/core/log"
)

const (
	redisInfoPairLen            int = 2
	redisCommandReplicaOf           = "REPLICAOF"
	redisInfoSectionReplication     = "Replication"
	redisInfoKeyRole                = "role"
	redisInfoKeyReplID              = "master_replid"
	redisInfoKeyReplOffset          = "master_repl_offset"
)

var redisCommandReplicaOfForMaster = []string{redisCommandReplicaOf, "NO", "ONE"}

type clients map[internal.RedisHost]*goredis.Client

type redisDatabase struct {
	sync.Mutex
	clock   internal.Clock
	conf    *config.RedisConfig
	logger  log.Logger
	clients clients
}

func New(conf *config.RedisConfig, logger log.Logger) (database.DB, error) {
	if conf.ClusterName == "" {
		return nil, fmt.Errorf("redis is not configured please fill the cluster name")
	}
	if len(conf.Nodes) == 0 {
		return nil, fmt.Errorf("redis is not configured please fill the redis hosts list")
	}

	return &redisDatabase{
		clock:   &internal.RealClock{},
		conf:    conf,
		logger:  logger,
		clients: clients{},
	}, nil
}

func (d *redisDatabase) UpdateHostsConnections(ctx context.Context, info internal.AllDBsInfo) error {
	d.Lock()
	defer d.Unlock()

	newClients := clients{}
	oldClients := d.clients

	for host, hostInfo := range info {
		if _, ok := d.clients[host]; ok && hostInfo.Alive { // if we already have connection to certain client then ping it and keep this connection
			newClients, oldClients = d.keepExistingClients(ctx, host, newClients, oldClients)
		} else {
			newClients = d.addNewConnection(host, newClients)
		}
	}
	d.clients = newClients
	if len(oldClients) > 0 {
		hosts := []string{}
		for host := range oldClients {
			hosts = append(hosts, string(host))
		}
		d.logger.Warn("some database connections are outdated", log.Strings("hosts", hosts))
	}

	return nil
}

func (d *redisDatabase) InitHostsConnections(ctx context.Context) {
	d.Lock()
	defer d.Unlock()

	for host, creds := range d.conf.Nodes {
		d.clients[host] = goredis.NewClient(&goredis.Options{
			Addr:     string(host),
			Username: creds.User,
			Password: creds.Password,
		})
	}
}

func (d *redisDatabase) Ping(ctx context.Context) error {
	b := database.NewPingErrorBuilder()
	for host, connection := range d.clients {
		err := connection.Ping(ctx)
		if err != nil {
			b.AddHost(host)
		}
	}

	return b.Build()
}

func (d *redisDatabase) PingHost(ctx context.Context, host internal.RedisHost) error {
	b := database.NewPingErrorBuilder()
	err := d.clients[host].Ping(ctx)
	if err != nil {
		b.AddHost(host)
	}

	return b.Build()
}

func (d *redisDatabase) DatabasesInfo(ctx context.Context) (internal.AllDBsInfo, error) {
	result := internal.AllDBsInfo{}
	for host, connection := range d.clients {
		response := connection.Info(ctx, redisInfoSectionReplication)
		if err := response.Err(); err != nil {
			result[host] = internal.DBInfo{
				Host:  host,
				Alive: false,
				Error: err,
			}

			continue
		}
		info := d.parseRedisInfo(response.String())
		result[host] = info
	}

	return result, nil
}

func (d *redisDatabase) PromoteMaster(ctx context.Context, master internal.RedisHost) error {
	err := d.clients[master].Do(ctx, redisCommandReplicaOfForMaster).Err() // at first sending replicaof with NO ONE to master
	if err != nil {
		return fmt.Errorf("unable to make host %s master: %w", master, err)
	}

	for host, connection := range d.clients {
		if host == master { // skipping sending replicaof to host that already set to master
			continue
		}
		hostname, port := host.HostnameAndPort()
		err := connection.Do(ctx, redisCommandReplicaOf, hostname, port).Err()
		if err != nil {
			return fmt.Errorf("unable to make host %s replica of %s: %w", host, master, err)
		}
	}

	return nil
}

func (d *redisDatabase) parseRedisInfo(info string) internal.DBInfo {
	result := internal.DBInfo{
		Alive:              true,
		LastSuccessfulPing: d.clock.Now(),
	}
	infoPairs := strings.Split(info, "\n")
	for _, pair := range infoPairs {
		slicedInfoPair := strings.Split(pair, ":")
		if len(slicedInfoPair) == 0 {
			continue
		}
		if len(slicedInfoPair) != redisInfoPairLen {
			d.logger.Warn("strange info pair found while trying to get database info", log.String("info pair", pair))

			continue
		}
		name, value := slicedInfoPair[0], slicedInfoPair[1]
		switch name {
		case redisInfoKeyRole:
			result.Role = internal.RedisRole(value)
		case redisInfoKeyReplID:
			result.ReplicaID = value
		case redisInfoKeyReplOffset:
			converted, err := strconv.Atoi(value)
			if err != nil {
				d.logger.Error("unable to convert replica offset to integer", log.String("replica_offset", value))
			}
			result.ReplicaOffset = int64(converted)
		}
	}

	return result
}

func (d *redisDatabase) keepExistingClients(ctx context.Context, host internal.RedisHost, newClients, oldClients clients) (clients, clients) {
	status := oldClients[host].Ping(ctx)
	if err := status.Err(); err != nil {
		d.logger.Warn("unable to ping database host", log.String("host", string(host)), log.Error(err))

		return newClients, oldClients
	}
	newClients[host] = oldClients[host]
	delete(oldClients, host)

	return newClients, oldClients
}

func (d *redisDatabase) addNewConnection(host internal.RedisHost, clients map[internal.RedisHost]*goredis.Client) map[internal.RedisHost]*goredis.Client {
	hostCreds, ok := d.conf.Nodes[host]
	if !ok {
		d.logger.Error("unable to find credentials for redis host", log.String("host", string(host)))

		return clients
	}
	clients[host] = goredis.NewClient(&goredis.Options{
		Addr:     string(host),
		Username: hostCreds.User,
		Password: hostCreds.Password,
	})

	return clients
}
