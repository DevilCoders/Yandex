package http

import (
	"a.yandex-team.ru/cloud/mdb/internal/httputil"
	"a.yandex-team.ru/cloud/mdb/internal/juggler/push"
	"a.yandex-team.ru/library/go/core/log"
)

const (
	localJugglerURL = "http://localhost:31579/events"
)

type Pusher struct {
	client *httputil.Client
	url    string
}

func NewLocalPusher(l log.Logger) (push.Pusher, error) {
	client, err := httputil.NewClient(httputil.ClientConfig{Name: "local_juggler_pusher"}, l)
	if err != nil {
		return nil, err
	}
	return &Pusher{
		client: client,
		url:    localJugglerURL,
	}, nil
}
