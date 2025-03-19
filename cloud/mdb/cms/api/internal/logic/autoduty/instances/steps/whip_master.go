package steps

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/instances/opcontext"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/shipments"
	"a.yandex-team.ru/cloud/mdb/deploy/api/pkg/deployapi"
	deployModels "a.yandex-team.ru/cloud/mdb/deploy/api/pkg/models"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type WhipMaster struct {
	client shipments.ShipmentClient
}

func (s WhipMaster) Name() string {
	return StepNameWhipMaster
}

func (s WhipMaster) RunStep(ctx context.Context, stepCtx *opcontext.OperationContext, _ log.Logger) RunResult {
	if stepCtx.State().CheckIsMasterStep.IsNotMaster {
		return continueWithMessage(stepCtx.State().CheckIsMasterStep.Reason)
	}

	state := stepCtx.State().WhipMasterStep
	shipmentID := state.Shipment
	if shipmentID == 0 {
		shipment, err := s.client.EnsureNoPrimary(ctx, stepCtx.FQDN())
		if err != nil {
			if xerrors.Is(err, deployapi.ErrNotFound) {
				return failWithErrorAndMessage(err, "deploy knows nothing about this host")
			}
			return waitWithErrAndMessage(err, "can not create shipment")
		}
		state.Shipment = shipment
		return waitWithMessageFmt("created shipment id %d", shipment)
	} else {
		shipment, err := s.client.Shipment(ctx, shipmentID)
		if err != nil {
			if xerrors.Is(err, deployapi.ErrNotFound) {
				return failWithErrorAndMessageFmt(err, "something went wrong, there is no shipment id %d", shipmentID)
			}
			return waitWithErrAndMessage(err, "can not get shipment info")
		}

		switch shipment.Status {
		case deployModels.ShipmentStatusDone:
			return continueWithMessageFmt("shipment %d has been finished successfully", shipmentID)
		case deployModels.ShipmentStatusInProgress:
			return waitWithMessageFmt("shipment %d is not finished yet", shipmentID)
		default:
			return failWithMessageFmt("shipment %d has been failed", shipmentID)
		}
	}
}

func NewWhipMaster(client shipments.ShipmentClient) WhipMaster {
	return WhipMaster{client: client}
}
