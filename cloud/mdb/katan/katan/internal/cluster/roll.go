package cluster

import (
	"bytes"
	"context"
	"encoding/json"
	"fmt"
	"time"

	"a.yandex-team.ru/cloud/mdb/deploy/api/pkg/deployapi"
	deploymodels "a.yandex-team.ru/cloud/mdb/deploy/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/internal/encodingutil"
	jugglerapi "a.yandex-team.ru/cloud/mdb/internal/juggler"
	"a.yandex-team.ru/cloud/mdb/internal/requestid"
	"a.yandex-team.ru/cloud/mdb/internal/retry"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/katan/internal/katandb"
	"a.yandex-team.ru/cloud/mdb/katan/katan/internal/cluster/health"
	"a.yandex-team.ru/cloud/mdb/katan/katan/internal/cluster/health/flaps"
	"a.yandex-team.ru/cloud/mdb/katan/katan/internal/cluster/locker"
	mlocklocker "a.yandex-team.ru/cloud/mdb/katan/katan/internal/cluster/locker/mlock"
	"a.yandex-team.ru/cloud/mdb/katan/katan/internal/cluster/planner"
	"a.yandex-team.ru/cloud/mdb/katan/katan/internal/models"
	healthapi "a.yandex-team.ru/cloud/mdb/mdb-health/pkg/client"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
	"a.yandex-team.ru/cloud/mdb/mlock/pkg/mlockclient"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/slices"
)

var (
	ErrRolloutSkipped        = xerrors.NewSentinel("rollout on cluster was skipped")
	ErrRolloutCanceled       = xerrors.NewSentinel("rollout on cluster was canceled")
	ErrUnhealthyAfterRollout = xerrors.NewSentinel("clusters is unhealthy after rollout")
	ErrRolloutFailed         = xerrors.NewSentinel("rollout on cluster failed")
	errClusterLocked         = xerrors.NewSentinel("cluster locked")
)

var errorToRolloutState = map[*xerrors.Sentinel]katandb.ClusterRolloutState{
	ErrRolloutSkipped:        katandb.ClusterRolloutSkipped,
	ErrRolloutCanceled:       katandb.ClusterRolloutCancelled,
	ErrUnhealthyAfterRollout: katandb.ClusterRolloutFailed,
	ErrRolloutFailed:         katandb.ClusterRolloutFailed,
}

type Config struct {
	DeployTimeout         encodingutil.Duration `json:"deploy_timeout" yaml:"deploy_timeout"`
	WaitAfterDeploy       encodingutil.Duration `json:"wait_after_deploy" yaml:"wait_after_deploy"`
	HealthCheckInterval   encodingutil.Duration `json:"health_check_interval" yaml:"health_check_interval"`
	HealthCheckMaxWait    encodingutil.Duration `json:"health_check_max_wait_sec" yaml:"health_check_max_wait_sec"`
	HealthMeasureCount    int                   `json:"health_measure_count" yaml:"health_measure_count"`
	ShipmentCheckInterval encodingutil.Duration `json:"shipment_check_interval" yaml:"shipment_check_interval"`
	HealthMaxRetries      uint64                `json:"health_max_retries" yaml:"health_max_retries"`
	DeployMaxRetries      uint64                `json:"deploy_max_retries" yaml:"deploy_max_retries"`
	Locker                mlocklocker.Config    `json:"locker" yaml:"locker"`
	AllowedDeployGroups   []string              `json:"allowed_deploy_groups" yaml:"allowed_deploy_groups"`
}

func DefaultConfig() Config {
	return Config{
		DeployTimeout:         encodingutil.FromDuration(time.Hour),
		WaitAfterDeploy:       encodingutil.FromDuration(time.Minute),
		ShipmentCheckInterval: encodingutil.FromDuration(5 * time.Second),
		HealthMaxRetries:      10,
		DeployMaxRetries:      10,
		Locker:                mlocklocker.DefaultConfig(),
		HealthCheckInterval:   encodingutil.FromDuration(time.Second * 15),
		HealthCheckMaxWait:    encodingutil.FromDuration(time.Second * 120),
		HealthMeasureCount:    3,
	}
}

type RollOptions struct {
	ShipmentRetries int `json:"shipment_retries" yaml:"shipment_retries"`
}

func UnmarshalRollOptions(o string) (RollOptions, error) {
	ro := RollOptions{ShipmentRetries: 2}
	dec := json.NewDecoder(bytes.NewReader([]byte(o)))
	dec.DisallowUnknownFields()
	if err := dec.Decode(&ro); err != nil {
		return RollOptions{}, xerrors.Errorf("malformed rollout options: %w", err)
	}
	return ro, nil
}

