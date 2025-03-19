package cherrypy

import (
	"context"
	"encoding/json"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/saltapi"
)

type Config struct {
	c *Client
}

var _ saltapi.Config = &Config{}

func (cfg *Config) WorkerThreads(ctx context.Context, sec saltapi.Secrets, timeout time.Duration) (int32, error) {
	ret, err := cfg.c.run(ctx, sec, timeout, runArgs{client: clientTypeRunner, fun: "config.get", args: []string{"worker_threads"}})
	if err != nil {
		return 0, err
	}

	var res int32
	if err = json.Unmarshal(ret, &res); err != nil {
		return 0, err
	}

	return res, nil
}
