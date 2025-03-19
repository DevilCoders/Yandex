package models

import (
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/environment"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
)

type ListOperationsArgs struct {
	ClusterID         optional.String
	Limit             optional.Int64
	Environment       environment.SaltEnv
	ClusterType       clusters.Type
	Type              operations.Type
	CreatedBy         optional.String
	PageTokenID       optional.String
	PageTokenCreateTS optional.Time
	IncludeHidden     optional.Bool
}

func NewOperationsPageToken(ops []operations.Operation, expectedPageSize int64) operations.OperationsPageToken {
	actualSize := int64(len(ops))

	var lastOpID string
	var lastOpCreatedAt time.Time
	if actualSize > 0 {
		lastOpID = ops[actualSize-1].OperationID
		lastOpCreatedAt = ops[actualSize-1].CreatedAt
	}

	return operations.NewOperationsPageToken(actualSize, expectedPageSize, lastOpID, lastOpCreatedAt)
}
