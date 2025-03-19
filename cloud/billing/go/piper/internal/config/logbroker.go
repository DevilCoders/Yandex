//go:build cgo
// +build cgo

package config

import (
	"context"
	"errors"
	"fmt"
	"time"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/config/cfgtypes"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling/logf"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling/logging"
	lbks "a.yandex-team.ru/cloud/billing/go/pkg/logbroker"
	"a.yandex-team.ru/cloud/billing/go/pkg/logbroker/lbtypes"
	"a.yandex-team.ru/kikimr/public/sdk/go/persqueue"
	lblog "a.yandex-team.ru/kikimr/public/sdk/go/persqueue/log"
	"a.yandex-team.ru/library/go/core/log"
)

func (c *Container) makeLogbrokerService(
	name string, instance int, cfg cfgtypes.LogbrokerSourceConfig, cons consumerSelector,
	handler lbtypes.Handler, offsets lbtypes.OffsetReporter,
) (logbrokerServer, error) {
	c.debug("logbroker reader construction")
	c.debug("logbroker reader installation", logf.Value(cfg.Installation))
	instCfg, err := c.GetLbInstallationConfig(cfg.Installation)
	if err != nil {
		return nil, err
	}

	fields := []log.Field{logf.Service("logbroker-reader"), logf.SourceName(name)}

	// Use simple logger for service
	logger := log.With(logging.LibsLogger(true), fields...)
	// and modifying logger for sdk
	lbLogger := lbLogger{l: log.With(logging.LibsLogger(false), fields...)}

	c.debug("logbroker reader auth type", logf.Value(instCfg.Auth.Type.String()))
	auth, err := c.getAuth(instCfg.Auth)
	if err != nil {
		return nil, err
	}

	consumer := cons(instCfg)

	c.debug("logbroker reader endpoint", logf.Value(fmt.Sprintf("%s:%d", instCfg.Host, instCfg.Port)))
	c.debug("logbroker reader database", logf.Value(instCfg.Database))
	c.debug("logbroker reader consumer", logf.Value(consumer))
	c.debug("logbroker reader topic", logf.Value(cfg.Topic))
	params := []lbks.ConsumerOption{
		lbks.Credentials(auth.YDBAuth()),
		lbks.Database(instCfg.Database),
		lbks.Endpoint(instCfg.Host, instCfg.Port),
		lbks.Logger(logger),
		lbks.ReaderLogger(&lbLogger),
		lbks.ConsumeTopic(consumer, cfg.Topic),
		lbks.ConsumeLimits(uint32(cfg.MaxMessages), uint32(cfg.MaxSize), 0, cfg.Lag.Duration()),
		lbks.ReadLimits(cfg.BatchTimeout.Duration(), uint32(cfg.BatchLimit), int(cfg.BatchSize)),
		lbks.SkipFatals(),
	}
	if !instCfg.DisableTLS.Bool() {
		c.debug("logbroker reader tls enabled")
		tls, err := c.GetTLS()
		if err != nil {
			return nil, err
		}
		params = append(params, lbks.TLS(tls))
	}

	srv, err := lbks.NewConsumerService(handler, offsets, params...)
	if err != nil {
		return nil, c.failInitF("create logbroker service %s: %w", name, err)
	}
	c.debug("logbroker reader constructed")

	return logbrokerServerWrapper{srv}, nil
}

