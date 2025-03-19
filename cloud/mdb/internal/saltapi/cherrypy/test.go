package cherrypy

import (
	"context"
	"encoding/json"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/saltapi"
)

type Test struct {
	c *Client
}

var _ saltapi.Test = &Test{}

func (t *Test) Ping(ctx context.Context, sec saltapi.Secrets, timeout time.Duration, target string) (map[string]bool, error) {
	ret, err := t.c.run(ctx, sec, timeout, runArgs{target: target, fun: "test.ping"})
	if err != nil {
		return nil, err
	}

	var res modelRunPingResp
	if err = json.Unmarshal(ret, &res); err != nil {
		return nil, err
	}

	return res, nil
}

func (t *Test) AsyncPing(ctx context.Context, sec saltapi.Secrets, timeout time.Duration, target string) (string, error) {
	return t.c.AsyncRun(ctx, sec, timeout, target, "test.ping")
}

type modelRunPingResp = map[string]bool
