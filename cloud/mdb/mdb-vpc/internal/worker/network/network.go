package network

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/models"
)

//go:generate ../../../../scripts/mockgen.sh Service

type Service interface {
	CreateNetwork(ctx context.Context, op models.Operation) error
	DeleteNetwork(ctx context.Context, op models.Operation) error
	CreateNetworkConnection(ctx context.Context, op models.Operation) error
	DeleteNetworkConnection(ctx context.Context, op models.Operation) error
	ImportVPC(ctx context.Context, op models.Operation) error
}
