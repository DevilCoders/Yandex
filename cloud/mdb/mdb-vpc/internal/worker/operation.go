package worker

import (
	"context"
	"sync"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/models"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/worker/network"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (w *Worker) RunOperation(ctx context.Context, wg *sync.WaitGroup) {
	defer wg.Done()

	ctx, err := w.vpcdb.Begin(ctx, sqlutil.Primary)
	if err != nil {
		ctxlog.Error(ctx, w.log, "Can't create tx", log.Error(err))
		return
	}
	defer func() {
		err = w.vpcdb.Commit(ctx)
		if err != nil {
			ctxlog.Error(ctx, w.log, "Can not commit result", log.Error(err))
			// TODO
			_ = w.vpcdb.Rollback(ctx)
		}
	}()

	op, err := w.vpcdb.OperationToProcess(ctx)
	if err != nil {
		if semerr.IsNotFound(err) {
			ctxlog.Debug(ctx, w.log, "Nothing to process")
		} else {
			ctxlog.Error(ctx, w.log, "Can not get operation to process", log.Error(err))
			// todo error handler
		}
		return
	}
	if op.StartTime.IsZero() {
		op.StartTime = time.Now()
	}
	op.Status = models.OperationStatusRunning

	ctx = ctxlog.WithFields(ctx, log.String("operationID", op.ID))
	ctxlog.Info(ctx, w.log, "Start operation processing")
	defer ctxlog.Info(ctx, w.log, "Finish operation processing")
	defer func() {
		err = w.vpcdb.UpdateOperationFields(ctx, op)
		if err != nil {
			ctxlog.Error(ctx, w.log, "Can not update operation", log.Error(err))
			// TODO
		}
	}()

	switch op.Action {
	case models.OperationActionCreateVPC:
		err = w.CreateNetworkUsingProvider(ctx, op)
		if err != nil {
			ctxlog.Error(ctx, w.log, "Can not create network", log.Error(err))
			return
			// TODO
		}
	case models.OperationActionDeleteVPC:
		if err = w.DeleteNetworkUsingProvider(ctx, op); err != nil {
			ctxlog.Error(ctx, w.log, "Can not delete network", log.Error(err))
			return
			// TODO
		}
	case models.OperationActionCreateNetworkConnection:
		if err = w.CreateNetworkConnectionUsingProvider(ctx, op); err != nil {
			ctxlog.Error(ctx, w.log, "Can not create network connection", log.Error(err))
			return
			// TODO
		}
	case models.OperationActionDeleteNetworkConnection:
		if err = w.DeleteNetworkConnectionUsingProvider(ctx, op); err != nil {
			ctxlog.Error(ctx, w.log, "Can not delete network connection", log.Error(err))
			return
			// TODO
		}
	case models.OperationActionImportVPC:
		if err = w.ImportVPCUsingProvider(ctx, op); err != nil {
			ctxlog.Error(ctx, w.log, "Can not import VPC", log.Error(err))
			return
		}
	default:
		ctxlog.Errorf(ctx, w.log, "Unknown action: %q", op.Action)
		// todo error
		return
	}

	op.Status = models.OperationStatusDone
	op.FinishTime = time.Now()
}

func (w *Worker) operationProvider(op models.Operation) (network.Service, error) {
	var networkProvider network.Service
	switch op.Provider {
	case models.ProviderAWS:
		var ok bool
		networkProvider, ok = w.awsNetworkProviders[op.Region]
		if !ok {
			return nil, xerrors.Errorf("unknown region of provider %q", op.Region)
		}
	default:
		return nil, xerrors.Errorf("unknown provider %q", op.Provider)
	}

	return networkProvider, nil
}
