package internal

import (
	"context"
	"time"

	"google.golang.org/grpc"

	chv1 "a.yandex-team.ru/cloud/mdb/datacloud/private_api/datacloud/clickhouse/v1"
	kfv1 "a.yandex-team.ru/cloud/mdb/datacloud/private_api/datacloud/kafka/v1"
	dcv1 "a.yandex-team.ru/cloud/mdb/datacloud/private_api/datacloud/v1"
	"a.yandex-team.ru/cloud/mdb/internal/app"
	"a.yandex-team.ru/cloud/mdb/internal/compute/resmanager"
	resmanagergrpc "a.yandex-team.ru/cloud/mdb/internal/compute/resmanager/grpc"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil"
	"a.yandex-team.ru/cloud/mdb/internal/ready"
	"a.yandex-team.ru/cloud/mdb/mdb-reaper/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/mdb-reaper/internal/metadb/pg"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type App struct {
	*app.App
	config         Config
	rm             resmanager.Client
	meta           metadb.MetaDB
	kfClusterSvc   kfv1.ClusterServiceClient
	kfOperationSvc kfv1.OperationServiceClient
	chClusterSvc   chv1.ClusterServiceClient
	chOperationSvc chv1.OperationServiceClient
}

func NewApp(baseApp *app.App, opts ...AppOption) (*App, error) {
	a := &App{App: baseApp}
	for _, opt := range opts {
		opt(a)
	}

	if a.rm == nil {
		resClient, err := resmanagergrpc.NewClient(
			a.ShutdownContext(),
			a.config.ResourceManager.Target,
			"mdb-reaper",
			a.config.ResourceManager.ClientConfig,
			a.ServiceAccountCredentials(),
			a.L(),
		)
		if err != nil {
			return nil, err
		}
		a.rm = resClient
	}

	var dcConn *grpc.ClientConn
	if a.chClusterSvc == nil || a.chOperationSvc == nil || a.kfClusterSvc == nil || a.kfOperationSvc == nil {
		var err error
		dcConn, err = grpcutil.NewConn(
			a.ShutdownContext(),
			a.config.DoubleCloud.Target,
			"mdb-reaper",
			a.config.DoubleCloud.ClientConfig,
			a.L(),
			grpcutil.WithClientCredentials(a.ServiceAccountCredentials()),
		)
		if err != nil {
			return nil, err
		}
	}

	if a.chClusterSvc == nil {
		a.chClusterSvc = chv1.NewClusterServiceClient(dcConn)
	}

	if a.chOperationSvc == nil {
		a.chOperationSvc = chv1.NewOperationServiceClient(dcConn)
	}

	if a.kfClusterSvc == nil {
		a.kfClusterSvc = kfv1.NewClusterServiceClient(dcConn)
	}

	if a.kfOperationSvc == nil {
		a.kfOperationSvc = kfv1.NewOperationServiceClient(dcConn)
	}

	if a.meta == nil {
		meta, err := pg.New(a.config.MetaDB, a.L())
		if err != nil {
			return nil, err
		}
		a.meta = meta
	}

	return a, nil
}

func (a *App) collectUnusedClusters(ctx context.Context, clouds metadb.ClusterIDsByCloudID) ([]metadb.Cluster, error) {
	var allClusterIDs []string
	for _, clusterIDs := range clouds {
		allClusterIDs = append(allClusterIDs, clusterIDs...)
	}

	clusters, err := a.meta.Clusters(ctx, allClusterIDs)
	if err != nil {
		return nil, xerrors.Errorf("get information about clusters: %w", err)
	}
	var stopClusterIDs []metadb.Cluster
	for _, cluster := range clusters {
		if time.Since(cluster.LastActionAt) > time.Hour*24*3 {
			stopClusterIDs = append(stopClusterIDs, cluster)
		}
	}

	return stopClusterIDs, nil
}

func (a *App) collectClustersInBlockedCloud(ctx context.Context, clouds metadb.ClusterIDsByCloudID) ([]metadb.Cluster, error) {
	var blockedCloudIDs []string
	for cloudID := range clouds {
		cloud, err := a.rm.Cloud(ctx, cloudID)
		if err != nil {
			return nil, xerrors.Errorf("get information about cloud %q: %w", cloudID, err)
		}
		if cloud.Status == resmanager.CloudStatusBlocked {
			a.L().Infof("Found a blocked cloud %q(%q) with running clusters", cloud.CloudID, cloud.Name)
			blockedCloudIDs = append(blockedCloudIDs, cloud.CloudID)
		}
	}

	if len(blockedCloudIDs) == 0 {
		a.L().Info("There is now blocked clouds with running clusters. Exit.")
		return nil, nil
	}

	var stopClusterIDs []string
	for _, cloudID := range blockedCloudIDs {
		stopClusterIDs = append(stopClusterIDs, clouds[cloudID]...)
	}
	clusters, err := a.meta.Clusters(ctx, stopClusterIDs)
	if err != nil {
		return nil, xerrors.Errorf("get information about clusters: %w", err)
	}
	return clusters, nil
}

func (a *App) Reap() error {
	ctx := a.ShutdownContext()
	if err := ready.WaitWithTimeout(ctx, 10*time.Second, a.meta, &ready.DefaultErrorTester{Name: "metadb", L: a.L()}, time.Second); err != nil {
		return xerrors.Errorf("init metadb: %w", err)
	}
	clouds, err := a.meta.CloudsWithRunningClusters(ctx, a.config.StopAllUnused)
	if err != nil {
		return xerrors.Errorf("get all running clusters: %w", err)
	}
	if len(clouds) == 0 {
		a.L().Info("There is now clouds with running clusters. Exit.")
		return nil
	}

	var clustersToStop []metadb.Cluster
	if a.config.StopAllUnused {
		clustersToStop, err = a.collectUnusedClusters(ctx, clouds)
		if err != nil {
			return xerrors.Errorf("collect unused clusters: %w", err)
		}
	} else {
		clustersToStop, err = a.collectClustersInBlockedCloud(ctx, clouds)
		if err != nil {
			return xerrors.Errorf("collect clusters at blocked clouds: %w", err)
		}
	}
	if len(clustersToStop) == 0 {
		a.L().Info("There is now clusters to stop. Exit.")
		return nil
	}

	for _, cluster := range clustersToStop {
		if a.config.DryRun {
			a.L().Infof("Cluster %q with type %q plan to be stopped", cluster.ID, cluster.Type)
			continue
		}
		switch cluster.Type {
		case metadb.ClickhouseCluster:
			resp, err := a.chClusterSvc.Stop(ctx, &chv1.StopClusterRequest{ClusterId: cluster.ID})
			if err != nil {
				return xerrors.Errorf("stop ClickHouse cluster %q: %w", cluster.ID, err)
			}
			err = a.WaitForOperationComplete(ctx, resp.OperationId, func() (*dcv1.Operation, error) {
				return a.chOperationSvc.Get(ctx, &chv1.GetOperationRequest{OperationId: resp.OperationId})
			})
			if err != nil {
				return xerrors.Errorf("wait for operation %q to stop ClickHouse cluster %q: %w", resp.OperationId, cluster.ID, err)
			}
			a.L().Infof("ClickHouse cluster %q stopped by operation %q", cluster.ID, resp.OperationId)
		case metadb.KafkaCluster:
			resp, err := a.kfClusterSvc.Stop(ctx, &kfv1.StopClusterRequest{ClusterId: cluster.ID})
			if err != nil {
				return xerrors.Errorf("stop Kafka cluster %q: %w", cluster.ID, err)
			}
			err = a.WaitForOperationComplete(ctx, resp.OperationId, func() (*dcv1.Operation, error) {
				return a.kfOperationSvc.Get(ctx, &kfv1.GetOperationRequest{OperationId: resp.OperationId})
			})
			if err != nil {
				return xerrors.Errorf("wait for operation %q to stop Kafka cluster %q: %w", resp.OperationId, cluster.ID, err)
			}
			a.L().Infof("Kafka cluster %q stopped by operation %q", cluster.ID, resp.OperationId)
		default:
			return xerrors.Errorf("unknown cluster type %q", cluster.Type)
		}
	}
	a.L().Info("Stop completed")
	return nil
}

type OperationGetter = func() (*dcv1.Operation, error)

func (a *App) WaitForOperationComplete(ctx context.Context, operationID string, getOp OperationGetter) error {
	ticker := time.NewTicker(2 * time.Second)
	defer ticker.Stop()
	for {
		select {
		case <-ticker.C:
			op, err := getOp()
			if err != nil {
				return xerrors.Errorf("get cluster operation %q failed: %w", operationID, err)
			}
			if op.Status != dcv1.Operation_STATUS_DONE {
				a.L().Infof("Still waiting for operation %q", op.Id)
				continue
			}
			if op.Error != nil {
				return xerrors.Errorf("stop operation failed with code %d and message %q", op.Error.Code, op.Error.Message)
			}
			a.L().Infof("Waiting for operation %q completed.", op.Id)
			return nil
		case <-ctx.Done():
			return xerrors.Errorf("waiting operation %q timeout", operationID)
		}
	}
}
