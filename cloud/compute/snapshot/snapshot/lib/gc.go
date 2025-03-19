package lib

import (
	"fmt"
	"sync"
	"time"

	"go.uber.org/zap"
	"golang.org/x/net/context"
	"golang.org/x/sync/errgroup"

	log "a.yandex-team.ru/cloud/compute/go-common/logging/ctxlog"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/common"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/config"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/misc"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/storage"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/logging"
)

const gcMaxIter = 1000

type gcIter struct {
	Index int
	Total int
}

type gcEntry struct {
	Info common.SnapshotInfo
	Iter gcIter
}

type gcStats struct {
	Err  error
	Iter gcIter
}

type statsMapValue struct {
	Total    int
	Received int
	Success  int
}

type statsMap struct {
	m         map[int]*statsMapValue
	lastFull  int
	lastInRow int
}

func newStatsMap() statsMap {
	return statsMap{make(map[int]*statsMapValue), -1, -1}
}

func (m *statsMap) UpdateCheckProcessed(stat gcStats) bool {
	// update counters
	if m.m[stat.Iter.Index] == nil {
		m.m[stat.Iter.Index] = &statsMapValue{Total: stat.Iter.Total}
	}
	s := m.m[stat.Iter.Index]
	s.Received++
	if stat.Err == nil {
		s.Success++
	}
	if m.m[stat.Iter.Index].Received == m.m[stat.Iter.Index].Total {
		// iteration is fully processed
		if stat.Iter.Index > m.lastFull {
			m.lastFull = stat.Iter.Index
		}
		// promote lastInRow checking that something was processed on each step
		prevLastInRow := m.lastInRow
		for i := m.lastInRow + 1; i <= m.lastFull; i++ {
			if s, ok := m.m[i]; !ok || s.Received < s.Total {
				break
			}
			// Check only on successful promotion,
			// since newer iterations may be blocked by older one
			if i == prevLastInRow+1 && m.m[i].Success == 0 {
				// Everything is locked, stop trying
				return false
			}
			delete(m.m, i)
			m.lastInRow = i
		}
	}
	return true
}

// GC is a garbage collector controller.
type GC struct {
	config *config.Config
	st     storage.Storage
	cancel chan struct{}
}

// NewGC creates a new GC from config.
func NewGC(ctx context.Context, conf *config.Config) (*GC, error) {
	if !conf.GC.Enabled {
		return nil, fmt.Errorf("GC is disabled in config")
	}

	st, err := PrepareStorage(ctx, conf, misc.TableOpNone)
	if err != nil {
		return nil, err
	}

	return &GC{
		config: conf,
		st:     st,
		cancel: make(chan struct{}),
	}, nil
}

// Run runs GC process until GC.cancel is Done.
func (gc *GC) Run(ctx context.Context) error {
	if gc.config.GC.Interval.Duration <= 0 {
		return fmt.Errorf("invalid duration %v", gc.config.GC.Interval.Duration)
	}

	t := time.NewTicker(gc.config.GC.Interval.Duration)
	defer t.Stop()
	for {
		select {
		case <-t.C:
			log.G(ctx).Debug("GC loop started")
			err := gc.Once(ctx)
			log.G(ctx).Debug("GC loop finished", zap.Error(err))
		case <-gc.cancel:
			return nil
		}
	}
}

// Once runs a full GC procedure once.
func (gc *GC) Once(ctx context.Context) error {
	infos := make(chan gcEntry)
	stats := make(chan gcStats)
	g, ctx := errgroup.WithContext(ctx)

	workers := gc.config.Performance.GCWorkers
	if workers <= 0 {
		workers = 1
	}
	var wg sync.WaitGroup
	wg.Add(workers + 1)

	// Consumers
	for i := 0; i < workers; i++ {
		g.Go(func() error {
			defer wg.Done()
			for {
				select {
				case entry, ok := <-infos:
					if !ok {
						return nil
					}

					var err error
					if entry.Info.State.Code == storage.StateDeleted {
						err = gc.processTombstone(ctx, entry.Info)
					} else {
						err = gc.processSnapshot(ctx, entry.Info)
					}

					switch err {
					case nil, misc.ErrAlreadyLocked:
						select {
						case stats <- gcStats{err, entry.Iter}:
						case <-ctx.Done():
							return ctx.Err()
						}
					default:
						return err
					}
				case <-ctx.Done():
					return ctx.Err()
				}
			}
		})
	}

	// Producer
	g.Go(func() error {
		defer wg.Done()
		for i := 0; i < gcMaxIter; i++ {
			l, err := gc.st.ListGC(ctx, &storage.GCListRequest{
				N:                gc.config.GC.BatchSize,
				FailedCreation:   gc.config.GC.FailedCreation.Duration,
				FailedConversion: gc.config.GC.FailedConversion.Duration,
				FailedDeletion:   gc.config.GC.FailedDeletion.Duration,
				Tombstone:        gc.config.GC.Tombstone.Duration,
			})
			if err != nil {
				return err
			}
			log.G(ctx).Debug("GC: delete", zap.Any("snapshots", l))

			for _, info := range l {
				select {
				case infos <- gcEntry{info, gcIter{i, len(l)}}:
				case <-ctx.Done():
					return ctx.Err()
				}
			}

			// No snapshots were successfully deleted
			if len(l) == 0 {
				close(infos)
				return nil
			}
		}
		log.G(ctx).Warn("GC: too many producer iterations", zap.Int("max", gcMaxIter))
		close(infos)
		return nil
	})

	// Counter
	g.Go(func() error {
		sm := newStatsMap()
		for {
			select {
			case stat, ok := <-stats:
				if !ok {
					return nil
				}
				log.G(ctx).Debug("Received stats", zap.Any("stats", stat))
				if !sm.UpdateCheckProcessed(stat) {
					return misc.ErrNothingProcessed
				}
			case <-ctx.Done():
				return ctx.Err()
			}
		}
	})

	// In case of successful exit, we must wait for workers before closing stats
	wg.Wait()
	close(stats)

	return g.Wait()
}

func (gc *GC) processSnapshot(ctx context.Context, i common.SnapshotInfo) error {
	ctx = log.WithLogger(ctx, log.GetLogger(ctx).With(logging.SnapshotID(i.ID)))
	log.G(ctx).Debug("GC: processSnapshot", logging.Request(i))

	// Consider partially deleted
	err := gc.st.BeginDeleteSnapshot(ctx, i.ID)
	switch err {
	case nil:
	case misc.ErrSnapshotNotFound:
		if i.State.Code == storage.StateCreating {
			// Someone else took this
			return nil
		}
		// Partial delete, collect it
	default:
		return err
	}

	return gc.st.EndDeleteSnapshotSync(ctx, i.ID, false)
}

func (gc *GC) processTombstone(ctx context.Context, i common.SnapshotInfo) error {
	ctx = log.WithLogger(ctx, log.GetLogger(ctx).With(logging.SnapshotID(i.ID)))
	log.G(ctx).Debug("GC: processTombstone", logging.Request(i))

	// Consider partially deleted
	err := gc.st.DeleteTombstone(ctx, i.ID)
	switch err {
	case misc.ErrSnapshotNotFound:
		return nil
	default:
		return err
	}
}

// Stop stops infinite garbage collection.
func (gc *GC) Stop() {
	gc.cancel <- struct{}{}
}

// Close releases GC-related resources.
func (gc *GC) Close() {
	if err := gc.st.Close(); err != nil {
		zap.L().Warn("Storage.Close failed", zap.Error(err))
	}
}
