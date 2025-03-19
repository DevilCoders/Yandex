package cherrypy

import (
	"context"
	"encoding/json"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/saltapi"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type State struct {
	c *Client
}

var _ saltapi.State = &State{}

func (s *State) Running(ctx context.Context, sec saltapi.Secrets, timeout time.Duration, target string) (map[string][]string, error) {
	ret, err := s.c.run(ctx, sec, timeout, runArgs{target: target, fun: "state.running"})
	if err != nil {
		return nil, err
	}

	var res map[string][]string
	if err = json.Unmarshal(ret, &res); err != nil {
		return nil, xerrors.Errorf("failed to unmarshal state.running response: %w", err)
	}

	return res, nil
}

func (s *State) Highstate(ctx context.Context, sec saltapi.Secrets, timeout time.Duration, target string, args ...string) error {
	return s.c.Run(ctx, sec, timeout, target, "state.highstate", args...)
}

func (s *State) AsyncHighstate(ctx context.Context, sec saltapi.Secrets, timeout time.Duration, target string, args ...string) (string, error) {
	return s.c.AsyncRun(ctx, sec, timeout, target, "state.highstate", args...)
}
