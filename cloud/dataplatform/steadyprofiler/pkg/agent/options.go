package agent

import (
	"crypto/tls"
	"os"
	"time"

	"github.com/ydb-platform/ydb-go-sdk/v3/credentials"

	"a.yandex-team.ru/cloud/dataplatform/internal/logger"
	"a.yandex-team.ru/kikimr/public/sdk/go/persqueue"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/transfer_manager/go/pkg/config"
	yccreds "a.yandex-team.ru/transfer_manager/go/pkg/yc/credentials"
)

type Option func(a *Agent)

func WithTickInterval(tick time.Duration) Option {
	return func(a *Agent) {
		a.tick = tick
	}
}

func WithCPUProfile(duration time.Duration) Option {
	return func(a *Agent) {
		a.CPUProfile = true
		a.CPUProfileDuration = duration
	}
}

func WithHeapProfile() Option {
	return func(a *Agent) {
		a.HeapProfile = true
	}
}

func WithBlockProfile() Option {
	return func(a *Agent) {
		a.BlockProfile = true
	}
}

func WithMutexProfile() Option {
	return func(a *Agent) {
		a.MutexProfile = true
	}
}

func WithGoroutineProfile() Option {
	return func(a *Agent) {
		a.GoroutineProfile = true
	}
}

func WithThreadcreateProfile() Option {
	return func(a *Agent) {
		a.ThreadcreateProfile = true
	}
}

func WithLabels(args map[string]string) Option {
	return func(a *Agent) {
		a.labels = args
	}
}

func WithLogbrokerOptions(option config.LogLogbroker) Option {
	return func(a *Agent) {
		a.writerOptions, _ = NewPQConfig(&option)
	}
}

func NewPQConfig(lbConfig *config.LogLogbroker) (opts *persqueue.WriterOptions, err error) {
	var creds credentials.Credentials
	switch specificCreds := lbConfig.Creds.(type) {
	case *config.UseCloudCreds:
		creds, err = yccreds.NewIamCreds(logger.Log)
		if err != nil {
			return nil, xerrors.Errorf("Cannot init persqueue writer config without credentials client: %w", err)
		}
	case *config.LogbrokerOAuthToken:
		creds = credentials.NewAccessTokenCredentials(string(specificCreds.Token))
	}

	var tlsConfig *tls.Config
	switch tlsMode := lbConfig.TLSMode.(type) {
	case *config.TLSModeDisabled:
	case *config.TLSModeEnabled:
		tlsConfig, err = config.TLSConfig(tlsMode.CACertificatePath)
		if err != nil {
			return nil, xerrors.Errorf("Cannot initialize TLS config: %w", err)
		}
	}

	sourceID := lbConfig.SourceID
	if sourceID == "" {
		sourceID, _ = os.Hostname()
	}

	return &persqueue.WriterOptions{
		Topic:          lbConfig.Topic,
		Endpoint:       lbConfig.Endpoint,
		Database:       lbConfig.Database,
		Credentials:    creds,
		TLSConfig:      tlsConfig,
		SourceID:       []byte(sourceID),
		RetryOnFailure: true,
		MaxMemory:      50 * 1024 * 1024,
	}, nil
}

func WithLogger(logger log.Logger) Option {
	return func(a *Agent) {
		a.logger = logger
	}
}
