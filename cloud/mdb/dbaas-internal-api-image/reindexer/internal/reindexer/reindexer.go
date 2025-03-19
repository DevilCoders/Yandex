package reindexer

import (
	"context"
	"fmt"
	"io/ioutil"
	"net/http"
	"time"

	"github.com/jmoiron/sqlx"

	"a.yandex-team.ru/cloud/mdb/internal/app"
	"a.yandex-team.ru/cloud/mdb/internal/encodingutil"
	"a.yandex-team.ru/cloud/mdb/internal/httputil"
	"a.yandex-team.ru/cloud/mdb/internal/ready"
	"a.yandex-team.ru/cloud/mdb/internal/requestid"
	"a.yandex-team.ru/cloud/mdb/internal/sentry"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/pgutil"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/nop"
	"a.yandex-team.ru/library/go/core/xerrors"
)

const name = "mdb-search-reindexer"

var queryCloudsWithAllocatedResources = sqlutil.Stmt{
	Name:  "CloudsWithAllocatedResources",
	Query: "SELECT cloud_ext_id FROM dbaas.clouds WHERE cpu_used > 0",
}

type RESTAPIConfig struct {
	httputil.ClientConfig
	Host string
}

type Config struct {
	App                app.Config            `json:"app" yaml:"app"`
	RestAPI            RESTAPIConfig         `json:"rest_api" yaml:"rest_api"`
	MaxSequentialFails int                   `json:"max_sequential_fails" yaml:"max_sequential_fails"`
	MetaDB             pgutil.Config         `json:"metadb" yaml:"metadb"`
	InitTimeout        encodingutil.Duration `json:"init_timeout" yaml:"init_timeout"`
	WorkTimeout        encodingutil.Duration `json:"work_timeout" yaml:"work_timeout"`
}

func (c *Config) AppConfig() *app.Config {
	return &c.App
}

func DefaultConfig() Config {
	a := app.DefaultConfig()
	a.ServiceAccount.ID = "yc.mdb.search-reindexer"
	a.AppName = name
	meta := pgutil.DefaultConfig()
	meta.DB = "dbaas_metadb"
	return Config{
		MaxSequentialFails: 42,
		InitTimeout:        encodingutil.FromDuration(time.Minute),
		WorkTimeout:        encodingutil.FromDuration(time.Hour * 4),
		RestAPI: RESTAPIConfig{
			ClientConfig: httputil.ClientConfig{
				Transport: httputil.DefaultTransportConfig(),
			},
		},
		MetaDB: meta,
		App:    a,
	}
}

type Reindexer struct {
	l      log.Logger
	app    *app.App
	mdb    *sqlutil.Cluster
	client *httputil.Client
	cfg    Config
}

func (r *Reindexer) cloudsWithAllocatedResources(ctx context.Context) ([]string, error) {
	var ret []string
	if _, err := sqlutil.QueryContext(
		ctx,
		r.mdb.AliveChooser(),
		queryCloudsWithAllocatedResources,
		nil, func(rows *sqlx.Rows) error {
			var c string
			if err := rows.Scan(&c); err != nil {
				return err
			}
			ret = append(ret, c)
			return nil
		}, r.l); err != nil {
		return nil, xerrors.Errorf("query %s failed: %w", queryCloudsWithAllocatedResources.Name, err)
	}
	return ret, nil
}

func (r *Reindexer) getToken(ctx context.Context) (string, error) {
	return r.app.ServiceAccountCredentials().Token(ctx)
}

