package cherrypy

import (
	"context"
	"encoding/json"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/saltapi"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type SaltUtil struct {
	c *Client
}

var _ saltapi.SaltUtil = &SaltUtil{}

func (su *SaltUtil) SyncAll(ctx context.Context, sec saltapi.Secrets, timeout time.Duration, target string) error {
	return su.c.Run(ctx, sec, timeout, target, "saltutil.sync_all")
}

func (su *SaltUtil) AsyncSyncAll(ctx context.Context, sec saltapi.Secrets, timeout time.Duration, target string) (string, error) {
	return su.c.AsyncRun(ctx, sec, timeout, target, "saltutil.sync_all")
}

func (su *SaltUtil) IsRunning(ctx context.Context, sec saltapi.Secrets, timeout time.Duration, target, fun string) (map[string][]saltapi.RunningFunc, error) {
	ret, err := su.c.run(ctx, sec, timeout, runArgs{target: target, fun: "saltutil.is_running", args: []string{fun}})
	if err != nil {
		return nil, err
	}

	mr, err := parseMinionsList(ret)
	if err != nil {
		return nil, err
	}

	return parseRunningFuncResponse(mr)
}

func parseRunningFuncResponse(mr map[string]json.RawMessage) (map[string][]saltapi.RunningFunc, error) {
	res := make(map[string][]saltapi.RunningFunc, len(mr))
	for k, v := range mr {
		// Parse real response
		var rf []saltapi.RunningFunc
		if err := json.Unmarshal(v, &rf); err != nil {
			return nil, xerrors.Errorf("failed to unmarshal running function response %q: %w", v, err)
		}

		res[k] = rf
	}

	return res, nil
}

func (su *SaltUtil) AsyncIsRunning(ctx context.Context, sec saltapi.Secrets, timeout time.Duration, target, fun string) (string, error) {
	return su.c.AsyncRun(ctx, sec, timeout, target, "saltutil.is_running", fun)
}

func (su *SaltUtil) FindJob(ctx context.Context, sec saltapi.Secrets, timeout time.Duration, target, jid string) (map[string]saltapi.RunningFunc, error) {
	ret, err := su.c.run(ctx, sec, timeout, runArgs{target: target, fun: "saltutil.find_job", args: []string{jid}})
	if err != nil {
		return nil, err
	}

	var res map[string]saltapi.RunningFunc
	if err = json.Unmarshal(ret, &res); err != nil {
		return nil, xerrors.Errorf("failed to unmarshal saltutil.find_job response: %w", err)
	}

	return res, nil
}

func (su *SaltUtil) AsyncFindJob(ctx context.Context, sec saltapi.Secrets, timeout time.Duration, target, jid string) (string, error) {
	return su.c.AsyncRun(ctx, sec, timeout, target, "saltutil.find_job", jid)
}

func (su *SaltUtil) TermJob(ctx context.Context, sec saltapi.Secrets, timeout time.Duration, target, jid string) error {
	return su.c.Run(ctx, sec, timeout, target, "saltutil.term_job", jid)
}

func (su *SaltUtil) AsyncTermJob(ctx context.Context, sec saltapi.Secrets, timeout time.Duration, target, jid string) (string, error) {
	return su.c.AsyncRun(ctx, sec, timeout, target, "saltutil.term_job", jid)
}

func (su *SaltUtil) KillJob(ctx context.Context, sec saltapi.Secrets, timeout time.Duration, target, jid string) error {
	return su.c.Run(ctx, sec, timeout, target, "saltutil.kill_job", jid)
}

func (su *SaltUtil) AsyncKillJob(ctx context.Context, sec saltapi.Secrets, timeout time.Duration, target, jid string) (string, error) {
	return su.c.AsyncRun(ctx, sec, timeout, target, "saltutil.kill_job", jid)
}
