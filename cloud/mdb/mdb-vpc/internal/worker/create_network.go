package worker

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/models"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (w *Worker) CreateNetworkUsingProvider(ctx context.Context, op models.Operation) error {
	networkProvider, err := w.operationProvider(op)
	if err != nil {
		return err
	}

	err = networkProvider.CreateNetwork(ctx, op)
	if err != nil {
		return xerrors.Errorf("create network: %w", err)
	}
	return nil
}
