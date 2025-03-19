package internal

import (
	"context"
	"encoding/json"
	"fmt"
	"sync"
	"time"

	"github.com/gofrs/flock"
	"github.com/gofrs/uuid"

	"a.yandex-team.ru/cloud/mdb/internal/app"
	"a.yandex-team.ru/cloud/mdb/internal/logbroker/writer"
	"a.yandex-team.ru/cloud/mdb/internal/logbroker/writer/logbroker"
	"a.yandex-team.ru/cloud/mdb/internal/timeutil"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type App struct {
	*app.App
	Cfg      Config
	LbWriter writer.Writer
	logMutex sync.Mutex
}

func NewApp() (*App, error) {
	cfg := DefaultConfig()
	opts := app.DefaultToolOptions(&cfg, ConfigFileName)
	opts = append(opts, app.WithLoggerConstructor(ConfigureLogger))
	baseApp, err := app.New(opts...)
	if err != nil {
		return nil, err
	}
	app := &App{App: baseApp, Cfg: cfg}
	if len(app.Cfg.TelegrafChecks) == 0 {
		return nil, xerrors.Errorf("telegraf_checks is empty")
	}
	return app, nil
}

func (app *App) InitLogbroker(ctx context.Context) bool {
	var err error
	for {
		app.LbWriter, err = logbroker.New(
			app.ShutdownContext(),
			app.Cfg.LogBroker,
			logbroker.Logger(app.L()),
			logbroker.WithRetryOnFailure(true),
		)
		if err == nil {
			return true
		}
		app.L().Errorf("failed to init logbroker: %v", err)
		if !sleep(ctx, app.Cfg.RetryTimeout.Duration) {
			return false
		}
	}
}

type Metric struct {
	Version    string `json:"version"`
	Schema     string `json:"schema"`
	ID         string `json:"id"`
	CloudID    string `json:"cloud_id"`
	FolderID   string `json:"folder_id"`
	ResourceID string `json:"resource_id"`
	SourceID   string `json:"source_id"`
	SourceWT   int64  `json:"source_wt"`
	Usage      struct {
		Start    int64  `json:"start"`
		Finish   int64  `json:"finish"`
		Quantity int64  `json:"quantity"`
		Type     string `json:"type"`
		Unit     string `json:"unit"`
	} `json:"usage"`
	Tags map[string]interface{} `json:"tags"`
}

func (m Metric) String() string {
	d, err := json.Marshal(m)
	if err != nil {
		panic(fmt.Sprintf("failed to serialize metric: %v", err))
	}
	return string(d)
}

func (app *App) makeMetric(start, finish time.Time) *Metric {
	m := new(Metric)
	m.Version = MetricVersion
	m.Schema = app.Cfg.Billing.Schema
	m.ID = uuid.Must(uuid.NewV4()).String()
	m.CloudID = app.Cfg.Billing.CloudID
	m.FolderID = app.Cfg.Billing.FolderID
	m.SourceID = app.Cfg.Billing.FQDN
	m.ResourceID = app.Cfg.Billing.FQDN
	m.SourceWT = finish.Unix()
	m.Tags = app.Cfg.Billing.Tags
	m.Usage.Type = "delta"
	m.Usage.Unit = "seconds"
	m.Usage.Start = start.Unix()
	m.Usage.Finish = finish.Unix()
	m.Usage.Quantity = m.Usage.Finish - m.Usage.Start
	return m
}

func (app *App) makeLicenseMetric(start, finish time.Time) *Metric {
	m := new(Metric)
	m.Version = MetricVersion
	m.Schema = app.Cfg.Billing.LicenseSchema
	m.ID = uuid.Must(uuid.NewV4()).String()
	m.CloudID = app.Cfg.Billing.CloudID
	m.FolderID = app.Cfg.Billing.FolderID
	m.SourceID = app.Cfg.Billing.FQDN
	m.ResourceID = app.Cfg.Billing.ClusterID
	// hardware and license metrics should have different SourceWT timestamps
	// as we use SourceWT for message deduplication in LogBroker
	m.SourceWT = finish.Unix() + 1
	m.Tags = app.Cfg.Billing.LicenseTags
	m.Usage.Type = "delta"
	m.Usage.Unit = "seconds"
	m.Usage.Start = start.Unix()
	m.Usage.Finish = finish.Unix()
	m.Usage.Quantity = m.Usage.Finish - m.Usage.Start
	return m
}

func (app *App) Report(ctx context.Context) {
	prevState := app.GetState()
	app.L().Debugf("prev state %v", prevState)
	now := time.Now()
	alive, needSendLicense, err := app.GetStatus(ctx, now)
	if err != nil {
		app.L().Errorf("failed to check alive: %v", err)
		return
	}
	currState := &State{
		Alive:  alive,
		LastTS: now,
	}
	app.L().Debugf("curr state %v", currState)
	if prevState.Alive && currState.Alive {
		ts := timeutil.SplitHours(prevState.LastTS, currState.LastTS)
		for i := 0; i < len(ts)-1; i++ {
			hwMetric := app.makeMetric(ts[i], ts[i+1])
			app.L().Infof("new metric %s: %s", hwMetric.ID, hwMetric)
			err := app.LogMetric(hwMetric)
			if err != nil {
				app.L().Errorf("failed to log metric: %s", hwMetric.ID)
			}
			if needSendLicense {
				licenseMetric := app.makeLicenseMetric(ts[i], ts[i+1])
				app.L().Infof("new license metric %s: %s", licenseMetric.ID, licenseMetric)
				err := app.LogMetric(licenseMetric)
				if err != nil {
					app.L().Errorf("failed to log license metric: %s", licenseMetric.ID)
				}
			}
		}
	}
	err = app.SetState(currState)
	if err != nil {
		app.L().Errorf("failed to save state: %v", err)
	}
}

func (app *App) Reporter(ctx context.Context) {
	timeout := time.Duration(0.9 * float64(app.Cfg.ReportInterval.Duration))
	ctx2, cancel := context.WithTimeout(ctx, timeout)
	defer cancel()
	app.Report(ctx2)
	t := time.NewTicker(app.Cfg.ReportInterval.Duration)
	defer t.Stop()
	for {
		select {
		case <-t.C:
			ctx2, cancel := context.WithTimeout(ctx, timeout)
			defer cancel()
			app.Report(ctx2)
		case <-ctx.Done():
			return
		}
	}
}

func (app *App) AcquireLock() *flock.Flock {
	lock := flock.New(app.Cfg.LockFile)
	locked, err := lock.TryLock()
	if err != nil {
		app.L().Errorf("failed to acquire lock: %v", err)
		return nil
	}
	if !locked {
		app.L().Errorf("failed to acquire lock: not locked")
		return nil
	}
	return lock
}

func (app *App) Run(ctx context.Context) {
	lock := app.AcquireLock()
	if lock == nil {
		return
	}
	defer lock.Unlock()
	if !app.InitLogbroker(ctx) {
		return
	}
	go app.Reporter(ctx)
	go app.Sender(ctx)
	<-ctx.Done()
}
