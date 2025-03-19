package models

import (
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/encodingutil"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
)

// WorkerConfig describes worker task configuration.
type WorkerConfig struct {
	// Required. Operation type.
	OperationType string `json:"operation_type" yaml:"operation_type"`
	// Required. Task type.
	TaskType string `json:"task_type" yaml:"task_type"`
	// Task arguments. Either TaskArgs or TaskArgsQuery must be specified.
	TaskArgs map[string]interface{} `json:"task_args" yaml:"task_args"`
	// SQL query to construct task arguments. Either TaskArgs or TaskArgsQuery must be specified.
	TaskArgsQuery string `json:"task_args_query" yaml:"task_args_query"`
	// Task timeout. Either Timeout or TimeoutQuery must be specified.
	Timeout encodingutil.Duration `json:"timeout" yaml:"timeout"`
	// SQL query to calculate taks timeout. Either Timeout or TimeoutQuery must be specified.
	TimeoutQuery string `json:"timeout_query" yaml:"timeout_query"`
}

type CMSStepClusterSelection struct {
	EndsWith string `yaml:"endswith"`
}

type CMSClusterSelection struct {
	ClusterType string                  `yaml:"cluster_type"`
	Steps       CMSStepClusterSelection `yaml:"steps"`
	Duration    time.Duration           `yaml:"duration"`
}

type ClusterSelection struct {
	DB  string                `json:"db" yaml:"db"`
	CMS []CMSClusterSelection `json:"cms" yaml:"cms"`
}

func (c *ClusterSelection) Empty() bool {
	return c.DB == "" && len(c.CMS) == 0
}

type MaintenanceTaskConfig struct {
	ID                string           `json:"-" yaml:"-"`
	Info              string           `json:"info" yaml:"info"`
	ClustersSelection ClusterSelection `json:"clusters_selection" yaml:"clusters_selection"`
	PillarChange      string           `json:"pillar_change" yaml:"pillar_change"`
	Worker            WorkerConfig     `json:"worker" yaml:"worker"`
	Repeatable        bool             `json:"repeatable" yaml:"repeatable"`
	// Disable environment ordering (DEV->QA->PROD->COMPUTE-PROD)
	EnvOrderDisabled bool `json:"env_order_disabled" yaml:"env_order_disabled"`
	Disabled         bool `json:"disabled" yaml:"disabled"`
	MinDays          int  `json:"min_days" yaml:"min_days"`
	SupportsOffline  bool `json:"supports_offline" yaml:"supports_offline"`
	MaxDays          int  `json:"max_delay_days" yaml:"max_delay_days"`
	Priority         int  `json:"priority" yaml:"priority"`
	HoursDelay       int  `json:"hours_delay" yaml:"hours_delay"`
}

func DefaultTaskConfig() MaintenanceTaskConfig {
	return MaintenanceTaskConfig{
		Worker: WorkerConfig{
			Timeout: encodingutil.FromDuration(time.Hour),
		},
		MinDays: 7,
		MaxDays: 21,
	}
}

type ClusterRevs struct {
	Rev     int64
	NextRev int64
}

type MaintenanceSettings struct {
	Valid            bool
	Weekday          time.Weekday
	UpperBoundMWHour int
}

func (mw *MaintenanceSettings) Set(weekday time.Weekday, hour int) {
	mw.Valid = true
	mw.Weekday = weekday
	mw.UpperBoundMWHour = hour
}

func NewMaintenanceSettings(weekday time.Weekday, hour int) MaintenanceSettings {
	return MaintenanceSettings{
		Valid:            true,
		Weekday:          weekday,
		UpperBoundMWHour: hour,
	}
}

type Cluster struct {
	ID                       string
	FolderID                 int64
	FolderExtID              string
	CloudExtID               string
	ClusterType              string
	ClusterName              string
	Settings                 MaintenanceSettings
	Env                      Env
	HasActiveTasks           bool
	TargetMaintenanceVtypeID string
}

type MaintenanceTaskStatus string

const (
	MaintenanceTaskPlanned   MaintenanceTaskStatus = "PLANNED"
	MaintenanceTaskCanceled  MaintenanceTaskStatus = "CANCELED"
	MaintenanceTaskCompleted MaintenanceTaskStatus = "COMPLETED"
	MaintenanceTaskRejected  MaintenanceTaskStatus = "REJECTED"
	MaintenanceTaskFailed    MaintenanceTaskStatus = "FAILED"
)

func (mts MaintenanceTaskStatus) IsRescheduable() bool {
	switch mts {
	case MaintenanceTaskRejected, MaintenanceTaskCanceled:
		return true
	default:
		return false
	}
}

func (mts MaintenanceTaskStatus) IsCancellable() bool {
	switch mts {
	case MaintenanceTaskPlanned, MaintenanceTaskCanceled, MaintenanceTaskRejected:
		return true
	default:
		return false
	}
}

func (mts MaintenanceTaskStatus) IsRepeatable() bool {
	switch mts {
	case MaintenanceTaskCompleted:
		return true
	default:
		return false
	}
}

type MaintenanceTask struct {
	ClusterID string
	ConfigID  string
	TaskID    optional.String
	CreateTS  time.Time
	PlanTS    optional.Time
	MaxDelay  time.Time
	Status    MaintenanceTaskStatus
}

type PlanMaintenanceTaskRequest struct {
	MaxDelay      time.Time
	ID            string
	ClusterID     string
	ConfigID      string
	FolderID      int64
	OperationType string
	TaskType      string
	TaskArgs      string
	Version       int
	Metadata      string
	UserID        string
	Rev           int64
	TargetRev     int64
	PlanTS        time.Time
	CreateTS      time.Time
	Timeout       time.Duration
	Info          string
}

type Env int

const (
	EnvDev Env = iota
	EnvQA
	EnvProd
	EnvComputeProd
)

type ClustersByEnv []Cluster

func (s ClustersByEnv) Len() int {
	return len(s)
}

func (s ClustersByEnv) Less(i, j int) bool {
	return s[i].Env < s[j].Env
}

func (s ClustersByEnv) Swap(i, j int) {
	s[i], s[j] = s[j], s[i]
}