type RollApis struct {
	Deploy  deployapi.Client
	Juggler jugglerapi.API
	Health  healthapi.MDBHealthClient
	MLock   mlockclient.Locker
}

func Roll(
	ctx context.Context,
	lg log.Logger,
	config Config,
	apis RollApis,
	onCluster func(models.ClusterChange),
	onShipment func(models.Shipment),
	rolloutID int64,
	cluster models.Cluster,
	commands []deploymodels.CommandDef,
	options RollOptions,
) error {
	rr := newRoller(lg, config, apis, onCluster, onShipment)
	return rr.Roll(ctx, rolloutID, cluster, commands, options)
}

// Cleanup remove garbage that could have been left by a fallen rollout.
// Currently it is only locks in MLock.
func Cleanup(
	ctx context.Context,
	lg log.Logger,
	config Config,
	apis RollApis,
	rolloutID int64,
	clusterID string,
) error {
	rr := newRoller(lg, config, apis, func(change models.ClusterChange) {}, func(shipment models.Shipment) {})
	return rr.Cleanup(ctx, rolloutID, clusterID)
}

func newRoller(
	lg log.Logger,
	config Config,
	apis RollApis,
	onCluster func(models.ClusterChange),
	onShipment func(models.Shipment),
) *roller {
	return &roller{
		onCluster:  onCluster,
		onShipment: onShipment,
		deploy:     apis.Deploy,
		health:     apis.Health,
		locker:     mlocklocker.New(config.Locker, apis.MLock, lg),
		lg:         lg,
		juggler:    apis.Juggler,
		config:     config,
		healthRetry: retry.New(retry.Config{
			MaxRetries: config.HealthMaxRetries,
		}),
		deployRetry: retry.New(retry.Config{
			MaxRetries: config.DeployMaxRetries,
		}),
	}
}

type roller struct {
	onCluster  func(models.ClusterChange)
	onShipment func(models.Shipment)

	apis        RollApis
	deploy      deployapi.Client
	health      healthapi.MDBHealthClient
	lg          log.Logger
	juggler     jugglerapi.API
	locker      locker.Locker
	config      Config
	healthRetry *retry.BackOff
	deployRetry *retry.BackOff
}

func wrapPermanentHealthError(err error) error {
	if xerrors.Is(err, healthapi.ErrInternalError) || semerr.IsUnavailable(err) {
		return err
	}
	return retry.Permanent(err)
}

func (rr *roller) waitForShipment(ctx context.Context, shipmentID int64) (string, error) {
	timer := time.NewTimer(rr.config.ShipmentCheckInterval.Duration)
	for {
		select {
		case <-ctx.Done():
			return fmt.Sprintf(
				"context is done (probably shipment %d not finished within '%s' timeout)",
				shipmentID,
				rr.config.DeployTimeout.Duration), nil
		case <-timer.C:
			var shipment deploymodels.Shipment
			if err := rr.deployRetry.RetryWithLog(
				ctx,
				func() error {
					sh, err := rr.deploy.GetShipment(ctx, deploymodels.ShipmentID(shipmentID))
					if err != nil {
						return err
					}
					shipment = sh
					return nil
				},
				"get deploy shipment",
				rr.lg,
			); err != nil {
				if xerrors.Is(err, deployapi.ErrNotFound) {
					rr.lg.Warnf("deploy say that shipment %d not found. Ignore that error", shipmentID)
					continue
				}
				return fmt.Sprintf("deploy error: %s", err), xerrors.Errorf("get %d shipment failed: %w", shipmentID, err)
			}
			rr.lg.Debugf("got deploy shipment %+v", shipment)
			if shipment.Status == deploymodels.ShipmentStatusInProgress {
				timer.Reset(rr.config.ShipmentCheckInterval.Duration)
				continue
			}

			if shipment.Status == deploymodels.ShipmentStatusDone {
				return "", nil
			}
			return fmt.Sprintf("shipment %d in unexpected status: %s", shipmentID, shipment.Status), nil
		}
	}
}

