package status

import (
	"context"
	"fmt"
	"strings"
	"sync"
	"time"

	"golang.org/x/sync/errgroup"

	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"

	"a.yandex-team.ru/cloud/marketplace/pkg/ctxtools"
)

type StatusCode int

const (
	StatusCodeOK StatusCode = iota
	StatusCodeWarn
	StatusCodeError
)

func (s StatusCode) String() string {
	switch s {
	case StatusCodeOK:
		return "ok"
	case StatusCodeWarn:
		return "warning"
	case StatusCodeError:
		return "error"
	default:
		panic(xerrors.New("wrong status code format"))
	}
}

const (
	defaultPollInterval = 10 * time.Second
)

type Status struct {
	Name        string
	Description string

	Code StatusCode
}

type StatusChecker interface {
	Check(ctx context.Context) Status
	Ping(ctx context.Context) error
}

type StatusCollector struct {
	checkers   []StatusChecker
	checkersMu sync.RWMutex

	statuses   []Status
	statusesMu sync.RWMutex

	config struct {
		pollInterval time.Duration
	}
}

type Config struct {
	PollInterval time.Duration
}

func NewStatusChecker(config Config, checkers ...StatusChecker) *StatusCollector {
	out := &StatusCollector{
		checkers: checkers,
	}

	out.config.pollInterval = config.PollInterval
	if out.config.pollInterval == 0 {
		out.config.pollInterval = defaultPollInterval
	}

	return out
}

func (s *StatusCollector) AddChecker(checker StatusChecker) {
	s.checkersMu.Lock()
	defer s.checkersMu.Unlock()

	s.checkers = append(s.checkers, checker)
}

func (s *StatusCollector) Statuses() (out []Status) {
	s.statusesMu.RLock()
	defer s.statusesMu.RUnlock()

	if len(s.statuses) == 0 {
		return
	}

	out = make([]Status, len(s.statuses))
	copy(out, s.statuses)

	return
}

func (s *StatusCollector) OverviewStatus() Status {
	const overallStatusName = "yc-mkt-license-check"
	var (
		hasErrors   bool
		hasWarnings bool

		names        []string
		descriptions []string
	)

	for _, s := range s.Statuses() {
		switch s.Code {
		case StatusCodeError:
			hasErrors = true
		case StatusCodeWarn:
			hasWarnings = true
		}

		names = append(names, s.Name)
		descriptions = append(names, s.Description)
	}

	overallStatusCode := StatusCodeOK

	switch {
	case hasErrors:
		overallStatusCode = StatusCodeError
	case hasWarnings:
		overallStatusCode = StatusCodeWarn
	}

	var descBuilder strings.Builder

	for i := range names {
		fmt.Fprintf(&descBuilder, "%s: %s", names[i], descriptions[i])
		if i != len(names)-1 {
			_, _ = descBuilder.WriteString(", ")
		}
	}

	return Status{
		Code:        overallStatusCode,
		Name:        overallStatusName,
		Description: descBuilder.String(),
	}
}

func (s *StatusCollector) Run(runCtx context.Context) error {
	group, ctx := errgroup.WithContext(runCtx)

	group.Go(func() error {
		s.gatherStatus(ctx)
		ticker := time.NewTicker(s.config.pollInterval)
		defer ticker.Stop()

		for {
			select {
			case <-ticker.C:
				s.gatherStatus(ctx)
			case <-ctx.Done():
				return ctx.Err()
			}
		}
	})

	return group.Wait()
}

func (s *StatusCollector) gatherStatus(ctx context.Context) {
	var statuses []Status

	{
		s.checkersMu.RLock()
		defer s.checkersMu.RUnlock()

		for i := range s.checkers {
			statuses = append(statuses, s.checkers[i].Check(ctx))
		}
	}

	scoppedLogger := ctxtools.Logger(ctx)
	scoppedLogger.Debug("collected monitoring statuses", log.Int("count", len(statuses)))

	{
		s.statusesMu.Lock()
		defer s.statusesMu.Unlock()

		s.statuses = statuses
	}
}
