package worker

import (
	"context"
	"time"

	"github.com/aws/aws-sdk-go/aws/credentials"

	"a.yandex-team.ru/cloud/mdb/internal/httputil"
	"a.yandex-team.ru/cloud/mdb/internal/ready"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/pg"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/worker/config"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/worker/network"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/worker/network/aws"
	"a.yandex-team.ru/library/go/core/log"
)

type Worker struct {
	log   log.Logger
	vpcdb vpcdb.VPCDB

	maxConcurrentTasks int

	awsNetworkProviders map[string]network.Service
}

func (w *Worker) Run(ctx context.Context) {
	workDuration := 10 * time.Second
	ticker := time.NewTicker(workDuration)
	defer ticker.Stop()
	w.log.Info("Init worker loop")

	w.Iteration(ctx)
	for {
		select {
		case <-ticker.C:
			w.Iteration(ctx)

		case <-ctx.Done():
			w.log.Info("Stop worker loop")
			return
		}
	}
}

func NewCustomWorker(
	l log.Logger,
	db vpcdb.VPCDB,
	maxConcurrentTasks int,
	awsNetworkProviders map[string]network.Service,
) *Worker {
	return &Worker{
		log:                 l,
		vpcdb:               db,
		maxConcurrentTasks:  maxConcurrentTasks,
		awsNetworkProviders: awsNetworkProviders,
	}
}

func NewWorkerFromConfig(ctx context.Context, logger log.Logger, config config.Config) *Worker {
	db, err := pg.New(config.Vpcdb, log.With(logger, log.String("cluster", config.Vpcdb.DB)))
	if err != nil {
		logger.Fatal("could not create vpcdb client", log.Error(err))
	}

	if err = ready.WaitWithTimeout(ctx, 10*time.Second, db, &ready.DefaultErrorTester{Name: "vpc database", L: logger}, time.Second); err != nil {
		logger.Fatal("vpcdb is not ready", log.Error(err))
	}

	httpClient, err := httputil.NewClient(config.Aws.HTTP, logger)
	if err != nil {
		logger.Fatal("could not create http client", log.Error(err))
	}

	creds := credentials.NewStaticCredentials(config.Aws.AccessKeyID, config.Aws.SecretAccessKey.Unmask(), "")
	awsNetworkProviders := make(map[string]network.Service)
	for _, region := range config.Aws.Regions {
		awsNetworkProviders[region] = aws.NewService(creds, region, httpClient.Client, logger, db, config.Aws.ControlPlane)
	}

	return NewCustomWorker(logger, db, config.MaxConcurrentTasks, awsNetworkProviders)
}