func (c *Container) makeLBWriter(cfg cfgtypes.LogbrokerSinkConfig) (lbtypes.ShardProducer, error) {
	if !cfg.Enabled.Bool() {
		return c.dummy(), nil
	}
	c.debug("logbroker writer construction")
	c.debug("logbroker installation", logf.Value(cfg.Installation))

	instConfig, err := c.GetLbInstallationConfig(cfg.Installation)
	if err != nil {
		return nil, err
	}
	if instConfig.Host == "" {
		return nil, fmt.Errorf("unknown logbroker installation %s", cfg.Installation)
	}

	c.debug("logbroker auth type", logf.Value(instConfig.Auth.Type.String()))
	auth, err := c.getAuth(instConfig.Auth)
	if err != nil {
		return nil, err
	}

	// TODO: add debug parameter if needed INFO and DEBUG levels
	logger := log.With(logging.LibsLogger(false), logf.LBTopic(cfg.Topic))
	c.debug("logbroker endpoint", logf.Value(fmt.Sprintf("%s:%d", instConfig.Host, instConfig.Port)))
	c.debug("logbroker database", logf.Value(instConfig.Database))
	c.debug("logbroker topic", logf.Value(cfg.Topic))
	wo := persqueue.WriterOptions{
		Credentials: auth.YDBAuth(),
		Database:    instConfig.Database,
		Endpoint:    instConfig.Host,
		Port:        instConfig.Port,
		Topic:       cfg.Topic,
		Logger:      &lbLogger{l: logger},
	}

	if !instConfig.DisableTLS.Bool() {
		c.debug("logbroker tls enabled")
		tls, err := c.GetTLS()
		if err != nil {
			return nil, err
		}
		wo.TLSConfig = tls
	}
	if !cfg.Compression.Disabled.Bool() {
		wo.Codec = persqueue.Gzip
		wo.CompressionLevel = cfg.Compression.Level
	} else {
		c.debug("logbroker comperssion disabled")
	}

	return lbks.NewShardProducer(wo, cfg.Route, cfg.Partitions, cfg.SplitSize.Int(), cfg.MaxParallel)
}

type logbrokerServerWrapper struct {
	*lbks.ConsumerService
}

func (w logbrokerServerWrapper) HealthCheck(context.Context) error {
	stats := w.ConsumerService.Stats()
	switch stats.Status {
	case lbtypes.ServiceError:
		return errors.New("logbroker consumer service error")
	case lbtypes.ServiceNotRunning:
		return errors.New("logbroker consumer service not running")
	case lbtypes.ServiceShutdown:
		return errors.New("logbroker consumer service shutting down")
	}
	return nil
}

func (w logbrokerServerWrapper) GetStats() lbtypes.ServiceCounters {
	return w.ConsumerService.Stats()
}

func (w logbrokerServerWrapper) Close() error {
	ctx, cancel := context.WithTimeout(context.Background(), time.Second*10)
	defer cancel()
	return w.ConsumerService.Stop(ctx)
}

var _ lblog.Logger = &lbLogger{}

// lbLogger is custom adapter for logbroker writer logging.
// In out case context cancelation does not deserves log.Error level.
type lbLogger struct {
	l log.Logger
}

func (l *lbLogger) Log(ctx context.Context, lvl lblog.Level, message string, data map[string]interface{}) {
	if lvl == lblog.LevelNone {
		return
	}

	f := logging.GetFields()
	defer logging.PutFields(f)

	if requestID := tooling.ExposeRequestID(ctx); requestID != "" {
		f.Fields = append(f.Fields, logf.RequestID(requestID))
	}

	for k, v := range data {
		f.Fields = append(f.Fields, log.Any(k, v))
	}

	switch lvl {
	case lblog.LevelTrace:
		l.l.Trace(message, f.Fields...)
	case lblog.LevelDebug:
		l.l.Debug(message, f.Fields...)
	case lblog.LevelInfo:
		l.l.Info(message, f.Fields...)
	case lblog.LevelWarn:
		l.l.Warn(message, f.Fields...)
	case lblog.LevelError:
		// shift log level for canceled context
		if ctx.Err() != nil {
			l.l.Warn(message, f.Fields...)
		} else {
			l.l.Error(message, f.Fields...)
		}
	case lblog.LevelFatal:
		l.l.Fatal(message, f.Fields...)
	}
}

func (l *lbLogger) With(fieldName string, fieldValue interface{}) lblog.Logger {
	return &lbLogger{
		l: log.With(l.l, log.Any(fieldName, fieldValue)),
	}
}
