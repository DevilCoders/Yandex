package instances

import (
	"fmt"
	"os"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/instances/duty"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/instances/duty/config"
	"a.yandex-team.ru/cloud/mdb/internal/app"
)

type Duty struct {
	autoDuty *duty.AutoDuty
	app      *app.App
}

func NewDuty() *Duty {
	cfg := config.DefaultConfig()
	toolOptions := app.DefaultServiceOptions(&cfg, fmt.Sprintf("%s.yaml", config.AppCfgName))
	baseApp, err := app.New(toolOptions...)
	if err != nil {
		fmt.Printf("cannot make base app, %s\n", err)
		os.Exit(1)
	}

	logger := baseApp.L()

	autoDuty := duty.NewAutoDutyFromConfig(baseApp.ShutdownContext(), logger, cfg)

	return &Duty{
		autoDuty: autoDuty,
		app:      baseApp,
	}
}

func (d *Duty) Run() {
	go d.app.WaitForStop()
	d.autoDuty.Run(d.app.ShutdownContext())
}
