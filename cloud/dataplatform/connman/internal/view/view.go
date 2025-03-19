package view

import (
	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/operation"
	"a.yandex-team.ru/cloud/dataplatform/connman/internal/protoutil"
	"a.yandex-team.ru/cloud/dataplatform/connman/internal/storage/types"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func PrepareConnectionOperationView(op *types.ConnectionOperation, connection *types.Connection) (*operation.Operation, error) {
	protoutil.ClearSensitiveData(op.Metadata.ProtoReflect())
	if connection != nil {
		protoutil.ClearSensitiveData(connection.Params.ProtoReflect())
	}
	operationProto, err := op.ToProto(connection)
	if err != nil {
		return nil, xerrors.Errorf("unable to marshal connection operation to proto: %w", err)
	}
	return operationProto, nil
}
