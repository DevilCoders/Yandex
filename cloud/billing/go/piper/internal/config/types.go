package config

import (
	"context"
	"errors"
	"fmt"
	"sync"
	"time"

	"github.com/jmoiron/sqlx"
	"github.com/uber/jaeger-client-go"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/config/cfgtypes"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/core/entities"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/interconnect/auth"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling/logf"
	"a.yandex-team.ru/cloud/billing/go/pkg/logbroker/lbtypes"
	"a.yandex-team.ru/library/go/core/log"
)

var ErrNotConfigured = errors.New("not configured - dummy implementation")

type HTTPServer interface {
	ListenAndServe() error
}

type NamespaceConfigurator interface {
	GetConfig(ctx context.Context, namespace string) (map[string]string, error)
}

type YAMLConfigurator interface {
	GetYaml(ctx context.Context) ([]byte, error)
}

type FullConfigurator interface {
	NamespaceConfigurator
	YAMLConfigurator
}

type DBCluster interface {
	DB(partition int) (*sqlx.DB, error)
	Partitions() int
}

type dummyImpl struct {
	ctx context.Context
}

func (d dummyImpl) ListenAndServe() error {
	<-d.ctx.Done()
	return nil
}

func (dummyImpl) Token(context.Context) (string, error) {
	return "", nil
}

func (dummyImpl) GetRequestMetadata(ctx context.Context, uri ...string) (map[string]string, error) {
	return nil, nil
}

func (dummyImpl) RequireTransportSecurity() bool {
	return false
}

func (d dummyImpl) GRPCAuth() auth.GRPCAuthenticator {
	return d
}

func (d dummyImpl) YDBAuth() auth.YDBAuthenticator {
	return nil
}

func (dummyImpl) GetConfig(ctx context.Context, namespace string) (map[string]string, error) {
	return map[string]string{}, nil
}

func (dummyImpl) GetYaml(ctx context.Context) ([]byte, error) {
	return []byte(""), nil
}

func (dummyImpl) GetOffset(context.Context, lbtypes.SourceID) (uint64, error) {
	return 0, ErrNotConfigured
}

func (dummyImpl) Write(context.Context, lbtypes.SourceID, uint32, []lbtypes.ShardMessage) (uint64, error) {
	return 0, ErrNotConfigured
}

func (dummyImpl) PartitionsCount() uint32 {
	return 0
}

func (dummyImpl) ResolveAbc(context.Context, entities.ProcessingScope, []int64) ([]entities.AbcFolder, error) {
	return nil, nil
}

func (dummyImpl) FindDuplicates(context.Context, entities.ProcessingScope, time.Time, []entities.MetricIdentity) ([]entities.MetricIdentity, error) {
	return nil, nil
}

func (dummyImpl) DB(partition int) (*sqlx.DB, error) {
	return nil, ErrNotConfigured
}

func (dummyImpl) Partitions() int {
	return 0
}

func (dummyImpl) PushE2EUsageQuantityMetric(context.Context, entities.ProcessingScope, entities.E2EMetric) error {
	return nil
}

func (dummyImpl) PushE2EPricingQuantityMetric(context.Context, entities.ProcessingScope, entities.E2EMetric) error {
	return nil
}

func (dummyImpl) FlushE2EQuantityMetrics(context.Context) error {
	return nil
}

type initSync struct {
	once sync.Once
	err  error
}

func (s *initSync) Do(f func() error) error {
	s.once.Do(func() {
		s.err = f()
	})
	return s.err
}

type jaegerLogger struct {
	l log.Structured
}

func newJaegerLogger(l log.Structured) *jaegerLogger {
	return &jaegerLogger{l: l}
}

var (
	_              jaeger.Logger = &jaegerLogger{}
	jaegerLogField               = logf.Service("tracing")
)

// Error logs message at warning priority (tracing is not that important)
func (l *jaegerLogger) Error(msg string) {
	l.l.Warn(msg, jaegerLogField)
}

// Infof logs message at debug priority
func (l *jaegerLogger) Infof(msg string, args ...interface{}) {
	// They really are debug messages...
	l.l.Debug(fmt.Sprintf(msg, args...), jaegerLogField)
}

type logbrokerServer interface {
	Runner
	HealthCheck(context.Context) error
}

type consumerSelector func(cfgtypes.LbInstallationConfig) string

func constantConsumer(name string) consumerSelector {
	return func(cfgtypes.LbInstallationConfig) string { return name }
}

func resharderConsumer(name string) consumerSelector {
	return func(cfg cfgtypes.LbInstallationConfig) string {
		if name == "" {
			return cfg.ResharderConsumer
		}
		return name
	}
}
