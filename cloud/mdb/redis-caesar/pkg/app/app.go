package app

import (
	"context"
	"fmt"

	"a.yandex-team.ru/cloud/mdb/redis-caesar/internal/app"
	"a.yandex-team.ru/cloud/mdb/redis-caesar/internal/app/states"
	"a.yandex-team.ru/cloud/mdb/redis-caesar/internal/telemetry"
	"a.yandex-team.ru/cloud/mdb/redis-caesar/pkg/config"
	"a.yandex-team.ru/cloud/mdb/redis-caesar/pkg/config/flags"
)

// Start is a main entrypoint to application.
func Start(startFlags *flags.Root) int {
	ctx := context.Background()

	conf, err := config.Load(ctx, *startFlags)
	if err != nil {
		fmt.Printf("Unable to load config: %s\n", err.Error())

		return 1
	}

	stopTelemetry, err := telemetry.StartTelemetryServer(ctx, conf.Telemetry)
	if err != nil {
		fmt.Printf("Unable to start telemetry server: %s\n", err.Error())

		return 1
	}

	defer func() {
		if err = stopTelemetry(ctx); err != nil {
			fmt.Printf("Unable to stop telemetry server: %s\n", err.Error())
		}
	}()

	logger, err := telemetry.ConfigureLogging(ctx, conf.Telemetry)
	if err != nil {
		fmt.Printf("Unable to configure logging: %s\n", err.Error())

		return 1
	}
	logger.Infof("Config: %+v", conf)

	stateFirstRun := states.NewStateFirstRun(conf, logger)
	stateMachine := app.NewStateMachine(logger, conf)
	stateMachine.Start(ctx, stateFirstRun)

	return 0
}
