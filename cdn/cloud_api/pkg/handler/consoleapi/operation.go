package consoleapi

import (
	"context"

	"google.golang.org/protobuf/types/known/timestamppb"

	cdnpb "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/cdn/v1/console"
	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/operation"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

type OperationService struct {
	Logger log.Logger
}

func (o OperationService) Get(ctx context.Context, request *cdnpb.GetOperationRequest) (*operation.Operation, error) {
	ctxlog.Info(ctx, o.Logger, "operation get", log.Sprintf("request", "%s", request))

	op := &operation.Operation{
		Id:        request.GetOperationId(),
		CreatedAt: timestamppb.Now(),
		Done:      true,
	}

	return op, nil
}
