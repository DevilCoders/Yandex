package worker

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/models"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (w *Worker) DeleteNetworkUsingProvider(ctx context.Context, op models.Operation) error {
	networkProvider, err := w.operationProvider(op)
	if err != nil {
		return err
	}

	err = networkProvider.DeleteNetwork(ctx, op)
	if err != nil {
		return xerrors.Errorf("delete network: %w", err)
	}
	return nil
}
