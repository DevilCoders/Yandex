package afpillars

import (
	"encoding/json"

	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/library/go/core/xerrors"
)

const (
	DefaultSegmentBytes = int64(1024 * 1024 * 1024)
)

type Cluster struct {
	Data   ClusterData `json:"data"`
	Values HelmValues  `json:"values"`
}

type ClusterData struct {
	S3Bucket   string         `json:"s3_bucket"`
	Kubernetes KubernetesData `json:"kubernetes"`
}

type KubernetesData struct {
	// Per-airflow-cluster info
	ClusterID   string   `json:"cluster_id"`
	NodeGroupID string   `json:"node_group_id"`
	SubnetIDs   []string `json:"node_group_subnet_ids"`
	// Per-k8s-cluster info
	ClusterServiceAccountID    string `json:"cluster_service_account_id,omitempty"`
	NodeServiceAccountID       string `json:"node_service_account_id,omitempty"`
	MasterNetworkID            string `json:"master_network_id,omitempty"`
	MasterZoneID               string `json:"master_zone_id,omitempty"`
	NodeServiceSecurityGroupID string `json:"node_service_security_group_id"`
}

type HelmValues struct {
	Config    map[string]string `json:"config"`
	Version   string            `json:"airflowVersion"`
	Webserver WebserverValues   `json:"webserver"`
	Scheduler SchedulerValues   `json:"scheduler"`
	Triggerer *TriggererValues  `json:"triggerer"`
	Worker    WorkerValues      `json:"workers"`
}

type WebserverValues struct {
	ComponentValues
	Replicas int64 `json:"replicas"`
}

type SchedulerValues struct {
	ComponentValues
	Replicas int64 `json:"replicas"`
}

type TriggererValues struct {
	ComponentValues
	Replicas int64 `json:"replicas"`
}

type WorkerValues struct {
	ComponentValues
	KEDA KEDAConfig `json:"keda"`
}

type KEDAConfig struct {
	MinReplicaCount    int64 `json:"minReplicaCount"`
	MaxReplicaCount    int64 `json:"maxReplicaCount"`
	CooldownPeriodSec  int64 `json:"cooldownPeriod"`
	PollingIntervalSec int64 `json:"pollingInterval"`
}

type ComponentValues struct {
	Resources PodResources `json:"resources"`
}

type PodResources struct {
	Limits   ResourcesDesc `json:"limits"`
	Requests ResourcesDesc `json:"requests"`
}

type ResourcesDesc struct {
	VCPUCount   int64 `json:"cpu"`
	MemoryBytes int64 `json:"memory"`
}

func NewCluster() *Cluster {
	var pillar Cluster
	pillar.Values.Config = make(map[string]string)
	return &pillar
}

func (c *Cluster) MarshalPillar() (json.RawMessage, error) {
	raw, err := json.Marshal(c)
	if err != nil {
		return nil, xerrors.Errorf("failed to marshal airflow cluster pillar: %w", err)
	}
	return raw, err
}

func (c *Cluster) UnmarshalPillar(raw json.RawMessage) error {
	if err := json.Unmarshal(raw, c); err != nil {
		return xerrors.Errorf("failed to unmarshal airflow cluster pillar: %w", err)
	}
	if c.Values.Config == nil {
		c.Values.Config = make(map[string]string)
	}
	return nil
}

func (c *ResourcesDesc) Validate() error {
	if c.VCPUCount < 1 {
		return semerr.InvalidInput("CPU resource must be positive number")
	}
	if c.MemoryBytes < 1 {
		return semerr.InvalidInput("Memory resource must be positive number")
	}
	return nil
}

func (c *ComponentValues) Validate() error {
	if err := c.Resources.Limits.Validate(); err != nil {
		return err
	}
	if err := c.Resources.Requests.Validate(); err != nil {
		return err
	}
	if c.Resources.Requests.VCPUCount > c.Resources.Limits.VCPUCount {
		return semerr.InvalidInput("CPU request is bigger than limit")
	}
	if c.Resources.Requests.MemoryBytes > c.Resources.Limits.MemoryBytes {
		return semerr.InvalidInput("Memory request is bigger than limit")
	}
	return nil
}

func (c *Cluster) Validate() error {
	if err := c.Values.Webserver.Validate(); err != nil {
		return err
	}
	if err := c.Values.Scheduler.Validate(); err != nil {
		return err
	}
	if c.Values.Triggerer != nil {
		if err := c.Values.Triggerer.Validate(); err != nil {
			return err
		}
	}
	if err := c.Values.Worker.Validate(); err != nil {
		return err
	}
	return nil
}
