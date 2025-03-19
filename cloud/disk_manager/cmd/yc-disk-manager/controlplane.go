package main

import (
	"context"
	"fmt"
	"net"
	"time"

	"google.golang.org/grpc"
	"google.golang.org/grpc/keepalive"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/auth"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/clients/nbs"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/clients/nfs"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/clients/snapshot"
	server_config "a.yandex-team.ru/cloud/disk_manager/internal/pkg/configs/server/config"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/dataplane"
	grpc_facade "a.yandex-team.ru/cloud/disk_manager/internal/pkg/grpc"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/logging"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/monitoring"
	performance_config "a.yandex-team.ru/cloud/disk_manager/internal/pkg/performance/config"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/persistence"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/resources"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/disks"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/filesystem"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/images"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/placementgroup"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/pools"
	pools_storage "a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/pools/storage"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/snapshots"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/transfer"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks"
	tasks_storage "a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/storage"
)

////////////////////////////////////////////////////////////////////////////////

func runControlplane(
	ctx context.Context,
	config *server_config.ServerConfig,
	hostname string,
	logger logging.Logger,
	mon *monitoring.Monitoring,
	creds auth.Credentials,
	db *persistence.YDBClient,
	taskStorage tasks_storage.Storage,
	taskRegistry *tasks.Registry,
	taskScheduler tasks.Scheduler,
	nbsFactory nbs.Factory,
) error {

	address := fmt.Sprintf(":%d", config.GetGrpcConfig().GetPort())

	listener, err := net.Listen("tcp", address)
	if err != nil {
		return fmt.Errorf("failed to listen on %v: %w", config.GetGrpcConfig().GetPort(), err)
	}

	logging.Info(ctx, "Disk Manager listening on %v", address)

	// Data plane tasks registration is also needed for control plane nodes.
	// TODO: reconsider it.
	err = dataplane.Register(
		ctx,
		taskRegistry,
		taskScheduler,
		nbsFactory,
		nil, // storage
		nil, // legacyStorage
		config.GetDataplaneConfig(),
	)
	if err != nil {
		return fmt.Errorf("failed to register dataplane tasks: %w", err)
	}

	poolStorage, err := pools_storage.CreateStorage(config.GetPoolsConfig(), db)
	if err != nil {
		return fmt.Errorf("failed to create pools.Storage: %w", err)
	}

	poolService := pools.CreateService(taskScheduler, poolStorage)

	snapshotClientMetricsRegistry := mon.CreateSolomonRegistry("snapshot_client")
	snapshotFactory := snapshot.CreateFactoryWithCreds(
		config.GetSnapshotConfig(),
		creds,
		snapshotClientMetricsRegistry,
	)

	fsStorageFolder := ""
	if config.GetFilesystemConfig() != nil {
		fsStorageFolder = config.GetFilesystemConfig().GetStorageFolder()
	}

	resourceStorage, err := resources.CreateStorage(
		config.GetDisksConfig().GetStorageFolder(),
		config.GetImagesConfig().GetStorageFolder(),
		config.GetSnapshotsConfig().GetStorageFolder(),
		fsStorageFolder,
		config.GetPlacementGroupConfig().GetStorageFolder(),
		db,
	)
	if err != nil {
		return fmt.Errorf("failed to create resources.Storage: %w", err)
	}

	transferService := transfer.CreateService(taskScheduler)

	err = pools.Register(
		ctx,
		config.GetPoolsConfig(),
		taskRegistry,
		taskScheduler,
		poolStorage,
		nbsFactory,
		snapshotFactory,
		transferService,
		resourceStorage,
	)
	if err != nil {
		return fmt.Errorf("failed to register pools tasks: %w", err)
	}

	performanceConfig := config.PerformanceConfig
	if performanceConfig == nil {
		performanceConfig = &performance_config.PerformanceConfig{}
	}

	err = disks.Register(
		ctx,
		config.GetDisksConfig(),
		performanceConfig,
		resourceStorage,
		taskRegistry,
		taskScheduler,
		poolService,
		nbsFactory,
		transferService,
	)
	if err != nil {
		return fmt.Errorf("failed to register disks tasks: %w", err)
	}

	err = transfer.Register(
		ctx,
		config.GetTransferConfig(),
		taskRegistry,
		snapshotFactory,
		nbsFactory,
	)
	if err != nil {
		return fmt.Errorf("failed to register transfer tasks: %w", err)
	}

	err = images.Register(
		ctx,
		config.GetImagesConfig(),
		performanceConfig,
		taskRegistry,
		taskScheduler,
		resourceStorage,
		nbsFactory,
		snapshotFactory,
		poolService,
	)
	if err != nil {
		return fmt.Errorf("failed to register images tasks: %w", err)
	}

	err = snapshots.Register(
		ctx,
		config.GetSnapshotsConfig(),
		performanceConfig,
		taskRegistry,
		taskScheduler,
		resourceStorage,
		nbsFactory,
		snapshotFactory,
	)
	if err != nil {
		return fmt.Errorf("failed to register snapshots tasks: %w", err)
	}

	var filesystemService filesystem.Service
	if config.GetFilesystemConfig() != nil {
		nfsClientMetricsRegistry := mon.CreateSolomonRegistry("nfs_client")
		nfsFactory := nfs.CreateFactoryWithCreds(config.GetNfsConfig(), creds, nfsClientMetricsRegistry)

		err = filesystem.Register(
			ctx,
			config.GetFilesystemConfig(),
			taskScheduler,
			taskRegistry,
			resourceStorage,
			nfsFactory)
		if err != nil {
			return fmt.Errorf("failed to register filesystem tasks: %w", err)
		}

		filesystemService = filesystem.CreateService(
			taskScheduler,
			config.GetFilesystemConfig(),
			nfsFactory,
		)
	}

	err = placementgroup.Register(
		ctx,
		config.GetPlacementGroupConfig(),
		taskRegistry,
		taskScheduler,
		resourceStorage,
		nbsFactory,
	)
	if err != nil {
		return fmt.Errorf("failed to register placement_group tasks: %w", err)
	}

	runnerMetricsRegistry := mon.CreateSolomonRegistry("runners")

	err = tasks.StartRunners(
		ctx,
		taskStorage,
		taskRegistry,
		runnerMetricsRegistry,
		config.GetTasksConfig(),
		hostname,
	)
	if err != nil {
		return fmt.Errorf("failed to start runners: %w", err)
	}

	accessServiceClient, err := auth.NewAccessServiceClientWithCreds(
		config.GetAuthConfig(),
		creds,
	)
	if err != nil {
		return fmt.Errorf("failed to create access service client: %w", err)
	}

	grpcFacadeMetricsRegistry := mon.CreateSolomonRegistry("grpc_facade")

	keepAliveTime, err := time.ParseDuration(config.GetGrpcConfig().GetKeepAlive().GetTime())
	if err != nil {
		return err
	}

	keepAliveTimeout, err := time.ParseDuration(config.GetGrpcConfig().GetKeepAlive().GetTimeout())
	if err != nil {
		return err
	}

	keepAliveMinTime, err := time.ParseDuration(config.GetGrpcConfig().GetKeepAlive().GetMinTime())
	if err != nil {
		return err
	}

	serverOptions := []grpc.ServerOption{
		grpc.KeepaliveParams(keepalive.ServerParameters{
			Time:    keepAliveTime,
			Timeout: keepAliveTimeout,
		}),
		grpc.KeepaliveEnforcementPolicy(keepalive.EnforcementPolicy{
			MinTime:             keepAliveMinTime,
			PermitWithoutStream: config.GetGrpcConfig().GetKeepAlive().GetPermitWithoutStream(),
		}),
	}
	var s *grpc.Server
	if config.GetGrpcConfig().GetInsecure() {
		serverOptions = append(
			serverOptions,
			grpc.UnaryInterceptor(grpc_facade.CreateInterceptor(
				logger,
				grpcFacadeMetricsRegistry,
				accessServiceClient,
			)),
		)
		s = grpc.NewServer(serverOptions...)
	} else {
		transportCreds, err := createTransportCredentials(config.GetGrpcConfig().GetCerts())
		if err != nil {
			return fmt.Errorf("failed to create creds: %w", err)
		}

		serverOptions = append(
			serverOptions,
			grpc.UnaryInterceptor(grpc_facade.CreateInterceptor(
				logger,
				grpcFacadeMetricsRegistry,
				accessServiceClient,
			)),
			grpc.Creds(transportCreds),
		)

		s = grpc.NewServer(serverOptions...)
	}
	grpc_facade.RegisterDiskService(
		s,
		taskScheduler,
		disks.CreateService(
			taskScheduler,
			config.GetDisksConfig(),
			nbsFactory,
			poolService,
		),
	)
	grpc_facade.RegisterImageService(
		s,
		taskScheduler,
		images.CreateService(taskScheduler, config.GetImagesConfig()),
	)
	grpc_facade.RegisterOperationService(
		s,
		taskScheduler,
	)
	grpc_facade.RegisterPlacementGroupService(
		s,
		taskScheduler,
		placementgroup.CreateService(taskScheduler, nbsFactory),
	)
	grpc_facade.RegisterSnapshotService(
		s,
		taskScheduler,
		snapshots.CreateService(taskScheduler, config.GetSnapshotsConfig(), transferService),
	)
	grpc_facade.RegisterPrivateService(
		s,
		taskScheduler,
		nbsFactory,
		poolService,
		resourceStorage,
	)

	if filesystemService != nil {
		grpc_facade.RegisterFilesystemService(
			s,
			taskScheduler,
			filesystemService,
		)
	}

	return s.Serve(listener)
}
