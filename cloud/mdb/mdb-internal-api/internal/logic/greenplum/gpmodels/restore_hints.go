package gpmodels

import (
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/console"
)

type RestoreHints struct {
	console.RestoreHints
	MasterResources  console.RestoreResources
	SegmentResources console.RestoreResources
	MasterHostCount  int64
	SegmentHostCount int64
	SegmentInHost    int64
}
