package juggler

import (
	"context"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/juggler"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type JugglerChecker struct {
	jugAPI       juggler.API
	checkHost    string
	checkService string
	reqStability time.Duration
}

func NewJugglerChecker(api juggler.API, checkHost, checkService string, reqStability time.Duration) *JugglerChecker {
	return &JugglerChecker{
		jugAPI:       api,
		checkHost:    checkHost,
		checkService: checkService,
		reqStability: reqStability,
	}
}

func (jc *JugglerChecker) IsStable(ctx context.Context, since, now time.Time) error {
	timePassed := now.Sub(since)
	if timePassed < jc.reqStability {
		return xerrors.Errorf("too early to check, need %v more", jc.reqStability-timePassed)
	}
	s, err := getFirstState(jc.jugAPI, ctx, jc.checkHost, jc.checkService)
	if err != nil {
		return xerrors.Errorf("failed to get juggler state: %w", err)
	}
	if s.Status == "OK" {
		if stableTime := now.Sub(s.ChangeTime); stableTime < jc.reqStability {
			return xerrors.Errorf("juggler check is %s, but need %v more stable time", s.Status, jc.reqStability-stableTime)
		}
		return nil // everything's fine
	} else {
		return xerrors.Errorf("juggler check is %s", s.Status)
	}
}

func getFirstState(jugCli juggler.API, ctx context.Context, host, service string) (juggler.CheckState, error) {
	states, err := jugCli.GetChecksState(ctx, host, service)
	if err != nil {
		return juggler.CheckState{}, err
	}
	for _, s := range states {
		return s, nil
	}
	return juggler.CheckState{}, xerrors.Errorf("no states for host %s and service %s", host, service)
}
