package afmodels

import (
	"time"
)

type ClusterConfigSpec struct {
	Version string
	Airflow AirflowConfig
	// Configuration for workloads
	Webserver WebserverConfig
	Scheduler SchedulerConfig
	Triggerer *TriggererConfig
	Worker    WorkerConfig
}

func (cs ClusterConfigSpec) Validate() error {
	// TODO
	return nil
}

type AirflowConfig struct {
	Config map[string]string
}

type WebserverConfig struct {
	Count     int64
	Resources Resources
}

type SchedulerConfig struct {
	Count     int64
	Resources Resources
}

type TriggererConfig struct {
	Count     int64
	Resources Resources
}

type WorkerConfig struct {
	MinCount        int64
	MaxCount        int64
	Resources       Resources
	PollingInterval time.Duration
	CooldownPeriod  time.Duration
}

type Resources struct {
	VCPUCount   int64
	MemoryBytes int64
}

type NetworkConfig struct {
	SecurityGroupIDs []string
	SubnetIDs        []string
}
