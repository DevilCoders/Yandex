package roller

import (
	"context"
	"encoding/json"
	"fmt"

	deploymodels "a.yandex-team.ru/cloud/mdb/deploy/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/internal/retry"
	"a.yandex-team.ru/cloud/mdb/katan/internal/katandb"
	"a.yandex-team.ru/cloud/mdb/katan/internal/tags"
	"a.yandex-team.ru/cloud/mdb/katan/katan/internal/cluster"
	"a.yandex-team.ru/cloud/mdb/katan/katan/internal/models"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type Config struct {
	Rollout              cluster.Config `json:"rollout" yaml:"rollout"`
	SaveChangeMaxRetries uint64         `json:"save_change_max_retries" yaml:"save_change_max_retries"`
}

func DefaultConfig() Config {
	return Config{
		Rollout:              cluster.DefaultConfig(),
		SaveChangeMaxRetries: 10,
	}
}

type RollApis = cluster.RollApis

// Roller roll given rollout
type Roller struct {
	kdb      katandb.KatanDB
	rollApis cluster.RollApis
	L        log.Logger
	cfg      Config
}

func New(config Config, kdb katandb.KatanDB, rollApis RollApis, L log.Logger) *Roller {
	return &Roller{
		kdb:      kdb,
		rollApis: rollApis,
		L:        L,
		cfg:      config,
	}
}

// Group all host in given deploy by clusters
func GroupByClusters(clusters []katandb.Cluster, hosts []katandb.Host) ([]models.Cluster, error) {
	byID := make(map[string]models.Cluster, len(clusters))

	for _, c := range clusters {
		t, err := tags.UnmarshalClusterTags(c.Tags)
		if err != nil {
			return nil, xerrors.Errorf("fail to unmarshal %q tags: %w", c.ID, err)
		}
		byID[c.ID] = models.Cluster{
			ID:    c.ID,
			Tags:  t,
			Hosts: make(map[string]tags.HostTags),
		}
	}

	for _, h := range hosts {
		if _, ok := byID[h.ClusterID]; !ok {
			return nil, xerrors.Errorf("host %q from cluster %q not present in clusters", h.FQDN, h.ClusterID)
		}
		t, err := tags.UnmarshalHostTags(h.Tags)
		if err != nil {
			return nil, xerrors.Errorf("fail to unmarshal %q tags: %w", h.FQDN, err)
		}
		byID[h.ClusterID].Hosts[h.FQDN] = t
	}

	ret := make([]models.Cluster, 0, len(byID))
	for _, v := range byID {
		ret = append(ret, v)
	}

	return ret, nil
}

// Rollout process given rollout
func (k *Roller) Rollout(ctx context.Context, rollout katandb.Rollout) error {
	var commands []deploymodels.CommandDef
	if err := json.Unmarshal([]byte(rollout.Commands), &commands); err != nil {
		return xerrors.Errorf("malformed commands: %w", err)
	}

	options, err := cluster.UnmarshalRollOptions(rollout.Options)
	if err != nil {
		return err
	}

	rollClusters, err := k.kdb.RolloutClusters(ctx, rollout.ID)
	if err != nil {
		return xerrors.Errorf("unable to get clusters: %w", err)
	}
	rollHosts, err := k.kdb.RolloutClustersHosts(ctx, rollout.ID)
	if err != nil {
		return xerrors.Errorf("unable to get hosts: %w", err)
	}

	clusters, err := GroupByClusters(rollClusters, rollHosts)
	if err != nil {
		return err
	}
	k.L.Debugf("have %d clusters", len(clusters))

	return k.rollClusters(ctx, commands, rollout, clusters, options)
}

func (k *Roller) rollClusters(ctx context.Context, commands []deploymodels.CommandDef, rollout katandb.Rollout, clusters []models.Cluster, options cluster.RollOptions) error {
	onChangeRetry := retry.New(retry.Config{
		MaxRetries: k.cfg.SaveChangeMaxRetries,
	})

	onClusterChanges := func(c models.ClusterChange) {
		_ = onChangeRetry.RetryWithLog(
			ctx,
			func() error {
				return k.kdb.MarkClusterRollout(ctx, rollout.ID, c.ClusterID, c.State, c.Comment)
			},
			fmt.Sprintf("mark cluster change %+v", c),
			k.L,
		)
	}

	onHostChanges := func(c models.Shipment) {
		if onChangeRetry.RetryWithLog(
			ctx,
			func() error {
				return k.kdb.AddRolloutShipment(ctx, rollout.ID, c.FQDNs, c.ShipmentID)
			},
			fmt.Sprintf("add rollout shipment %+v", c),
			k.L,
		) == nil {
			_ = onChangeRetry.RetryWithLog(
				ctx,
				func() error {
					return k.kdb.TouchClusterRollout(ctx, rollout.ID, c.ClusterID)
				},
				fmt.Sprintf("touch cluster rollout %d", rollout.ID),
				k.L,
			)
		}
	}

	return RollClusters(
		clusters,
		func(cl models.Cluster) error {
			return cluster.Roll(
				ctx,
				log.With(k.L, log.String("cluster", cl.ID)),
				k.cfg.Rollout,
				k.rollApis,
				onClusterChanges,
				onHostChanges,
				rollout.ID,
				cl,
				commands,
				options,
			)
		},
		StrictLinearAccelerator(rollout.Parallel),
		k.L)
}
