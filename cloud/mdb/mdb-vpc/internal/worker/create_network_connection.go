package worker

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/models"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (w *Worker) CreateNetworkConnectionUsingProvider(ctx context.Context, op models.Operation) error {
	networkProvider, err := w.operationProvider(op)
	if err != nil {
		return err
	}

	err = networkProvider.CreateNetworkConnection(ctx, op)
	if err != nil {
		return xerrors.Errorf("create network connection: %w", err)
	}
	return nil
}