func (rr *roller) isHealthy(ctx context.Context, cluster models.Cluster) (string, error) {
	if health.ManagedByHealth(cluster.Tags) {
		verdict := flaps.Verdict{
			IsFlapping: true,
			Status:     healthapi.ClusterStatusUnknown,
		}
		healthObserver := flaps.NewFlapAwareHealthObserver(rr.config.HealthMeasureCount)
		startedAt := time.Now()
		for time.Since(startedAt) < rr.config.HealthCheckMaxWait.Duration {
			time.Sleep(rr.config.HealthCheckInterval.Duration)
			var clusterHealth healthapi.ClusterHealth
			if err := rr.healthRetry.RetryWithLog(
				ctx,
				func() error {
					h, err := rr.health.GetClusterHealth(ctx, cluster.ID)
					if err != nil {
						return wrapPermanentHealthError(err)
					}
					clusterHealth = h
					return nil
				},
				"get cluster health",
				rr.lg,
			); err != nil {
				return "", xerrors.Errorf("failed to get %q cluster health: %w", cluster.ID, err)
			}
			rr.lg.Debugf("cluster health is %+v", clusterHealth)
			healthObserver.LearnHealth(clusterHealth)
			verdict = healthObserver.Verdict()
			if verdict.IsFlapping {
				rr.lg.Infof("verdict is uncertain: cluster state is unstable: %+v", verdict)
				continue
			}
			if verdict.Status == healthapi.ClusterStatusAlive {
				break
			}
		}
		rr.lg.Infof("cluster verdict is %+v", verdict)
		if verdict.IsFlapping {
			return fmt.Sprintf("cluster is unstable since %s, details: %s", startedAt.Format(time.RFC3339), verdict.Interpretation), nil
		}
		if verdict.Status != healthapi.ClusterStatusAlive {
			return fmt.Sprintf("cluster is %q", verdict.Status), nil
		}
		return "", nil
	}

	for fqdn := range cluster.Hosts {
		for _, service := range health.ServicesByTags(cluster.Tags) {
			// TODO: backoff juggler calls
			events, err := rr.juggler.RawEvents(ctx, []string{fqdn}, []string{service})
			if err != nil {
				return "", xerrors.Errorf("failed to get juggler raw events: %w", err)
			}
			last := health.LastEvent(events)
			rr.lg.Debugf("last %q event on %q is: %+v", service, fqdn, last)
			if last.Status == "CRIT" {
				rr.lg.Infof("host %q has CRIT for service %q", fqdn, service)
				return last.String(), nil
			}
		}
	}
	return "", nil
}

func (rr *roller) createShipment(ctx context.Context, fqdns []string, commands []deploymodels.CommandDef) (int64, error) {
	var zeroTimeout encodingutil.Duration
	defaultTimeout := (time.Duration(rr.config.DeployTimeout.Nanoseconds()/int64(len(commands))) * time.Nanosecond).Round(time.Second)

	// Current deploy-api treat missing command timeout as timeout=0s.
	// And our shipments commands immediately fails into TIMEOUT - MDB-7382
	for i, cmd := range commands {
		if cmd.Timeout == zeroTimeout {
			rr.lg.Infof("command %+v has zero timeout. Set that command timeout to: %s ", cmd, defaultTimeout)
			cmd.Timeout = encodingutil.FromDuration(defaultTimeout)
			commands[i] = cmd
		}
	}
	// Use Parallel: 1 due a race in deploy - MDB-9055
	requestID := requestid.New()
	shipment, err := rr.deploy.CreateShipment(
		requestid.WithRequestID(ctx, requestID), fqdns, commands, 1, 1, rr.config.DeployTimeout.Duration,
	)
	if err != nil {
		return 0, xerrors.Errorf("fail to create shipment on %q hosts (Request-ID: %s): %w", fqdns, requestID, err)
	}
	rr.lg.Infof("successfully create shipment %d on %+v (Request-ID: %s)", shipment.ID, fqdns, requestID)
	return int64(shipment.ID), nil
}

// updateHosts - create shipment and wait until it finished
func (rr *roller) updateHosts(ctx context.Context, clusterID string, updateOnStep []string, commands []deploymodels.CommandDef) error {
	// We don't want to wait more then DeployTimeout
	// + 30% gap (cause here we prefer Shipment.TIMEOUT to context.Done)
	updateHostsTimeout := rr.config.DeployTimeout.Duration
	updateHostsTimeout += updateHostsTimeout * 30 / 100
	upCtx, cancel := context.WithTimeout(ctx, updateHostsTimeout)
	defer cancel()

	shipmentID, err := rr.createShipment(upCtx, updateOnStep, commands)
	if err != nil {
		return err
	}

	rr.onShipment(models.Shipment{
		FQDNs:      updateOnStep,
		ClusterID:  clusterID,
		ShipmentID: shipmentID,
	})

	failReason, err := rr.waitForShipment(upCtx, shipmentID)
	if err != nil {
		return err
	}
	if len(failReason) > 0 {
		rr.lg.Warnf("shipment %d on %q hosts failed: %s", shipmentID, updateOnStep, failReason)
		return ErrRolloutFailed.Wrap(xerrors.New(failReason))
	}

	return nil
}

