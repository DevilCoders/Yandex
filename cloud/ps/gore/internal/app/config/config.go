package config

import (
	"context"
	"os"

	"a.yandex-team.ru/library/go/core/log/zap"

	"a.yandex-team.ru/cloud/ps/gore/internal/pkg/abcclient"
	"a.yandex-team.ru/cloud/ps/gore/internal/pkg/calendarclient"
	"a.yandex-team.ru/cloud/ps/gore/internal/pkg/csv"
	"a.yandex-team.ru/cloud/ps/gore/internal/pkg/golemclient"
	"a.yandex-team.ru/cloud/ps/gore/internal/pkg/idmclient"
	"a.yandex-team.ru/cloud/ps/gore/internal/pkg/jugglerclient"
	"a.yandex-team.ru/cloud/ps/gore/internal/pkg/mongoclient"
	"a.yandex-team.ru/cloud/ps/gore/internal/pkg/stclient"

	"github.com/heetch/confita"
	"github.com/heetch/confita/backend"
	"github.com/heetch/confita/backend/env"
	"github.com/heetch/confita/backend/file"
	"github.com/heetch/confita/backend/flags"
)

type Config struct {
	Logger *zap.Logger

	Startrek *stclient.Config       `config:"Startrek"`
	Calendar *calendarclient.Config `config:"Calendar"`
	Golem    *golemclient.Config    `config:"Golem"`
	IDM      *idmclient.Config      `config:"IDM"`
	Juggler  *jugglerclient.Config  `config:"Juggler"`
	CSV      *csv.Config            `config:"CSV"`
	Mongo    *mongoclient.Config    `config:"MongoDB"`
	ABC      *abcclient.Config      `config:"ABC"`
}

func (c *Config) LoadConfigs() (err error) {
	bs := []backend.Backend{}
	_, err = os.Stat("/etc/gore/config.json")
	if err == nil {
		bs = append(bs, file.NewBackend("/etc/gore/config.json"))
	}

	bs = append(bs, env.NewBackend(), flags.NewBackend())
	loader := confita.NewLoader(bs...)
	return loader.Load(context.Background(), c)
}