func (r *Reindexer) reindex(ctx context.Context, cloudID string) error {
	requestID := requestid.New()
	url := "https://" + r.cfg.RestAPI.Host + "/mdb/1.0/search/cloud/" + cloudID + ":reindex"
	req, err := http.NewRequestWithContext(requestid.WithRequestID(ctx, requestID), http.MethodPost, url, nil)
	if err != nil {
		return xerrors.Errorf("new request: %w", err)
	}
	token, err := r.getToken(ctx)
	if err != nil {
		return xerrors.Errorf("get token: %w", err)
	}

	req.Header.Set("Content-Type", "application/json")
	req.Header.Set("X-YaCloud-SubjectToken", token)
	req.Header.Set("X-Request-Id", requestID)
	resp, err := r.client.Do(req, "reindex-cloud")
	if err != nil {
		return xerrors.Errorf("do request (X-Request-Id: %s): %w", requestID, err)
	}
	defer func() {
		closeErr := resp.Body.Close()
		if closeErr != nil {
			r.l.Warnf("response close error: %s", err)
		}
	}()
	if resp.StatusCode != http.StatusOK {
		var failReason string
		respBodyBytes, err := ioutil.ReadAll(resp.Body)
		if err != nil {
			r.l.Warnf("failed to read response body while processing response error: %s", err)
		} else {
			failReason = string(respBodyBytes)
		}
		return xerrors.Errorf("reindex request finished unsuccessfully: %s, status: %s (X-Request-Id: %s)", failReason, resp.Status, requestID)
	}
	return nil
}

func (r *Reindexer) IsReady(ctx context.Context) error {
	node := r.mdb.Standby()
	if node == nil {
		return xerrors.New("DB not ready, no standby nodes")
	}
	return nil
}

func (r *Reindexer) Run(ctx context.Context) error {
	if err := ready.WaitWithTimeout(ctx, r.cfg.InitTimeout.Duration, r, &ready.DefaultErrorTester{L: &nop.Logger{}}, time.Second); err != nil {
		return xerrors.Errorf("not ready within %s timeout: %w", r.cfg.InitTimeout.Duration, err)
	}
	clouds, err := r.cloudsWithAllocatedResources(ctx)
	if err != nil {
		return xerrors.Errorf("get clouds from metadb: %w", err)
	}
	r.l.Infof("got %d clouds from meta", len(clouds))
	var failedSequentially int
	var overallFailed int
	for _, cloudID := range clouds {
		err = r.reindex(ctx, cloudID)
		if err != nil {
			failedSequentially++
			overallFailed++
			if failedSequentially >= r.cfg.MaxSequentialFails {
				return xerrors.Errorf("%q reindex failed. I gave up, cause failed %d times sequentially: %w", cloudID, failedSequentially, err)
			}
			r.l.Warnf("cloud %q reindex failed: %s, but we don't gave up, cause have only %d sequential fails", cloudID, err, failedSequentially)
			continue
		}
		r.l.Infof("successfully reindex cloud %q", cloudID)
		failedSequentially = 0
	}
	r.l.Infof("finish reindex. On %d clouds reindex succeeded. On %d clouds reindex failed", len(clouds)-overallFailed, overallFailed)
	return nil
}

func New(cfg Config, a *app.App) (*Reindexer, error) {
	client, err := httputil.NewClient(cfg.RestAPI.ClientConfig, a.L())
	if err != nil {
		a.L().Errorf("new REST client: %v", err)
	}
	mdb, err := pgutil.NewCluster(cfg.MetaDB)
	if err != nil {
		return nil, xerrors.Errorf("new meta: %w", err)
	}
	return &Reindexer{
		app:    a,
		l:      a.L(),
		mdb:    mdb,
		client: client,
		cfg:    cfg,
	}, nil
}

func Run() int {
	cfg := DefaultConfig()
	appOpts := app.DefaultToolOptions(&cfg, name+".yaml")
	appOpts = append(appOpts, app.WithSentry())
	a, err := app.New(appOpts...)
	if err != nil {
		fmt.Printf("internal app construnction: %s\n", err)
		return 1
	}
	r, err := New(cfg, a)
	if err != nil {
		a.L().Errorf("new reindexer: %s", err)
		return 2
	}

	ctx, cancel := context.WithTimeout(context.Background(), cfg.WorkTimeout.Duration)
	defer cancel()

	err = r.Run(ctx)
	if err != nil {
		if !xerrors.Is(err, context.DeadlineExceeded) {
			sentry.GlobalClient().CaptureErrorAndWait(
				ctx, err, nil,
			)
		}
		a.L().Errorf("run failed: %s", err)
		return 3
	}
	return 0
}
