package steps

import (
	"context"
	"fmt"
	"strings"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/helpers/shipments"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/types"
	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/opmetas"
)

func WaitShipment(ctx context.Context, opMeta opmetas.DeployShipmentMeta, s shipments.AwaitShipment, rd *types.RequestDecisionTuple) RunResult {
	workResult, err := s.Wait(ctx, opMeta)
	if err != nil {
		return waitWithMessage(err.Error())
	}

	response := s.MessageForUserOnWait(ctx, workResult, rd)

	if workResult.IsSuccessful() {
		return continueWithMessage(response)
	}
	return waitWithMessage(response)
}

func MessageForUserOnCreate(createResult shipments.WrapperCreateResult) RunResult {
	message := fmt.Sprintf("successfully: %d", len(createResult.Success))
	if len(createResult.NotInDeploy) > 0 {
		message += fmt.Sprintf("\nnot in deploy: %s", strings.Join(createResult.NotInDeploy, ", "))
	}
	if len(createResult.NotHA) > 0 {
		message += fmt.Sprintf("\nnot ha cluster: %s", strings.Join(createResult.NotHA, ", "))
	}
	if len(createResult.NotCreated) > 0 {
		message += fmt.Sprintf("\nunable to create: %s", strings.Join(createResult.NotCreated, ", "))
	}
	if createResult.Error != nil {
		message += fmt.Sprintf("\nerror: %v", createResult.Error)
		return waitWithMessage("shipments creation ended with errors, this is unrecoverable, fix state yourself\n" + message)
	}
	if createResult.AllCreatedOk() {
		return waitWithMessage("created shipments\n" + message)
	}
	return waitWithMessage("this is unrecoverable, fix state yourself\n" + message)
}

func CreateShipment(ctx context.Context, fqdns []string, opMeta opmetas.DeployShipmentMeta, s shipments.AwaitShipment) RunResult {
	createResult := s.CreateShipment(ctx, fqdns, opMeta)
	return MessageForUserOnCreate(createResult)
}
