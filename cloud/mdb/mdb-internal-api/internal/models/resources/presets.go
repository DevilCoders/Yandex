package resources

import (
	"github.com/gofrs/uuid"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/environment"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/quota"
)

// TODO: its an array underneath, might be not very wise to copy it... On the other hand that array equals
//  4 x int64 bin struct below so... May be we use pointer for the entire struct.
type PresetID uuid.UUID

type Preset struct {
	ID               PresetID
	ExtID            string
	CPUGuarantee     float64
	CPULimit         float64
	CPUFraction      float64
	IOCoresLimit     int64
	GPULimit         int64
	MemoryGuarantee  int64
	MemoryLimit      int64
	NetworkGuarantee int64
	NetworkLimit     int64
	IOLimit          int64
	Description      string
	VType            environment.VType
	Type             string
	Generation       int64
	PlatformID       string
}

func (p Preset) Resources() quota.Resources {
	return quota.Resources{
		CPU:    p.CPUGuarantee,
		GPU:    p.GPULimit,
		Memory: p.MemoryGuarantee,
	}
}

func (p Preset) IsUpscaleOf(old Preset) bool {
	return p.CPUGuarantee-old.CPUGuarantee > 0.1 ||
		p.CPULimit-old.CPULimit > 0.1 ||
		p.MemoryGuarantee > old.MemoryGuarantee ||
		p.MemoryLimit > old.MemoryLimit
}

func (p Preset) IsDownscaleOf(old Preset) bool {
	return p.CPUGuarantee-old.CPUGuarantee < -0.1 ||
		p.CPULimit-old.CPULimit < -0.1 ||
		p.MemoryGuarantee < old.MemoryGuarantee ||
		p.MemoryLimit < old.MemoryLimit
}

type DefaultPreset struct {
	DiskTypeExtID string
	DiskSizeRange optional.IntervalInt64
	DiskSizes     []int64
	ExtID         string
}
