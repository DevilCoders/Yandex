package admin

import (
	"context"

	"a.yandex-team.ru/library/go/yandex/solomon"
)

type SyncOptions struct {
	// Call provided logging callbacks, without updating configuration.
	DryRun bool

	// Callbacks invoked before updating configurations.
	ChangeCluster func(from, to *solomon.Cluster)
	ChangeService func(from, to *solomon.Service)
	ChangeShard   func(from, to *solomon.Shard)

	// Remove all objects that are not listed in provided configuration.
	RemoveGarbage bool
}

func (o *SyncOptions) changeCluster(from, to *solomon.Cluster) {
	if o.ChangeCluster != nil {
		o.ChangeCluster(from, to)
	}
}

func (o *SyncOptions) changeService(from, to *solomon.Service) {
	if o.ChangeService != nil {
		o.ChangeService(from, to)
	}
}

func (o *SyncOptions) changeShard(from, to *solomon.Shard) {
	if o.ChangeShard != nil {
		o.ChangeShard(from, to)
	}
}

func equalClusters(a, b solomon.Cluster) bool {
	return a.IsEqual(b)
}

func equalServices(a, b solomon.Service) bool {
	return a.IsEqual(b)
}

func equalShards(a, b solomon.Shard) bool {
	return a.IsEqual(b)
}

// Sync edits solomon project configuration, updating clusters, services and shards.
//
// Clusters and services are matched by their ID. Shards are matched by the (clusterID, serviceID) pair.
func Sync(
	ctx context.Context,
	client solomon.AdminClient,
	expectedClusters []solomon.Cluster,
	expectedServices []solomon.Service,
	expectedShards []solomon.Shard,
	options *SyncOptions,
) error {
	var err error

	var shards struct {
		Actual, Expected []solomon.Shard
	}

	shards.Expected = expectedShards
	shards.Actual, err = client.ListShards(ctx)
	if err != nil {
		return err
	}
	addShards, removeShards, updateShards := Diff(ReflectDiff(&shards, equalShards))

	var clusters struct {
		Actual, Expected []solomon.Cluster
	}

	clusters.Expected = expectedClusters
	clusters.Actual, err = client.ListClusters(ctx)
	if err != nil {
		return err
	}
	addClusters, removeClusters, updateClusters := Diff(ReflectDiff(&clusters, equalClusters))

	var services struct {
		Actual, Expected []solomon.Service
	}

	services.Expected = expectedServices
	services.Actual, err = client.ListServices(ctx)
	if err != nil {
		return err
	}
	addServices, removeServices, updateServices := Diff(ReflectDiff(&services, equalServices))

	if options.RemoveGarbage {
		for _, i := range removeShards {
			options.changeShard(&shards.Actual[i], nil)

			if options.DryRun {
				continue
			}

			if err := client.DeleteShard(ctx, shards.Actual[i].ID); err != nil {
				return err
			}
		}

		for _, i := range removeClusters {
			options.changeCluster(&clusters.Actual[i], nil)

			if options.DryRun {
				continue
			}

			if err := client.DeleteCluster(ctx, clusters.Actual[i].ID); err != nil {
				return err
			}
		}

		for _, i := range removeServices {
			options.changeService(&services.Actual[i], nil)

			if options.DryRun {
				continue
			}

			if err := client.DeleteService(ctx, services.Actual[i].ID); err != nil {
				return err
			}
		}
	}

	for _, i := range addClusters {
		options.changeCluster(nil, &clusters.Expected[i])

		if options.DryRun {
			continue
		}

		if _, err := client.CreateCluster(ctx, clusters.Expected[i]); err != nil {
			return err
		}
	}

	for _, u := range updateClusters {
		options.changeCluster(&clusters.Actual[u.From], &clusters.Expected[u.To])

		if options.DryRun {
			continue
		}

		updated := &clusters.Expected[u.To]
		updated.Version = clusters.Actual[u.From].Version

		if _, err := client.UpdateCluster(ctx, *updated); err != nil {
			return err
		}
	}

	for _, i := range addServices {
		options.changeService(nil, &services.Expected[i])

		if options.DryRun {
			continue
		}

		if _, err := client.CreateService(ctx, services.Expected[i]); err != nil {
			return err
		}
	}

	for _, u := range updateServices {
		options.changeService(&services.Actual[u.From], &services.Expected[u.To])

		if options.DryRun {
			continue
		}

		updated := &services.Expected[u.To]
		updated.Version = services.Actual[u.From].Version

		if _, err := client.UpdateService(ctx, *updated); err != nil {
			return err
		}
	}

	for _, i := range addShards {
		options.changeShard(nil, &shards.Expected[i])

		if options.DryRun {
			continue
		}

		shards.Expected[i].GenerateID()

		if _, err := client.CreateShard(ctx, shards.Expected[i]); err != nil {
			return err
		}
	}

	for _, u := range updateShards {
		options.changeShard(&shards.Actual[u.From], &shards.Expected[u.To])

		if options.DryRun {
			continue
		}

		updated := &shards.Expected[u.To]
		updated.ID = shards.Actual[u.From].ID
		updated.Version = shards.Actual[u.From].Version

		if _, err := client.UpdateShard(ctx, *updated); err != nil {
			return err
		}
	}

	return nil
}
