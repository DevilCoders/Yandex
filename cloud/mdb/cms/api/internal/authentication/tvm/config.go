package tvm

import (
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/authentication"
	"a.yandex-team.ru/cloud/mdb/internal/auth/tvm/tvmtool"
	"a.yandex-team.ru/library/go/core/log"
)

func NewFromConfig(cfg tvmtool.Config, l log.Logger) (authentication.Authenticator, error) {
	client, err := tvmtool.NewFromConfig(cfg, l)
	if err != nil {
		return nil, err
	}

	return &TvmAuthenticator{
		client: *client,
	}, nil
}
