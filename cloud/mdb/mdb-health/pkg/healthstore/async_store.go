package healthstore

import (
	"context"
	"sync"
	"time"

	"github.com/opentracing/opentracing-go"
	tracelog "github.com/opentracing/opentracing-go/log"

	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/tracing/tags"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

type Store struct {
	l        log.Logger
	backends []Backend

	cfg      Config
	dataLock sync.Locker
	data     []HostHealthToStore
}

func NewStore(backends []Backend, cfg Config, l log.Logger) *Store {
	store := &Store{
		backends: backends,
		l:        l,
		dataLock: &sync.Mutex{},
		cfg:      cfg,
		data:     newStoreData(cfg),
	}
	return store
}

func (s *Store) StoreHostHealth(ctx context.Context, health types.HostHealth, ttl time.Duration) error {
	span, _ := opentracing.StartSpanFromContext(ctx, "StoreHostHealth")
	defer span.Finish()

	s.dataLock.Lock()
	defer s.dataLock.Unlock()

	if len(s.data) >= s.cfg.MaxBuffSize {
		stats.storeErrors.Inc()
		return semerr.Unavailablef("buffer size overflow: MaxBuffSize %d", s.cfg.MaxBuffSize)
	}

	s.data = append(s.data, HostHealthToStore{Health: health, TTL: ttl})
	return nil
}

func (s *Store) Run(ctx context.Context) {
	ctxlog.Info(ctx, s.l, "start store cycle")
	defer func() {
		ctxlog.Info(ctx, s.l, "stop store cycle")
	}()

	ticker := time.NewTicker(s.cfg.MaxWaitTime.Duration)
	defer ticker.Stop()

	var i int
	for func() bool {
		if s.cfg.MaxCycles == 0 {
			return true
		} else if i >= s.cfg.MaxCycles {
			return false
		} else {
			i++
			return true
		}
	}() {
		var iterCtx context.Context
		if s.cfg.MaxCycles > 0 {
			iterCtx = ctxlog.WithFields(ctx, log.Int("iteration number", i))
		} else {
			iterCtx = ctx
		}
		select {
		case <-ticker.C:
			s.doStore(iterCtx)

		case <-ctx.Done():
			return
		}
	}
}

func (s *Store) retrieveData(ctx context.Context) []HostHealthToStore {
	span, _ := opentracing.StartSpanFromContext(ctx, "Retrieve Data")
	defer span.Finish()

	newData := newStoreData(s.cfg)

	s.dataLock.Lock()
	defer s.dataLock.Unlock()

	data := s.data
	s.data = newData
	return data
}

func (s *Store) doStore(ctx context.Context) {
	startTime := time.Now()
	span, ctx := opentracing.StartSpanFromContext(ctx, "Store Data")
	defer span.Finish()

	data := s.retrieveData(ctx)
	batchSize := len(data)
	span.SetTag("batchSize", batchSize)
	stats.batchSize.Set(float64(batchSize))

	if batchSize > 0 {
		ctxlog.Debug(ctx, s.l, "start storing data to backends", log.Int("batchSize", batchSize))

		wg := &sync.WaitGroup{}
		wg.Add(len(s.backends))
		for _, backend := range s.backends {
			go s.storeDataInBackend(ctx, backend, data, wg)
		}
		wg.Wait()
		ctxlog.Debug(ctx, s.l, "finish storing data to backends")
	} else {
		ctxlog.Debug(ctx, s.l, "nothing to store", log.Int("batchSize", batchSize))
	}

	d := time.Since(startTime)
	stats.iterationTime.Observe(d.Seconds())
	if d > s.cfg.MaxWaitTime.Duration {
		stats.iterationTimeOverLimit.Observe(d.Seconds() - s.cfg.MaxWaitTime.Seconds())
	} else {
		stats.iterationTimeOverLimit.Observe(0)
	}
}

func (s *Store) storeDataInBackend(ctx context.Context, backend Backend, data []HostHealthToStore, wg *sync.WaitGroup) {
	defer wg.Done()

	span, ctx := opentracing.StartSpanFromContext(ctx, "Store Data In backend", tags.Backend.Tag(backend.Name()))
	defer span.Finish()

	if err := backend.StoreHostsHealth(ctx, data); err != nil {
		ctxlog.Error(ctx, s.l, "can not store data", log.String("backend", backend.Name()), log.Error(err))
		stats.backendErrors.Inc()
		span := opentracing.SpanFromContext(ctx)
		span.LogFields(tracelog.Error(err), tracelog.String("backend", backend.Name()))
	}
}

func newStoreData(cfg Config) []HostHealthToStore {
	return make([]HostHealthToStore, 0, cfg.MinBufSize)
}