// updateHostsWithRetries - create shipment and wait until it finished and do retries if it fails
func (rr *roller) updateHostsWithRetries(ctx context.Context, clusterID string, updateOnStep []string, commands []deploymodels.CommandDef, retries int) error {
	// The deployment can fail on different hosts when we create a shipment for several machines,
	// but multiplying retries onto hosts count can be a bad idea cause we increase the lock period.
	for {
		err := rr.updateHosts(ctx, clusterID, updateOnStep, commands)
		if err != nil {
			retries--
			// retries >= 0 cause we decrement retries before use them
			if xerrors.Is(err, ErrRolloutFailed) && retries >= 0 {
				rr.lg.Infof("retrying %q on %q cause: %s (have %d retries)", commands, updateOnStep, err, retries)
				continue
			}
		}
		return err
	}
}

func (rr *roller) getRolloutPlan(ctx context.Context, cluster models.Cluster) ([][]string, error) {
	var hostsHealth []types.HostHealth
	if planner.NeedHostsHealth(cluster) {
		if err := rr.healthRetry.RetryWithLog(
			ctx,
			func() error {
				hh, err := rr.health.GetHostsHealth(ctx, cluster.FQDNs())
				if err != nil {
					return wrapPermanentHealthError(err)
				}
				hostsHealth = hh
				return nil
			},
			"get hosts health",
			rr.lg,
		); err != nil {
			return nil, ErrRolloutSkipped.Wrap(xerrors.Errorf("get %q hosts health: %w", cluster.ID, err))
		}
		if len(hostsHealth) != len(cluster.Hosts) {
			return nil, ErrRolloutSkipped.Wrap(xerrors.Errorf("got %d hosts from mdb-health, when %d expected", len(hostsHealth), len(cluster.Hosts)))
		}
		rr.lg.Debugf("Health services %+v", hostsHealth)
	}
	clusterWithServices, err := planner.ComposeCluster(cluster, hostsHealth)
	if err != nil {
		return nil, err
	}
	rolloutPlan, err := GetPlanner(cluster)(clusterWithServices)
	if err != nil {
		// treat planner errors as skipped rollout,
		// cause we don't change anything at that point
		return nil, ErrRolloutSkipped.Wrap(xerrors.Errorf("failed to plan rollout on cluster: %w", err))
	}
	return rolloutPlan, nil
}

func (rr *roller) allNodesInValidGroups(ctx context.Context, cluster models.Cluster) error {
	if len(rr.config.AllowedDeployGroups) == 0 {
		rr.lg.Warn("AllowedDeployGroups is unset. Treat any deploy group as allowed")
		return nil
	}
	for fqdn := range cluster.Hosts {
		var minion deploymodels.Minion
		if err := rr.deployRetry.RetryWithLog(
			ctx,
			func() error {
				m, err := rr.deploy.GetMinion(ctx, fqdn)
				if err != nil {
					return err
				}
				minion = m
				return nil
			},
			"get deploy minion",
			rr.lg,
		); err != nil {
			return xerrors.Errorf("get %q minion info: %w", fqdn, err)
		}

		if !slices.ContainsString(rr.config.AllowedDeployGroups, minion.Group) {
			return xerrors.Errorf("%q in %q deploy group", fqdn, minion.Group)
		}
	}
	return nil
}

func (rr *roller) lockID(rolloutID int64, clusterID string) string {
	return fmt.Sprintf("katan-%d-rollout-on-%s", rolloutID, clusterID)
}

