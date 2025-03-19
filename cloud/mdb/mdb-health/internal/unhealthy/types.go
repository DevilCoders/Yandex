package unhealthy

import (
	"fmt"

	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
)

const MaxUnhealthyExamples = 100

type RWKey struct {
	AggType         types.AggType
	SLA             bool
	Env             string
	Readable        bool
	Writeable       bool
	UserfaultBroken bool
}

type StatusKey struct {
	Status string
	Env    string
	SLA    bool
}

type GeoKey struct {
	Geo string
	Env string
	SLA bool
}

type UARecord struct {
	Count    int
	Examples []string
}

type UARWRecord struct {
	NoReadCount  int
	NoWriteCount int
	Count        int
	Examples     []string
}

func (r *UARecord) AddExample(ID string) {
	if len(r.Examples) < MaxUnhealthyExamples {
		r.Examples = append(r.Examples, ID)
	}
}

func (r *UARWRecord) AddExample(ID string) {
	if len(r.Examples) < MaxUnhealthyExamples {
		r.Examples = append(r.Examples, ID)
	}
}

func (r *UARecord) AddCount() {
	r.Count++
}

func (r *UARWRecord) AddCount(rw *types.DBRWInfo) {
	r.Count++
	if rw == nil {
		return
	}
	if rw.DBWrite == 0 {
		r.NoWriteCount++
	}
	if rw.DBRead == 0 {
		r.NoReadCount++
	}
}

type UAInfo struct {
	RWRecs         map[RWKey]*UARWRecord
	StatusRecs     map[StatusKey]*UARecord
	WarningGeoRecs map[GeoKey]*UARecord
}

func (uai UAInfo) String() string {
	var res string
	for k, v := range uai.StatusRecs {
		res += fmt.Sprintf("\n\t%v: %v", k, v)
	}
	for k, v := range uai.RWRecs {
		res += fmt.Sprintf("\n\t%v: %v", k, v)
	}
	for k, v := range uai.WarningGeoRecs {
		res += fmt.Sprintf("\n\t%v: %v", k, v)
	}
	return res
}
