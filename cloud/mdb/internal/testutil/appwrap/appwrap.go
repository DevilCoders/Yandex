// Tiny wrapper around a App.
// It initialize database and run app in new goroutine.
package appwrap

import (
	"context"
	"fmt"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/pgutil"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/x/yandex/hasql/tracers"
)

var appNum int

// App ...
type App interface {
	Run(context.Context) error
}

type AppConstructor func(context.Context, log.Logger, *sqlutil.Cluster) (App, error)

// Wrap is wrapper around app
type Wrap struct {
	App     App
	cluster *sqlutil.Cluster
	cancel  context.CancelFunc
	stopped bool
}

// Stop stop the App and close cluster
func (w *Wrap) Stop() {
	if !w.stopped {
		w.cancel()
		_ = w.cluster.Close()
		w.stopped = true
	}
}

// New create new Wrap around App.
// Initialize cluster form clusterCfg, initialize App. Call App.Run in goroutine.
func New(parentCtx context.Context, clusterCfg pgutil.Config, constructor AppConstructor) (*Wrap, error) {
	appCtx, appCancel := context.WithTimeout(parentCtx, time.Hour)
	appNum++

	wrap, initErr := func() (*Wrap, error) {
		baseLogger, err := zap.New(zap.KVConfig(log.DebugLevel))
		if err != nil {
			return nil, xerrors.Errorf("failed to init logger: %w", err)
		}
		logger := log.With(baseLogger, log.String("name", fmt.Sprintf("app-%d", appNum)))

		cluster, err := pgutil.NewCluster(clusterCfg, sqlutil.WithTracer(tracers.Log(logger)))
		if err != nil {
			return nil, xerrors.Errorf("unable to construct cluster: %w", err)
		}

		app, err := constructor(appCtx, logger, cluster)
		if err != nil {
			if closeErr := cluster.Close(); closeErr != nil {
				logger.Warnf("close cluster with %s", closeErr)
			}
			return nil, xerrors.Errorf("app construction failed with: %w", err)
		}

		wrap := Wrap{
			App:     app,
			cancel:  appCancel,
			cluster: cluster,
		}
		return &wrap, nil
	}()
	if initErr != nil {
		appCancel()
		return nil, initErr
	}

	go func() {
		if err := wrap.App.Run(appCtx); err != nil {
			// don't panic on context errors, cause we initiate them with Wrap.Stop
			if xerrors.Is(err, context.Canceled) && xerrors.Is(err, context.DeadlineExceeded) {
				panic(err.Error())
			}
		}
	}()

	return wrap, nil
}
