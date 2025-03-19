package worker

import (
	"context"
	"time"

	"go.uber.org/atomic"
	"golang.org/x/sync/errgroup"

	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/actions/license/instances"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/services/env"
	"a.yandex-team.ru/cloud/marketplace/pkg/ctxtools"
	"a.yandex-team.ru/library/go/core/log"
)

type Service struct {
	baseService
	stopped *atomic.Bool
}

func NewService(env *env.Env) (*Service, error) {
	return &Service{
		baseService: baseService{env},
		stopped:     atomic.NewBool(false),
	}, nil
}

func (s *Service) Run(ctx context.Context) error {
	scoppedLogger := ctxtools.Logger(ctx)
	group, ctx := errgroup.WithContext(ctx)

	group.Go(func() error {
		scoppedLogger.Debug("running worker service")
		return s.demon(ctx)
	})

	group.Go(func() error {
		<-ctx.Done()
		scoppedLogger.Info("shutting down the worker service", log.Error(ctx.Err()))

		s.gracefulStop()
		return nil
	})

	return group.Wait()
}

func (s *Service) gracefulStop() {
	s.stopped.Store(true)
}

func (s *Service) demon(ctx context.Context) error {
	scoppedLogger := ctxtools.Logger(ctx)
	for !s.stopped.Load() {
		group, ctx := errgroup.WithContext(ctx)

		group.Go(func() error {
			res, err := instances.NewRecreateAction(s.Env).Do(ctx, instances.RecreateParams{})
			if err != nil {
				scoppedLogger.Error("worker: failed to execute recreate action", log.Error(err))
			} else {
				if res.Instances != nil {
					scoppedLogger.Info("instances recreated", log.Int("count", len(res.Instances)))
				} else {
					scoppedLogger.Debug("no new instances created")
					time.Sleep(time.Second)
				}
			}
			return nil
		})

		group.Go(func() error {
			res, err := instances.NewActivatePendingAction(s.Env).Do(ctx, instances.ActivatePendingParams{})
			if err != nil {
				scoppedLogger.Error("worker: failed to execute activate pending action", log.Error(err))
			} else {
				if res.Instances != nil {
					scoppedLogger.Info("instances activated", log.Int("count", len(res.Instances)))
				} else {
					scoppedLogger.Debug("no new instances activated")
					time.Sleep(time.Second)
				}
			}
			return nil
		})

		_ = group.Wait()
	}
	return nil
}
