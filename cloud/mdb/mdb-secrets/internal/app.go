package internal

import (
	"context"
	"net/http"
	"time"

	"github.com/go-openapi/runtime/middleware"
	"github.com/golang/mock/gomock"

	"a.yandex-team.ru/cloud/mdb/internal/app"
	"a.yandex-team.ru/cloud/mdb/internal/app/swagger"
	"a.yandex-team.ru/cloud/mdb/internal/auth/blackbox/restapi"
	"a.yandex-team.ru/cloud/mdb/internal/auth/httpauth"
	"a.yandex-team.ru/cloud/mdb/internal/auth/httpauth/blackboxauth"
	"a.yandex-team.ru/cloud/mdb/internal/auth/httpauth/combinedauth"
	"a.yandex-team.ru/cloud/mdb/internal/auth/httpauth/iamauth"
	"a.yandex-team.ru/cloud/mdb/internal/auth/httpauth/mocks"
	"a.yandex-team.ru/cloud/mdb/internal/auth/tvm/tvmtool"
	"a.yandex-team.ru/cloud/mdb/internal/crt"
	crtCloud "a.yandex-team.ru/cloud/mdb/internal/crt/cloudcrt"
	crtLetsEncrypt "a.yandex-team.ru/cloud/mdb/internal/crt/letsencrypt"
	"a.yandex-team.ru/cloud/mdb/internal/crt/letsencrypt/challenge/route53"
	crtRest "a.yandex-team.ru/cloud/mdb/internal/crt/yacrt"
	"a.yandex-team.ru/cloud/mdb/internal/flags"
	"a.yandex-team.ru/cloud/mdb/internal/httputil"
	"a.yandex-team.ru/cloud/mdb/internal/nacl"
	"a.yandex-team.ru/cloud/mdb/internal/ready"
	"a.yandex-team.ru/cloud/mdb/internal/secret"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/pgutil"
	"a.yandex-team.ru/cloud/mdb/mdb-secrets/internal/api/cert"
	"a.yandex-team.ru/cloud/mdb/mdb-secrets/internal/api/gpg"
	"a.yandex-team.ru/cloud/mdb/mdb-secrets/internal/secretsdb"
	"a.yandex-team.ru/cloud/mdb/mdb-secrets/internal/secretsdb/pg"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type AvailableCertProvider string

const (
	cloudCrt       AvailableCertProvider = "cloud"
	yandexCrt      AvailableCertProvider = "yandex"
	letsEncryptCrt AvailableCertProvider = "letsencrypt"
)

// App main application object - handles setup and teardown
type App struct {
	*swagger.App

	cfg Config

	GpgAPI        *gpg.Service
	CertAPI       *cert.Service
	ReadyProvider ready.Checker
}

var _ app.AppConfig = &Config{}

func (c *Config) AppConfig() *app.Config {
	return &c.App
}

// Config for mdb-secrets
type Config struct {
	App     app.Config            `json:"app" yaml:"app"`
	Swagger swagger.SwaggerConfig `json:"swagger" yaml:"swagger"`

	SecretsDB pgutil.Config `json:"secretsdb" yaml:"secretsdb"`
	Auth      AuthConfig    `json:"auth" yaml:"auth"`

	SaltPublicKey string        `json:"saltkey" yaml:"saltkey"`
	PrivateKey    secret.String `json:"privatekey" yaml:"privatekey"`

	CrtToUse       AvailableCertProvider `json:"certificate_to_use" yaml:"certificate_to_use"`
	CloudCrt       crtCloud.Config       `json:"cloud_crt" yaml:"cloud_crt"`
	YandexCrt      crtRest.Config        `json:"yandex_crt" yaml:"yandex_crt"`
	LetsEncryptCrt LetsEncryptCrtConfig  `json:"letsencrypt_crt" yaml:"letsencrypt_crt"`
}

type AuthConfig struct {
	BlackBoxEnabled bool                `json:"blackbox_enabled" yaml:"blackbox_enabled"`
	BlackBox        blackboxauth.Config `json:"blackbox" yaml:"blackbox"`
	IAMEnabled      bool                `json:"iam_enabled" yaml:"iam_enabled"`
	IAM             iamauth.Config      `json:"iam" yaml:"iam"`

	// TODO: remove after https://st.yandex-team.ru/ORION-104
	NoopAuth bool `json:"noop_auth" yaml:"noop_auth"`
}

type LetsEncryptCrtConfig struct {
	LetsEncrypt crtLetsEncrypt.Config `json:"letsencrypt" yaml:"letsencrypt"`
	Route53     route53.Config        `json:"route53" yaml:"route53"`
}

func defaultConfig() Config {
	config := Config{
		Swagger: swagger.DefaultSwaggerConfig(),
		App:     app.DefaultConfig(),
		SecretsDB: pgutil.Config{
			Addrs:    []string{"localhost"},
			DB:       "secretsdb",
			User:     "secrets_api",
			Password: secret.NewString("secrets_api"),
		},
		Auth: AuthConfig{
			BlackBoxEnabled: true,
			BlackBox: blackboxauth.Config{
				Tvm: tvmtool.Config{
					Alias: "mdb-secrets",
					Token: "",
					URI:   "http://localhost:50001",
				},
				BlackboxURI:   restapi.IntranetURL,
				BlackboxAlias: "blackbox",
			},
			IAMEnabled: true,
			IAM:        iamauth.DefaultConfig(),
			NoopAuth:   false,
		},
		CrtToUse: cloudCrt,
		CloudCrt: crtCloud.DefaultConfig(),
		LetsEncryptCrt: LetsEncryptCrtConfig{
			LetsEncrypt: crtLetsEncrypt.DefaultConfig(),
		},
	}
	config.App.Tracing.ServiceName = "mdb-secrets"
	return config
}

const (
	ConfigName = "mdb-secrets.yaml"
)

func init() {
	flags.RegisterConfigPathFlagGlobal()
}

// NewApp constructs application object
func NewApp(mdwCtx *middleware.Context) *App {
	cfg := defaultConfig()

	baseApp, err := swagger.New(mdwCtx, app.DefaultServiceOptions(&cfg, ConfigName)...)
	if err != nil {
		panic(err)
	}

	l := baseApp.L()

	authProvider, err := authProvider(baseApp.ShutdownContext(), cfg, l)
	if err != nil {
		l.Fatalf("init authentication: %s", err.Error())
	}

	if !cfg.SecretsDB.Password.FromEnv("SECRETSDB_PASSWORD") {
		l.Info("SECRETSDB_PASSWORD is empty")
	}

	if !cfg.LetsEncryptCrt.Route53.AWSSecretKey.FromEnv("ROUTE53_SECRET_KEY") {
		l.Info("ROUTE53_SECRET_KEY is empty")
	}

	if !cfg.PrivateKey.FromEnv("SECRETS_PRIVATE_KEY") {
		l.Info("SECRETS_PRIVATE_KEY is empty")
	}

	if !cfg.CloudCrt.OAuth.FromEnv("CLOUD_CRT_OAUTH") {
		l.Info("CLOUD_CRT_OAUTH is empty")
	}

	if !cfg.YandexCrt.OAuth.FromEnv("YANDEX_CRT_OAUTH") {
		l.Info("YANDEX_CRT_OAUTH is empty")
	}

	db, err := secretsDB(cfg, l)
	if err != nil {
		l.Fatalf("init secretsDB: %s", err.Error())
	}

	pReg := &ready.Aggregator{}
	pReg.Register(ready.CheckerFunc(authProvider.Ping))
	pReg.Register(ready.CheckerFunc(db.IsReady))

	crtClient, err := crtClient(cfg, db, l)
	if err != nil {
		l.Fatalf("init crt client: %s", err.Error())
	}

	gpgAPI := gpg.New(db, authProvider, l)
	certAPI := cert.New(db, authProvider, crtClient, l, time.Now)

	return &App{
		App:           baseApp,
		cfg:           cfg,
		GpgAPI:        gpgAPI,
		CertAPI:       certAPI,
		ReadyProvider: pReg,
	}
}

func authProvider(ctx context.Context, cfg Config, l log.Logger) (httpauth.Authenticator, error) {
	var authProviders []httpauth.Authenticator

	if cfg.Auth.IAMEnabled {
		iamAuth, err := iamauth.New(ctx, cfg.Auth.IAM, l)
		if err != nil {
			return nil, err
		}
		authProviders = append(authProviders, iamAuth)
	}

	if cfg.Auth.BlackBoxEnabled {
		bbAuth, err := blackboxauth.NewFromConfig(cfg.Auth.BlackBox, l)
		if err != nil {
			return nil, err
		}
		authProviders = append(authProviders, bbAuth)
	}

	if cfg.Auth.NoopAuth {
		ctrl := gomock.NewController(l)
		auth := mocks.NewMockAuthenticator(ctrl)
		auth.EXPECT().Ping(gomock.Any()).Return(nil).AnyTimes()
		auth.EXPECT().Auth(gomock.Any(), gomock.Any()).Return(nil).AnyTimes()
		authProviders = append(authProviders, auth)
	}

	return combinedauth.New(l, authProviders...)
}

func secretsDB(cfg Config, l log.Logger) (secretsdb.Service, error) {
	saltPubKey, err := nacl.ParseKey(cfg.SaltPublicKey)
	if err != nil {
		return nil, xerrors.Errorf("invalid salt public key: %w", err)
	}
	privateKey, err := nacl.ParseKey(cfg.PrivateKey.Unmask())
	if err != nil {
		return nil, xerrors.Errorf("invalid mdb-secrets private key: %w", err)
	}
	return pg.New(cfg.SecretsDB, l, saltPubKey, privateKey)
}

func crtClient(cfg Config, db secretsdb.Service, l log.Logger) (crt.Client, error) {
	switch cfg.CrtToUse {
	case yandexCrt:
		return crtRest.New(l, cfg.YandexCrt)
	case cloudCrt:
		return crtCloud.NewFromConfig(cfg.CloudCrt, l)
	case letsEncryptCrt:
		route53Challenge := route53.New(cfg.LetsEncryptCrt.Route53)
		return crtLetsEncrypt.New(cfg.LetsEncryptCrt.LetsEncrypt, route53Challenge, db, l)
	default:
		return nil, xerrors.Errorf("unknown crt %q", cfg.CrtToUse)
	}
}

// SetupGlobalMiddleware setups global middleware
func (app *App) SetupGlobalMiddleware(next http.Handler) http.Handler {
	return httputil.ChainStandardSwaggerMiddleware(
		next,
		app.cfg.App.Tracing.ServiceName,
		app.MiddlewareContext(),
		app.cfg.Swagger.Logging,
		app.L(),
	)
}