func (rr *roller) acquireAndUpdateHosts(ctx context.Context, rolloutID int64, rollOnStep []string, cluster models.Cluster, commands []deploymodels.CommandDef, firstInCluster bool, options RollOptions) (err error) {
	lockID := rr.lockID(rolloutID, cluster.ID)
	if err := rr.locker.Acquire(ctx, lockID, rollOnStep, fmt.Sprintf("rollout-%d", rolloutID)); err != nil {
		return errClusterLocked.Wrap(err)
	}
	defer func() {
		if r := recover(); r != nil {
			_ = rr.locker.Release(ctx, lockID)
			panic(r)
		}
		releaseErr := rr.locker.Release(ctx, lockID)
		if releaseErr != nil {
			rr.lg.Error("mark cluster as cancelled, cause failed to release lock", log.Error(err))
			if err == nil {
				// should stop rollout, due to lost lost lock
				err = ErrRolloutCanceled.Wrap(err)
			}
		}
	}()
	if unhealthyReason, err := rr.isHealthy(ctx, cluster); err != nil {
		return ErrRolloutCanceled.Wrap(err)
	} else {
		if len(unhealthyReason) > 0 {
			if firstInCluster {
				return ErrRolloutSkipped.Wrap(xerrors.New(unhealthyReason))
			}
			return ErrUnhealthyAfterRollout.Wrap(xerrors.New(unhealthyReason))
		}
	}

	err = rr.updateHostsWithRetries(ctx, cluster.ID, rollOnStep, commands, options.ShipmentRetries)
	if err != nil {
		return err
	}

	return nil
}

func (rr *roller) Roll(ctx context.Context, rolloutID int64, cluster models.Cluster, commands []deploymodels.CommandDef, options RollOptions) error {
	if err := rr.allNodesInValidGroups(ctx, cluster); err != nil {
		rr.onCluster(models.ClusterChange{
			ClusterID: cluster.ID,
			State:     katandb.ClusterRolloutSkipped,
			Comment:   err.Error(),
		})
		return ErrRolloutSkipped.Wrap(err)
	}
	unhealthyReason, err := rr.isHealthy(ctx, cluster)
	if err != nil {
		return err
	}
	if len(unhealthyReason) > 0 {
		rr.onCluster(models.ClusterChange{
			ClusterID: cluster.ID,
			State:     katandb.ClusterRolloutSkipped,
			Comment:   unhealthyReason,
		})
		return ErrRolloutSkipped.Wrap(xerrors.New(unhealthyReason))
	}

	rolloutPlan, err := rr.getRolloutPlan(ctx, cluster)
	if err != nil {
		if xerrors.Is(err, ErrRolloutSkipped) {
			rr.onCluster(models.ClusterChange{
				ClusterID: cluster.ID,
				State:     katandb.ClusterRolloutSkipped,
				Comment:   err.Error(),
			})
		}
		return err
	}

	rr.lg.Infof("rollout plan is %+v", rolloutPlan)

	rr.onCluster(models.ClusterChange{
		ClusterID: cluster.ID,
		State:     katandb.ClusterRolloutRunning,
	})

	for stepNo, rollOnStep := range rolloutPlan {
		err := rr.acquireAndUpdateHosts(ctx, rolloutID, rollOnStep, cluster, commands, stepNo == 0, options)
		if err != nil {
			if xerrors.Is(err, errClusterLocked) {
				if stepNo == 0 {
					rr.onCluster(models.ClusterChange{
						ClusterID: cluster.ID,
						State:     katandb.ClusterRolloutSkipped,
						Comment:   err.Error(),
					})
					return ErrRolloutSkipped.Wrap(err)
				}
				rr.onCluster(models.ClusterChange{
					ClusterID: cluster.ID,
					State:     katandb.ClusterRolloutCancelled,
					Comment:   err.Error(),
				})
				return ErrRolloutCanceled.Wrap(err)
			}
			for errKind, rollState := range errorToRolloutState {
				if xerrors.Is(err, errKind) {
					rr.onCluster(models.ClusterChange{
						ClusterID: cluster.ID,
						State:     rollState,
						Comment:   err.Error(),
					})
					break
				}
			}
			return err
		}
		time.Sleep(rr.config.WaitAfterDeploy.Duration)
	}
	unhealthyReason, err = rr.isHealthy(ctx, cluster)
	if err != nil {
		return err
	}
	if len(unhealthyReason) > 0 {
		rr.onCluster(models.ClusterChange{
			ClusterID: cluster.ID,
			State:     katandb.ClusterRolloutFailed,
			Comment:   fmt.Sprintf("rollout break cluster: %s", unhealthyReason),
		})
		return ErrUnhealthyAfterRollout.Wrap(xerrors.New(unhealthyReason))
	}

	rr.onCluster(models.ClusterChange{
		ClusterID: cluster.ID,
		State:     katandb.ClusterRolloutSucceeded,
	})
	return nil
}

func (rr *roller) Cleanup(ctx context.Context, rolloutID int64, clusterID string) error {
	return rr.locker.Release(ctx, rr.lockID(rolloutID, clusterID))
}
