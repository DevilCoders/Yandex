package tracker

import (
	"a.yandex-team.ru/cloud/mdb/internal/cli"
	"a.yandex-team.ru/cloud/mdb/internal/secret"
	"a.yandex-team.ru/cloud/mdb/internal/tracker"
	"a.yandex-team.ru/cloud/mdb/internal/tracker/http"
	"a.yandex-team.ru/cloud/mdb/mdb-cli/pkg/common/config"
	"a.yandex-team.ru/cloud/mdb/mdb-cli/pkg/common/token"
)

const stTokenURL = `https://oauth.yandex-team.ru/authorize?response_type=token&client_id=5f671d781aca402ab7460fde4050267b`

func getTrackerToken(env *cli.Env) (string, error) {
	cfg := config.FromEnv(env)
	if cfg.TrackerToken == "" {
		t, err := token.Get(env, token.Description{
			URL:  stTokenURL,
			Name: "Tracker",
		})
		if err != nil {
			return "", err
		}
		cfg.TrackerToken = t
		if err = config.WriteConfig(cfg, env.GetConfigPath()); err != nil {
			return "", err
		}
		env.RootCmd.Cmd.Printf("Write updated config to %s\n", env.GetConfigPath())
	}
	return cfg.TrackerToken, nil
}

func NewAPI(env *cli.Env) tracker.API {
	trackerConfig := http.DefaultConfig()
	cfg := config.FromEnv(env)
	trackerToken, err := getTrackerToken(env)
	if err != nil {
		env.L().Fatalf("Get a Tracker token: %s", err)
	}
	trackerConfig.Token = secret.NewString(trackerToken)
	trackerConfig.Client.Transport.Logging.LogRequestBody = cfg.LogHTTPBody
	trackerConfig.Client.Transport.Logging.LogResponseBody = cfg.LogHTTPBody
	st, err := http.New(trackerConfig, env.L())
	if err != nil {
		env.L().Fatalf("Initialize tracker client: %s", err)
	}
	return st
}
