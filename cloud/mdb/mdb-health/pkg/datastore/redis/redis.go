package redis

import (
	"context"
	"fmt"
	"strconv"
	"strings"
	"time"

	goredis "github.com/go-redis/redis/v8"
	"github.com/opentracing/opentracing-go"

	"a.yandex-team.ru/cloud/mdb/internal/redis/tracing"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/datastore"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

// TODO конвертация ошибок в ошибки pkg/datastore

const (
	modeReadWrite      = "rw"
	maxExampleCount    = 10
	numOfSystemMetrics = 3

	clusterHealthArrayHeaderSize = 5
)

type topologyPrefix string

const (
	rolePrefix    topologyPrefix = "role:"
	shardidPrefix topologyPrefix = "sid:"
	geoPrefix     topologyPrefix = "geo:"

	servicePrefix             = "service:"
	modePrefix                = "mode:"
	systemMetricsPrefix       = "system:"
	slaPrefix                 = "sla_"
	readablePrefix            = "readable_"
	writablePrefix            = "writable_"
	userfaultBrokenPrefix     = "userfaultBroken_"
	redisUnavailableErrText   = "redis is not availble"
	redisRequestFailedErrText = "request in redis failed"
)

// TODO optimize key values for size?

type backend struct {
	logger      log.Logger
	client      goredis.UniversalClient
	slaveClient goredis.UniversalClient
	limRec      int
}

func (b *backend) Name() string {
	return "redis"
}

// TODO 1. specific handling of errors like goredis.ErrPoolTimeout and goredis.ErrClosed
// TODO 2. implement reading from redis in local datacenter
// TODO 3. support context (client.WithContext), not very useful right now as there is no cancellation support
// TODO    in goredis

// New constructor for Redis datastore
func New(logger log.Logger, cfg Config) datastore.Backend {
	logger.Infof(
		"Loading redis datastore backend with following config: addrs '%s', masterName '%s', db '%d', limit agg recs '%d'",
		cfg.Addrs,
		cfg.MasterName,
		cfg.DB,
		cfg.LimitAggRec,
	)

	clientOptions := UniversalClientOptions(cfg)
	client := goredis.NewUniversalClient(clientOptions)
	client.AddHook(&tracing.Tracer{})

	var slaveClient goredis.UniversalClient
	if len(cfg.Addrs) > 1 {
		slaveClientOptions := clientOptions.Failover()
		slaveClientOptions.SlaveOnly = true
		slaveClient = goredis.NewFailoverClient(slaveClientOptions)
		slaveClient.AddHook(&tracing.Tracer{})
	} else {
		slaveClient = client
	}

	return &backend{
		logger:      logger,
		client:      client,
		slaveClient: slaveClient,
		limRec:      cfg.LimitAggRec,
	}
}

func UniversalClientOptions(cfg Config) *goredis.UniversalOptions {
	return &goredis.UniversalOptions{
		Addrs:      cfg.Addrs,
		MasterName: cfg.MasterName,
		Username:   cfg.Username,
		Password:   cfg.Password.Unmask(),
		DB:         cfg.DB,
		ReadOnly:   true, // applied only if MasterName == ""

		MaxRetries:      cfg.MaxRetries,
		MinRetryBackoff: cfg.MinRetryBackoff,
		MaxRetryBackoff: cfg.MaxRetryBackoff,

		DialTimeout:  cfg.DialTimeout,
		ReadTimeout:  cfg.ReadTimeout,
		WriteTimeout: cfg.WriteTimeout,

		PoolFIFO:           cfg.PoolFIFO,
		PoolSize:           cfg.PoolSize,
		MinIdleConns:       cfg.MinIdleConns,
		MaxConnAge:         cfg.MaxConnAge,
		PoolTimeout:        cfg.PoolTimeout,
		IdleTimeout:        cfg.IdleTimeout,
		IdleCheckFrequency: cfg.IdleCheckFrequency,

		OnConnect: tracing.OnConnect,
	}
}

// Close database
func (b *backend) Close() error {
	if err := b.client.Close(); err != nil {
		return err
	}
	if err := b.slaveClient.Close(); err != nil {
		return err
	}
	return nil
}

func (b *backend) parseMatchedField(ctx context.Context, cid, field string, raw interface{}, prefix topologyPrefix, data map[string][]string) bool {
	value, ok := unmarshalTopologyField(prefix, field)
	if !ok {
		return false
	}
	fqdnsStr, ok := raw.(string)
	if !ok {
		ctxlog.Warnf(ctx, b.logger, "for cid %s invalid field %s content value '%v'", cid, field, raw)
		return true
	}
	data[value] = strings.Split(fqdnsStr, " ")
	return true
}

func (b *backend) IsReady(ctx context.Context) error {
	span, _ := opentracing.StartSpanFromContext(ctx, "IsReady")
	defer span.Finish()

	if err := b.client.Ping(ctx).Err(); err != nil {
		return semerr.WrapWithUnavailable(xerrors.Errorf("ping master: %w", err), "unavailable")
	}
	if err := b.slaveClient.Ping(ctx).Err(); err != nil {
		return semerr.WrapWithUnavailable(xerrors.Errorf("ping slave: %w", err), "unavailable")
	}
	return nil
}

func (b *backend) CaptureTheLead(ctx context.Context, fqdn string, now time.Time, dur time.Duration) (string, time.Time, error) {
	span, _ := opentracing.StartSpanFromContext(ctx, "CaptureTheLead")
	defer span.Finish()

	separator := "|"
	tt := now.Add(dur)
	r := fmt.Sprintf("%s%s%x", fqdn, separator, tt.Unix())
	set := b.client.SetNX(ctx, "lead", r, dur)
	if err := set.Err(); err != nil {
		return "", time.Time{}, err
	}
	if set.Val() {
		return fqdn, tt, nil
	}
	get := b.client.Get(ctx, "lead")
	if err := get.Err(); err != nil {
		return "", time.Time{}, err
	}
	v := get.Val()
	vs := strings.Split(v, separator)
	if len(vs) != 2 {
		return "", time.Time{}, xerrors.Errorf("redis invalid lead content: %s", v)
	}

	unixTime, err := strconv.ParseInt(vs[1], 16, 64)
	if err != nil {
		return "", time.Time{}, xerrors.Errorf("redis invalid lead content: %s", v)
	}

	return vs[0], time.Unix(unixTime, 0), nil
}
