package afpillars

import "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/airflow/afmodels"

func (res *PodResources) ToModel() afmodels.Resources {
	return afmodels.Resources{
		VCPUCount:   res.Limits.VCPUCount,
		MemoryBytes: res.Limits.MemoryBytes,
	}
}

func FromModel(modelRes afmodels.Resources) (res PodResources) {
	res.Requests.VCPUCount = modelRes.VCPUCount
	res.Requests.MemoryBytes = modelRes.MemoryBytes
	res.Limits = res.Requests
	return
}
