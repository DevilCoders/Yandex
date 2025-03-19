package app

import (
	"context"
	"fmt"
	"time"

	"a.yandex-team.ru/cloud/mdb/deploy/api/pkg/deployapi"
	deployapirest "a.yandex-team.ru/cloud/mdb/deploy/api/pkg/deployapi/restapi"
	"a.yandex-team.ru/cloud/mdb/internal/app"
	"a.yandex-team.ru/cloud/mdb/internal/compute/iam"
	iamgrpc "a.yandex-team.ru/cloud/mdb/internal/compute/iam/grpc"
	"a.yandex-team.ru/cloud/mdb/internal/encodingutil"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil"
	"a.yandex-team.ru/cloud/mdb/internal/juggler"
	jugglerclient "a.yandex-team.ru/cloud/mdb/internal/juggler/http"
	"a.yandex-team.ru/cloud/mdb/internal/ready"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/pgutil"
	"a.yandex-team.ru/cloud/mdb/katan/internal/katandb"
	kdbpg "a.yandex-team.ru/cloud/mdb/katan/internal/katandb/pg"
	"a.yandex-team.ru/cloud/mdb/katan/katan/internal/katan"
	healthapi "a.yandex-team.ru/cloud/mdb/mdb-health/pkg/client"
	healthswagger "a.yandex-team.ru/cloud/mdb/mdb-health/pkg/client/swagger"
	"a.yandex-team.ru/cloud/mdb/mlock/pkg/mlockclient"
	mlockclientgrpc "a.yandex-team.ru/cloud/mdb/mlock/pkg/mlockclient/grpc"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/nop"
)

const (
	configName = "katan.yaml"
	appName    = "Katan"
)

type Config struct {
	App          app.Config             `json:"app" yaml:"app"`
	InitTimeout  encodingutil.Duration  `json:"init_timeout" yaml:"init_timeout"`
	Katandb      pgutil.Config          `json:"katandb" yaml:"katandb"`
	Katan        katan.Config           `json:"katan" yaml:"katan"`
	Juggler      jugglerclient.Config   `json:"juggler" yaml:"juggler"`
	Deploy       deployapirest.Config   `json:"deploy" yaml:"deploy"`
	Health       healthswagger.Config   `json:"health" yaml:"health"`
	MLock        mlockclientgrpc.Config `json:"mlock" yaml:"mlock"`
	TokenService grpcutil.ClientConfig  `json:"token_service" yaml:"token_service"`
}

func DefaultConfig() Config {
	return Config{
		App:         app.DefaultConfig(),
		InitTimeout: encodingutil.FromDuration(time.Minute),
		Katandb: pgutil.Config{
			User: "katan",
			DB:   kdbpg.DBName,
		},
		Katan:        katan.DefaultConfig(),
		Juggler:      jugglerclient.DefaultConfig(),
		Deploy:       deployapirest.DefaultConfig(),
		Health:       healthswagger.DefaultConfig(),
		MLock:        mlockclientgrpc.DefaultConfig(),
		TokenService: grpcutil.DefaultClientConfig(),
	}
}

func (c *Config) AppConfig() *app.Config {
	return &c.App
}

type App struct {
	*app.App
	kat *katan.Katan
	L   log.Logger
}

func (app *App) waitForReady(ctx context.Context, initTimeout time.Duration) error {
	awaitCtx, cancel := context.WithTimeout(ctx, initTimeout)
	defer cancel()
	return ready.Wait(awaitCtx, app.kat, &ready.DefaultErrorTester{L: &nop.Logger{}}, time.Second)
}

func (app *App) Run(ctx context.Context) {
	app.kat.Run(ctx)
}

func NewAppCustom(
	ctx context.Context,
	config Config,
	kdb katandb.KatanDB,
	client deployapi.Client,
	api juggler.API,
	health healthapi.MDBHealthClient,
	mlock mlockclient.Locker,
	baseApp *app.App,
) (*App, error) {
	katApp := &App{
		kat: katan.New(config.Katan, kdb, client, api, health, mlock, baseApp.L()),
		App: baseApp,
	}
	if err := katApp.waitForReady(ctx, config.InitTimeout.Duration); err != nil {
		return nil, err
	}
	return katApp, nil
}

func newAppFromConfig() (*App, error) {
	cfg := DefaultConfig()
	baseApp, err := app.New(app.DefaultServiceOptions(&cfg, configName)...)
	if err != nil {
		return nil, err
	}

	baseApp.L().Debugf("config is %+v", cfg)
	L := baseApp.L()

	ctx := context.Background()

	tsClient, err := iamgrpc.NewTokenServiceClient(
		ctx,
		cfg.App.Environment.Services.Iam.V1.TokenService.Endpoint,
		appName,
		cfg.TokenService,
		&grpcutil.PerRPCCredentialsStatic{},
		L,
	)
	if err != nil {
		return nil, fmt.Errorf("token service client: %w", err)
	}
	creds := tsClient.ServiceAccountCredentials(iam.ServiceAccount{
		ID:    cfg.App.ServiceAccount.ID,
		KeyID: cfg.App.ServiceAccount.KeyID.Unmask(),
		Token: []byte(cfg.App.ServiceAccount.PrivateKey.Unmask()),
	})

	deployClient, err := deployapirest.New(cfg.Deploy.URI, "", creds, cfg.Deploy.Transport.TLS, cfg.Deploy.Transport.Logging, L)
	if err != nil {
		return nil, fmt.Errorf("deploy client: %w", err)
	}

	jugglerClient, err := jugglerclient.NewClient(cfg.Juggler, L)
	if err != nil {
		return nil, fmt.Errorf("juggler client: %w", err)
	}

	// we don't need key, cause only read
	healthClient, err := healthswagger.NewClientTLSFromConfig(cfg.Health, L)
	if err != nil {
		return nil, fmt.Errorf("health client: %w", err)
	}

	mlockClient, err := mlockclientgrpc.NewFromConfig(ctx, cfg.MLock, appName, creds, L)
	if err != nil {
		return nil, fmt.Errorf("mlock client: %w", err)
	}

	kdb, err := kdbpg.New(cfg.Katandb, L)
	if err != nil {
		return nil, err
	}

	katApp, err := NewAppCustom(ctx, cfg, kdb, deployClient, jugglerClient, healthClient, mlockClient, baseApp)
	if err != nil {
		_ = kdb.Close()
		return nil, err
	}

	return katApp, nil
}

func Run() int {
	katApp, err := newAppFromConfig()
	if err != nil {
		fmt.Printf("katan initialization failed with: %s\n", err)
		return 1
	}
	katApp.Run(context.Background())
	return 0
}
