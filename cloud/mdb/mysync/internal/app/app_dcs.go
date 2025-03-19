package app

import (
	"fmt"

	"a.yandex-team.ru/cloud/mdb/internal/dcs"
)

func (app *App) GetActiveNodes() ([]string, error) {
	var activeNodes []string
	err := app.dcs.Get(pathActiveNodes, &activeNodes)
	if err != nil {
		if err == dcs.ErrNotFound {
			return nil, nil
		}
		return nil, fmt.Errorf("failed to get active nodes from zk %v", err)
	}
	return activeNodes, nil
}

func (app *App) GetClusterCascadeFqdnsFromDcs() ([]string, error) {
	fqdns, err := app.dcs.GetChildren(dcs.PathCascadeNodesPrefix)
	if err == dcs.ErrNotFound {
		return make([]string, 0), nil
	}
	if err != nil {
		return nil, err
	}

	return fqdns, nil
}

func (app *App) GetMaintenance() (*Maintenance, error) {
	maintenance := new(Maintenance)
	err := app.dcs.Get(pathMaintenance, maintenance)
	if err != nil {
		return nil, err
	}
	return maintenance, err
}

func (app *App) GetHostsOnRecovery() ([]string, error) {
	hosts, err := app.dcs.GetChildren(pathRecovery)
	if err == dcs.ErrNotFound {
		return nil, nil
	}
	return hosts, err
}

func (app *App) ClearRecovery(host string) error {
	return app.dcs.Delete(dcs.JoinPath(pathRecovery, host))
}
