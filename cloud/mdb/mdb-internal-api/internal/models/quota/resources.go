package quota

import (
	"fmt"
	"math"
	"strings"

	"github.com/dustin/go-humanize"

	"a.yandex-team.ru/cloud/mdb/internal/semerr"
)

type Resources struct {
	CPU      float64 `json:"cpu" yaml:"cpu"`             // Cores count
	GPU      int64   `json:"gpu" yaml:"gpu"`             // Cores count
	Memory   int64   `json:"memory" yaml:"memory"`       // Bytes
	SSDSpace int64   `json:"ssd_space" yaml:"ssd_space"` // Bytes
	HDDSpace int64   `json:"hdd_space" yaml:"hdd_space"` // Bytes
	Clusters int64   `json:"clusters" yaml:"clusters"`   // Count
}

type ResourceType int

const (
	ResourceTypeUnknown ResourceType = iota
	ResourceTypeCPU
	ResourceTypeGPU
	ResourceTypeMemory
	ResourceTypeSSDSpace
	ResourceTypeHDDSpace
	ResourceTypeClusters
)

func (r Resources) Add(add Resources) Resources {
	return Resources{
		CPU:      r.CPU + add.CPU,
		GPU:      r.GPU + add.GPU,
		Memory:   r.Memory + add.Memory,
		SSDSpace: r.SSDSpace + add.SSDSpace,
		HDDSpace: r.HDDSpace + add.HDDSpace,
		Clusters: r.Clusters + add.Clusters,
	}
}

func (r Resources) Sub(sub Resources) Resources {
	return Resources{
		CPU:      r.CPU - sub.CPU,
		GPU:      r.GPU - sub.GPU,
		Memory:   r.Memory - sub.Memory,
		SSDSpace: r.SSDSpace - sub.SSDSpace,
		HDDSpace: r.HDDSpace - sub.HDDSpace,
		Clusters: r.Clusters - sub.Clusters,
	}
}

func (r Resources) Mul(mul int64) Resources {
	var res Resources
	for i := int64(0); i < mul; i++ {
		res = res.Add(r)
	}

	return res
}

type Consumption struct {
	// Used quota
	used Resources

	// Difference in quota
	diff Resources

	// Current quota
	quota Resources
}

func NewConsumption(quota, used Resources) *Consumption {
	return &Consumption{used: used, quota: quota}
}

func (c *Consumption) Changed() bool {
	return c.diff != Resources{}
}

func (c *Consumption) Diff() Resources {
	return c.diff
}

func (c *Consumption) RequestChange(r Resources) {
	c.diff = c.diff.Add(r)
}

func (c *Consumption) Validate() error {
	target := c.used.Add(c.diff)

	var failed []string
	var details []Violation
	if target.CPU > c.quota.CPU {
		violation := Violation{Resource: ResourceTypeCPU, Usage: c.used.CPU, Limit: int64(math.Ceil(c.quota.CPU)), Required: int64(math.Ceil(target.CPU))}
		failed = append(failed, fmt.Sprintf("cpu: %d", int64(math.Ceil(target.CPU-c.quota.CPU))))
		details = append(details, violation)
	}
	if target.GPU > c.quota.GPU {
		violation := Violation{Resource: ResourceTypeGPU, Usage: float64(c.used.GPU), Limit: c.quota.GPU, Required: target.GPU}
		failed = append(failed, fmt.Sprintf("gpu cards: %d", target.GPU-c.quota.GPU))
		details = append(details, violation)
	}
	if target.Memory > c.quota.Memory {
		violation := Violation{Resource: ResourceTypeMemory, Usage: float64(c.used.Memory), Limit: c.quota.Memory, Required: target.Memory}
		failed = append(failed, fmt.Sprintf("memory: %s", humanize.IBytes(uint64(target.Memory-c.quota.Memory))))
		details = append(details, violation)
	}
	if target.SSDSpace > c.quota.SSDSpace {
		violation := Violation{Resource: ResourceTypeSSDSpace, Usage: float64(c.used.SSDSpace), Limit: c.quota.SSDSpace, Required: target.SSDSpace}
		failed = append(failed, fmt.Sprintf("ssd space: %s", humanize.IBytes(uint64(target.SSDSpace-c.quota.SSDSpace))))
		details = append(details, violation)
	}
	if target.HDDSpace > c.quota.HDDSpace {
		violation := Violation{Resource: ResourceTypeHDDSpace, Usage: float64(c.used.HDDSpace), Limit: c.quota.HDDSpace, Required: target.HDDSpace}
		failed = append(failed, fmt.Sprintf("hdd space: %s", humanize.IBytes(uint64(target.HDDSpace-c.quota.HDDSpace))))
		details = append(details, violation)
	}
	if target.Clusters > c.quota.Clusters {
		violation := Violation{Resource: ResourceTypeClusters, Usage: float64(c.used.Clusters), Limit: c.quota.Clusters, Required: target.Clusters}
		failed = append(failed, fmt.Sprintf("clusters: %d", target.Clusters-c.quota.Clusters))
		details = append(details, violation)
	}

	if len(failed) != 0 {
		err := semerr.FailedPreconditionf("Quota limits exceeded, not enough %q. Please contact support to request extra resource quota.", strings.Join(failed, ", "))
		err.Details = &Violations{Violations: details}
		return err
	}

	return nil
}

type Violation struct {
	Resource ResourceType
	Usage    float64
	Limit    int64
	Required int64
}

type Violations struct {
	CloudExtID string
	Violations []Violation
}

func (v *Violations) SetCloudExtID(id string) {
	v.CloudExtID = id
}
