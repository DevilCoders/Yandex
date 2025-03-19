package internal

import (
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/duty"
	grpcinstanceclient "a.yandex-team.ru/cloud/mdb/cms/api/pkg/instanceclient/grpc"
	"a.yandex-team.ru/cloud/mdb/deploy/api/pkg/deployapi/restapi"
	"a.yandex-team.ru/cloud/mdb/internal/app"
	"a.yandex-team.ru/cloud/mdb/internal/app/swagger"
	"a.yandex-team.ru/cloud/mdb/internal/auth/httpauth/blackboxauth"
	"a.yandex-team.ru/cloud/mdb/internal/conductor/httpapi"
	dbmapi "a.yandex-team.ru/cloud/mdb/internal/dbm/restapi"
	jugglerclient "a.yandex-team.ru/cloud/mdb/internal/juggler/http"
	healthclt "a.yandex-team.ru/cloud/mdb/mdb-health/pkg/client/swagger"
	grpcmlockclient "a.yandex-team.ru/cloud/mdb/mlock/pkg/mlockclient/grpc"
)

const (
	AppCfgName = "mdb-cms"
)

type Config struct {
	Conductor httpapi.ConductorConfig   `json:"conductor" yaml:"conductor"`
	Mlock     grpcmlockclient.Config    `json:"mlock" yaml:"mlock"`
	App       app.Config                `json:"app" yaml:"app"`
	Swagger   swagger.SwaggerConfig     `json:"swagger" yaml:"swagger"`
	Auth      blackboxauth.Config       `json:"auth" yaml:"auth"`
	Dbm       dbmapi.Config             `json:"dbm" yaml:"dbm"`
	Health    healthclt.Config          `json:"health" yaml:"health"`
	Cms       duty.CmsDom0DutyConfig    `json:"cms" yaml:"cms"`
	Juggler   jugglerclient.Config      `json:"juggler" yaml:"juggler"`
	Deploy    restapi.Config            `json:"deploy" yaml:"deploy"`
	Instance  grpcinstanceclient.Config `json:"instance" yaml:"instance"`
}

var _ app.AppConfig = &Config{}

func (c *Config) AppConfig() *app.Config {
	return &c.App
}

func DefaultConfig() Config {
	cfg := Config{
		Conductor: httpapi.DefaultConductorConfig(),
		App:       app.DefaultConfig(),
		Swagger:   swagger.DefaultSwaggerConfig(),
		Juggler:   jugglerclient.DefaultConfig(),
		Deploy:    restapi.DefaultConfig(),
		Dbm:       dbmapi.DefaultConfig(),
		Mlock:     grpcmlockclient.DefaultConfig(),
		Cms:       duty.DefaultConfig(),
		Instance:  grpcinstanceclient.DefaultConfig(),
	}
	cfg.App.Tracing.ServiceName = AppCfgName
	return cfg
}
