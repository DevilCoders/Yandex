package katandb

import (
	"context"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/library/go/core/xerrors"
)

//go:generate ../../../scripts/mockgen.sh KatanDB

type Cluster struct {
	ID   string
	Tags string
}

// Host is host definition
type Host struct {
	ClusterID string
	FQDN      string
	Tags      string
}

type ClusterRolloutState int

const (
	ClusterRolloutUnknown ClusterRolloutState = iota
	ClusterRolloutPending
	ClusterRolloutRunning
	ClusterRolloutSucceeded
	ClusterRolloutCancelled
	ClusterRolloutSkipped
	ClusterRolloutFailed
)

func (hs ClusterRolloutState) String() string {
	switch hs {
	case ClusterRolloutPending:
		return "pending"
	case ClusterRolloutRunning:
		return "running"
	case ClusterRolloutSucceeded:
		return "succeeded"
	case ClusterRolloutCancelled:
		return "cancelled"
	case ClusterRolloutSkipped:
		return "skipped"
	case ClusterRolloutFailed:
		return "failed"
	}
	return "unknown"
}

// ClusterRollout ...
type ClusterRollout struct {
	ClusterID string
	RolloutID int64
	State     ClusterRolloutState
	Comment   optional.String
	UpdatedAt time.Time
}

// Rollout ...
type Rollout struct {
	ID         int64
	Commands   string
	CreateAt   time.Time
	Parallel   int
	Options    string
	FinishedAt optional.Time
	CreatedBy  string
	ScheduleID optional.Int64
	Comment    optional.String
}

type ScheduleState int

const (
	ScheduleStateUnknown ScheduleState = iota
	ScheduleStateActive
	ScheduleStateStopped
	ScheduleStateBroken
)

func (ss ScheduleState) String() string {
	switch ss {
	case ScheduleStateActive:
		return "active"
	case ScheduleStateStopped:
		return "stopped"
	case ScheduleStateBroken:
		return "broken"
	}
	return "unknown"
}

// Schedule ...
type Schedule struct {
	ID                int64
	MatchTags         string
	Commands          string
	State             ScheduleState
	Age               time.Duration
	StillAge          time.Duration
	MaxSize           int64
	Parallel          int32
	Options           string
	DependsOn         []int64
	EditedAt          time.Time
	EditedBy          optional.String
	ExaminedRolloutID optional.Int64
	Name              string
	Namespace         string
}

type ScheduleFail struct {
	RolloutID int64
	ClusterID string
}

type RolloutDates struct {
	ID                   int64
	CreatedAt            time.Time
	LastUpdatedAt        optional.Time
	LastUpdatedAtCluster optional.String
}

type RolloutShipment struct {
	ShipmentID int64
	FQDNs      []string
}

var (
	ErrNotAvailable = xerrors.NewSentinel("Not available")
	ErrNoDataFound  = xerrors.NewSentinel("No data found")
)

// KatanDB API
type KatanDB interface {
	// ClustersByTagsQuery get clusters that match given query
	ClustersByTagsQuery(ctx context.Context, query string) ([]Cluster, error)
	// ClusterHosts get hosts that belongs to given cluster
	ClusterHosts(ctx context.Context, id string) ([]Host, error)
	// AutoUpdatedClustersIDsByQuery get cluster ids that is auto updated and match given query
	AutoUpdatedClustersIDsByQuery(ctx context.Context, query string) ([]string, error)
	// AutoUpdatedClustersBySchedule get cluster ids that:
	// - match given tags
	// - don't have successful rollouts for the specified age
	// - don't have that schedule attempts in stillAge
	// - imported not early then importCooldown
	AutoUpdatedClustersBySchedule(ctx context.Context, query string, scheduleID int64, age, stillAge, importCooldown time.Duration, limit int64) ([]string, error)

	// AddHost add host
	AddHost(ctx context.Context, host Host) error
	// UpdateHostTags update host tags
	UpdateHostTags(ctx context.Context, fqdn, tags string) error
	// AddCluster add cluster
	AddCluster(ctx context.Context, cluster Cluster) error
	// UpdateClusterTags update cluster tags
	UpdateClusterTags(ctx context.Context, id, tags string) error
	// DeleteHosts delete hosts
	DeleteHosts(ctx context.Context, fqdns []string) error
	// DeleteCluster delete cluster with its hosts
	DeleteCluster(ctx context.Context, id string) error

	// AddRollout add rollout for given clusters
	AddRollout(ctx context.Context, commands, options, createdBy string, parallel int32, scheduleID optional.Int64, clusterIDs []string) (Rollout, error)
	// Rollout return rollout by id
	Rollout(ctx context.Context, id int64) (Rollout, error)
	// OldestRunningRollout return oldest running rollout
	OldestRunningRollout(ctx context.Context) (RolloutDates, error)
	// StartPendingRollout start one rollout that ready to start (have finished dependencies)
	StartPendingRollout(ctx context.Context, rolledBy string) (Rollout, error)
	// UnfinishedRollouts returns unfinished rollouts rolled by given katan
	UnfinishedRollouts(ctx context.Context, rolledBy string) ([]Rollout, error)
	// LastRolloutsBySchedule return last limit rollouts started by given schedule
	LastRolloutsBySchedule(ctx context.Context, scheduleID int64, since time.Time, limit int) ([]Rollout, error)
	// RolloutClusters returns clusters in given rollout
	RolloutClusters(ctx context.Context, rolloutID int64) ([]Cluster, error)
	// RolloutShipmentsByCluster return shipments that started in given rollout at given cluster
	RolloutShipmentsByCluster(ctx context.Context, rolloutID int64, clusterID string) ([]RolloutShipment, error)
	// RolloutClustersHosts return hosts in given rollout
	RolloutClustersHosts(ctx context.Context, rolloutID int64) ([]Host, error)
	// ClusterRollouts returns cluster rollouts by given rolloutID
	ClusterRollouts(ctx context.Context, rolloutID int64) ([]ClusterRollout, error)
	// FinishRollout finish it
	FinishRollout(ctx context.Context, rolloutID int64, comment optional.String) error
	// AddRolloutShipment add shipmentID to given rollout
	AddRolloutShipment(ctx context.Context, rolloutID int64, FQDNs []string, shipmentID int64) error
	// MarkClusterRollout update state for given cluster rollout
	MarkClusterRollout(ctx context.Context, rolloutID int64, clusterID string, state ClusterRolloutState, comment string) error
	// TouchClusterRollout set updated_at for cluster rollout to now
	TouchClusterRollout(ctx context.Context, rolloutID int64, clusterID string) error
	// Schedules return all schedules
	Schedules(ctx context.Context) ([]Schedule, error)
	//// ClusterRolloutsFailedInSchedule return cluster rollouts that failed in given schedule
	ClusterRolloutsFailedInSchedule(ctx context.Context, scheduleID int64) ([]ClusterRollout, error)
	// MarkSchedule change schedule state
	MarkSchedule(ctx context.Context, scheduleID int64, state ScheduleState, examinedRolloutID int64, fails []ScheduleFail) error
	// Close cluster
	Close() error
	// IsReady ...
	IsReady(ctx context.Context) error
}
