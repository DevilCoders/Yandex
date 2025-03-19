package redis

import (
	"context"
	"time"

	goredis "github.com/go-redis/redis/v8"

	"a.yandex-team.ru/cloud/mdb/dataproc-manager/pkg/datastore"
	"a.yandex-team.ru/cloud/mdb/dataproc-manager/pkg/models"
	"a.yandex-team.ru/cloud/mdb/internal/secret"
	"a.yandex-team.ru/library/go/core/log"
)

// Config represents backend configuration
type Config struct {
	Addrs                      []string
	MasterName                 string
	Password                   secret.String
	DB                         int
	Expiration                 time.Duration
	DecommissionExpirationTime time.Duration
}

// DefaultConfig return default config for redis backend
func DefaultConfig() Config {
	return Config{
		Addrs:                      []string{"localhost:6379"},
		MasterName:                 "",
		Password:                   secret.NewString(""),
		DB:                         1,
		Expiration:                 time.Minute * 2,
		DecommissionExpirationTime: 24 * time.Hour,
	}
}

type backend struct {
	logger log.Logger
	cfg    Config
	client goredis.UniversalClient
}

// New constructor for Redis datastore
func New(logger log.Logger, cfg Config) datastore.Backend {
	return &backend{
		logger: logger,
		cfg:    cfg,
		client: goredis.NewUniversalClient(
			&goredis.UniversalOptions{
				Addrs:      cfg.Addrs,
				MasterName: cfg.MasterName,
				Password:   cfg.Password.Unmask(),
				DB:         cfg.DB,
			},
		),
	}
}

// Close database
func (b *backend) Close() error {
	return b.client.Close()
}

func (b *backend) StoreClusterHealth(ctx context.Context, cid string, report models.ClusterHealth) error {
	result := b.client.Set(ctx, cid, &report, b.cfg.Expiration)
	return result.Err()
}

func (b *backend) LoadClusterHealth(ctx context.Context, cid string) (models.ClusterHealth, error) {
	result := b.client.Get(ctx, cid)
	data, err := result.Bytes()
	ch := models.ClusterHealth{}
	if err != nil {
		if err == goredis.Nil {
			return models.ClusterHealth{}, datastore.ErrNotFound
		}
		return models.ClusterHealth{}, result.Err()
	}

	err = ch.UnmarshalBinary(data)
	if err != nil {
		return models.ClusterHealth{}, err
	}
	return ch, nil
}

func (b *backend) StoreHostsHealth(ctx context.Context, cid string, hosts map[string]models.HostHealth) error {
	key := cid + "/hosts"
	hostsUntyped := make(map[string]interface{}, len(hosts))
	for fqdn, host := range hosts {
		hostCopy := host
		hostsUntyped[fqdn] = &hostCopy
	}

	return b.client.Watch(ctx,
		func(tx *goredis.Tx) error {
			responseDel := tx.Del(ctx, key)
			if responseDel.Err() != nil {
				return responseDel.Err()
			}
			responseSet := tx.HMSet(ctx, key, hostsUntyped)
			if responseSet.Err() != nil {
				return responseSet.Err()
			}
			return tx.Expire(ctx, key, b.cfg.Expiration).Err()
		})
}

func (b *backend) LoadHostsHealth(ctx context.Context, cid string, fqdns []string) (map[string]models.HostHealth, error) {
	if len(fqdns) == 0 {
		return map[string]models.HostHealth{}, nil
	}
	response := b.client.HMGet(ctx, cid+"/hosts", fqdns...)
	resultSlice, err := response.Result()
	if err != nil {
		return nil, response.Err()
	}

	hostsHealth := make(map[string]models.HostHealth, len(resultSlice))
	for _, data := range resultSlice {
		if data == nil {
			continue
		}
		host := models.HostHealth{}
		err = host.UnmarshalBinary([]byte(data.(string)))
		if err != nil {
			b.logger.Errorf("Host unmarshall error: %s", err)
			continue
		}
		hostsHealth[host.Fqdn] = host
	}

	// Fill unknown for not found or not unmarshaled hosts
	for _, fqdn := range fqdns {
		if _, exists := hostsHealth[fqdn]; !exists {
			hostsHealth[fqdn] = models.NewHostUnknownHealth()
		}
	}

	return hostsHealth, nil
}

func (b *backend) StoreClusterTopology(ctx context.Context, cid string, topology models.ClusterTopology) error {
	// Long expiration for topology. Invalidation by revision
	result := b.client.Set(ctx, cid+"/topology", &topology, time.Hour)
	return result.Err()
}

func (b *backend) GetCachedClusterTopology(ctx context.Context, cid string) (models.ClusterTopology, error) {
	key := cid + "/topology"
	result := b.client.Get(ctx, key)
	data, err := result.Bytes()
	ch := models.ClusterTopology{}
	if err != nil {
		if err == goredis.Nil {
			return models.ClusterTopology{}, datastore.ErrNotFound
		}
		return models.ClusterTopology{}, result.Err()
	}

	err = ch.UnmarshalBinary(data)
	if err != nil {
		return models.ClusterTopology{}, err
	}

	// Move expiration time for topology after successful get
	b.client.Expire(ctx, key, time.Hour)
	return ch, nil
}

func (b *backend) DeleteDecommissionHosts(ctx context.Context, cid string) error {
	result := b.client.Del(ctx, cid+"/decommission")
	return result.Err()
}

func (b *backend) StoreDecommissionHosts(ctx context.Context, cid string, decommissionHosts models.DecommissionHosts) error {
	result := b.client.Set(ctx, cid+"/decommission", &decommissionHosts, b.cfg.DecommissionExpirationTime)
	return result.Err()
}

func (b *backend) LoadDecommissionHosts(ctx context.Context, cid string) (models.DecommissionHosts, error) {
	key := cid + "/decommission"
	result := b.client.Get(ctx, key)
	data, err := result.Bytes()
	hosts := models.DecommissionHosts{}
	if err != nil {
		if err == goredis.Nil {
			// It is an expected behaviour here not to return datastore.ErrNotFound as error
			// because absence of hosts to decommission is a default and normal state
			return models.DecommissionHosts{}, nil
		}
		return models.DecommissionHosts{}, result.Err()
	}

	err = hosts.UnmarshalBinary(data)
	if err != nil {
		return models.DecommissionHosts{}, err
	}

	return hosts, nil
}

func (b *backend) StoreDecommissionStatus(ctx context.Context, cid string, decommissionStatus models.DecommissionStatus) error {
	result := b.client.Set(ctx, cid+"/decommission_status", &decommissionStatus, b.cfg.DecommissionExpirationTime)
	return result.Err()
}

func (b *backend) LoadDecommissionStatus(ctx context.Context, cid string) (models.DecommissionStatus, error) {
	key := cid + "/decommission_status"
	result := b.client.Get(ctx, key)
	data, err := result.Bytes()
	status := models.DecommissionStatus{}
	if err != nil {
		if err == goredis.Nil {
			return models.DecommissionStatus{}, datastore.ErrNotFound
		}
		return models.DecommissionStatus{}, result.Err()
	}

	err = status.UnmarshalBinary(data)
	if err != nil {
		return models.DecommissionStatus{}, err
	}

	return status, nil
}
