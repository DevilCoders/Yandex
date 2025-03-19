// TODO: Split into internal/app
package main

import (
	"context"
	"crypto/tls"
	"fmt"
	"io/ioutil"
	"log"
	"os"
	"os/signal"
	"syscall"

	"github.com/golang/protobuf/proto"
	"github.com/spf13/cobra"
	"google.golang.org/grpc/credentials"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/accounting"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/auth"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/clients/nbs"
	server_config "a.yandex-team.ru/cloud/disk_manager/internal/pkg/configs/server/config"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/logging"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/monitoring"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/persistence"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks"
	tasks_storage "a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/storage"
)

////////////////////////////////////////////////////////////////////////////////

func parseConfig(
	configFileName string,
	config *server_config.ServerConfig,
) error {

	configBytes, err := ioutil.ReadFile(configFileName)
	if err != nil {
		return fmt.Errorf(
			"failed to read config file %v: %w",
			configFileName,
			err,
		)
	}

	err = proto.UnmarshalText(string(configBytes), config)
	if err != nil {
		return fmt.Errorf(
			"failed to parse config file %v as protobuf: %w",
			configFileName,
			err,
		)
	}

	return nil
}

////////////////////////////////////////////////////////////////////////////////

func createTransportCredentials(
	certFiles []*server_config.Cert,
) (credentials.TransportCredentials, error) {

	certificates := make([]tls.Certificate, 0, len(certFiles))
	for _, certFile := range certFiles {
		cert, err := tls.LoadX509KeyPair(
			certFile.GetCertFile(),
			certFile.GetPrivateKeyFile(),
		)
		if err != nil {
			return nil, fmt.Errorf(
				"failed to load cert file %v: %w",
				certFile.CertFile,
				err,
			)
		}

		certificates = append(certificates, cert)
	}

	cfg := &tls.Config{
		Certificates: certificates,
		MinVersion:   tls.VersionTLS12,
	}
	// TODO: https://golang.org/doc/go1.14#crypto/tls
	//nolint:SA1019
	cfg.BuildNameToCertificate()

	return credentials.NewTLS(cfg), nil
}

////////////////////////////////////////////////////////////////////////////////

func ignoreSigpipe() {
	c := make(chan os.Signal, 1)
	signal.Notify(c, syscall.SIGPIPE)
}

////////////////////////////////////////////////////////////////////////////////

func run(
	config *server_config.ServerConfig,
) error {

	var err error

	ignoreSigpipe()

	// Use cancellable context.
	ctx := context.Background()

	var hostname string

	// TODO: move hostname from GrpcConfig.
	if config.GrpcConfig != nil {
		hostname = config.GetGrpcConfig().GetHostname()
	}

	if len(hostname) == 0 {
		hostname, err = os.Hostname()
		if err != nil {
			return fmt.Errorf("failed to get hostname: %w", err)
		}
	}

	logger := logging.CreateLogger(config.LoggingConfig)
	ctx = logging.SetLogger(ctx, logger)

	mon := monitoring.CreateMonitoring(config.MonitoringConfig)
	mon.Start(ctx)

	accounting.Init(mon.CreateSolomonRegistry("accounting"))

	creds := auth.NewCredentials(ctx, config.AuthConfig)

	db, err := persistence.CreateYDBClient(
		ctx,
		config.PersistenceConfig,
		persistence.WithRegistry(mon.CreateSolomonRegistry("ydb")),
		persistence.WithCredentials(creds),
	)
	if err != nil {
		return fmt.Errorf("failed to connect to DB: %w", err)
	}

	defer db.Close(ctx)

	taskMetricsRegistry := mon.CreateSolomonRegistry("tasks")
	taskStorage, err := tasks_storage.CreateStorage(
		config.TasksConfig,
		taskMetricsRegistry,
		db,
	)
	if err != nil {
		return fmt.Errorf("failed to create task storage: %w", err)
	}

	taskRegistry := tasks.CreateRegistry()
	taskScheduler, err := tasks.CreateScheduler(
		ctx,
		taskRegistry,
		taskStorage,
		config.TasksConfig,
	)
	if err != nil {
		return fmt.Errorf("failed to create task scheduler: %w", err)
	}

	nbsClientMetricsRegistry := mon.CreateSolomonRegistry("nbs_client")
	nbsFactory, err := nbs.CreateFactoryWithCreds(
		ctx,
		config.NbsConfig,
		creds,
		nbsClientMetricsRegistry,
	)
	if err != nil {
		return fmt.Errorf("failed to create NBS factory: %w", err)
	}

	if config.DataplaneConfig != nil {
		return runDataplane(
			ctx,
			config,
			hostname,
			mon,
			creds,
			db,
			taskStorage,
			taskRegistry,
			taskScheduler,
			nbsFactory,
		)
	}

	return runControlplane(
		ctx,
		config,
		hostname,
		logger,
		mon,
		creds,
		db,
		taskStorage,
		taskRegistry,
		taskScheduler,
		nbsFactory,
	)
}

////////////////////////////////////////////////////////////////////////////////

func main() {
	var configFileName string
	config := &server_config.ServerConfig{}

	var rootCmd = &cobra.Command{
		Use:   "yc-disk-manager",
		Short: "Disk Manager server",
		PersistentPreRunE: func(cmd *cobra.Command, args []string) error {
			return parseConfig(configFileName, config)
		},
		RunE: func(cmd *cobra.Command, args []string) error {
			return run(config)
		},
	}
	rootCmd.Flags().StringVar(
		&configFileName,
		"config",
		"/etc/yc/disk-manager/server-config.txt",
		"Path to the config file",
	)

	if err := rootCmd.Execute(); err != nil {
		log.Fatalf("Error: %v", err)
	}
}
