package app

import (
	"errors"
	"fmt"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/app"
	cfg "a.yandex-team.ru/cloud/mdb/windows/mssync/internal/config"
	ss "a.yandex-team.ru/cloud/mdb/windows/mssync/internal/sqlserver"
	"github.com/gofrs/flock"
)

type App struct {
	*app.App
	config   cfg.Config
	filelock *flock.Flock
}

func NewApp() (*App, error) {
	config := cfg.DefaultConfig()
	opts := app.DefaultToolOptions(&config, cfg.ConfigFileName)
	opts = append(opts, app.WithLoggerConstructor(cfg.ConfigureLogger))
	baseApp, err := app.New(opts...)
	if err != nil {
		return nil, err
	}
	app := &App{App: baseApp, config: config}
	return app, nil
}

func (app *App) lockFile() error {
	app.filelock = flock.New(app.config.LockFile)
	if locked, err := app.filelock.TryLock(); !locked {
		msg := "Possibly another instance is running."
		if err != nil {
			msg = err.Error()
		}
		return fmt.Errorf("failed to acquire lock on %s: %w", app.config.LockFile, msg)
	}
	return nil
}

func (app *App) unlockFile() {
	_ = app.filelock.Unlock()
}

func (app *App) Run() int {
	err := app.lockFile()
	if err != nil {
		app.L().Errorf(err.Error())
		return 1
	}
	defer app.unlockFile()

	app.L().Infof("Starting mssync...")
	var errcount int
	db, err := ss.GetSQLServerConnection(app.config.ConnectionString)
	if err != nil {
		errcount = errcount + 1
		app.L().Errorf("Error ocurred during connection: %v", err)
	}
	for {
		var alive bool
		if errcount > app.config.MaxErrCount {
			app.L().Errorf("tries depleted, cooling down")
			time.Sleep(time.Duration(app.config.CooldownInterval) * time.Second)
			errcount = 0
		}
		sqlserver, err := ss.GetSQLServer(db)
		if err != nil {
			app.L().Errorf("could not initialize sqlserver object: %v", err)
			errcount++
		}
		alive, health, err := sqlserver.IsAlive(app.config.FailureConditionLevel, db)
		if err != nil {
			errcount++
			app.L().Errorf("IsAlive check failed with error: %v", err)
		}
		if alive {
			if split, prim, sec := sqlserver.IsHADRSplit(); split {
				app.L().Warnf("HADR is Split")
				time.Sleep(time.Duration(app.config.HealthCheckInterval) * time.Second)
				var winner bool
				demotionTime, promotionTime, err := sqlserver.GetRoleChangeTimes(db)
				if err != nil {
					app.L().Errorf("Failed to get LastAGDemotionTime: %v", err)
					errcount++
					continue
				}
				zeroday := (promotionTime == demotionTime)
				draw := (prim == sec)
				if zeroday {
					if draw {
						res, err := sqlserver.IsPreferredReplica(db)
						if err != nil {
							app.L().Errorf("Failed to check if current replica is prefered: %v", err)
							winner = false
						}
						winner = res
					} else {
						winner = (prim > sec)
					}
				} else {
					winner = demotionTime.Before(promotionTime)
					app.L().Infof("LastAGDemotionTime is %v; LastAGPromotionTime is %v", demotionTime, promotionTime)
				}
				if winner {
					err = runParallel(func(i int) error {
						ag := sqlserver.AGs[i]
						app.L().Infof("Starting failover precheck for AG %v", ag.Name)
						err := ag.LoadAGRole(db)
						if err != nil {
							app.L().Errorf("Failed to get the role of local replica in AG %v with error: %v", ag.Name, err)
						}
						app.L().Infof("AG %v local replica has role of %v", ag.Name, ag.Role)
						if ag.Role == ss.AGRoleSecondary {
							err := ag.Promote(db)
							if err != nil {
								app.L().Errorf("Promotion of AG %v failed with error: %v", ag.Name, err)
								return err
							}
						}
						return nil
					}, len(sqlserver.AGs), 999)
					if err != nil {
						app.L().Errorf("Promotion failed with error: %v", err)
					}
					deadline := time.Now().Add(time.Duration(app.config.PromotionTimeout) * time.Second)
					app.L().Infof("Waiting for all the AGs to promote until %v", deadline)
					split, _, _ := sqlserver.IsHADRSplit()
					for split && time.Now().Before(deadline) {
						app.L().Infof("Waiting for all the AGs to promote...")
						time.Sleep(time.Duration(app.config.HealthCheckInterval) * time.Second)
						err := sqlserver.LoadAGs(db)
						if err != nil {
							app.L().Errorf("Failed to check AGs status: %v", err)
							errcount++
							time.Sleep(time.Duration(app.config.HealthCheckInterval) * time.Second)
							continue
						}
						split, _, _ = sqlserver.IsHADRSplit()
						if !split {
							app.L().Infof("All AGs promoted and HADR is no longer split")
							time.Sleep(time.Duration(app.config.CooldownInterval) * time.Second)
						} else {
							if time.Now().After(deadline) {
								app.L().Errorf("Failover deadline reached. Halting for %v seconds", time.Duration(app.config.CooldownInterval)*time.Second)
								time.Sleep(time.Duration(app.config.CooldownInterval) * time.Second)
							}
						}
					}

				} else {
					app.L().Infof("Current replica is not the one to be promoted.")
					app.L().Infof("Last AG demotion time: %v", demotionTime)
					app.L().Infof("Last AG promotion time: %v", promotionTime)
					app.L().Infof("Is Zeroday: %v", zeroday)
					app.L().Infof("Is Draw: %v", draw)
					time.Sleep(time.Duration(app.config.CooldownInterval) * time.Second)
				}
			}
		}
		if !alive {
			errcount++
			app.L().Errorf("SQL Server failed IsAlive check with FailureConditionLevel = %v", app.config.FailureConditionLevel)
			app.L().Errorf("Health data: %v", formatHealthData(health))
		}
		time.Sleep(time.Duration(app.config.HealthCheckInterval) * time.Second)
	}
}

func runParallel(f func(int) error, cnt int, concurrency int) error {
	if concurrency <= 0 {
		concurrency = cnt
	}
	sem := make(chan struct{}, concurrency)
	errs := make(chan error, cnt)
	for i := 0; i < cnt; i++ {
		go func(i int) {
			sem <- struct{}{}
			defer func() { <-sem }()
			errs <- f(i)
		}(i)
	}
	var errStr string
	for i := 0; i < cnt; i++ {
		err := <-errs
		if err != nil {
			errStr += err.Error() + "\n"
		}
	}
	if errStr != "" {
		return errors.New(errStr)
	}
	return nil
}

func formatHealthData(health ss.InstanceHealth) string {
	result := fmt.Sprintf(`Health data:
	SystemState: %v
	ResourceState: %v
	QueryProcessingState: %v

	raw data:
	%+v`, ss.HealthStatetoString[health.SystemState],
		ss.HealthStatetoString[health.ResourceState],
		ss.HealthStatetoString[health.QueryProcessingState],
		health,
	)

	return result
}
