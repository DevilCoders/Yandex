package operations

import (
	"google.golang.org/grpc/status"
	"google.golang.org/protobuf/reflect/protoreflect"

	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/actions/operations"
	"a.yandex-team.ru/cloud/marketplace/pkg/protob"
)

func GetOperationUpdateParams(id string, meta, res protoreflect.ProtoMessage, opErr error) (*operations.UpdateParams, error) {
	opUpdateParams := &operations.UpdateParams{
		ID:   id,
		Done: true,
	}

	if meta != nil {
		metaRaw, err := protob.JSONFromProtoMessage(meta)
		if err != nil {
			return nil, err
		}
		opUpdateParams.Metadata = metaRaw
	}

	switch opErr {
	case nil:
		resRaw, err := protob.JSONFromProtoMessage(res)
		if err != nil {
			return nil, err
		}

		opUpdateParams.Result = resRaw
	default:
		opErrRaw, err := protob.JSONFromProtoMessage(status.Convert(opErr).Proto())
		if err != nil {
			return nil, err
		}

		opUpdateParams.Result = opErrRaw
	}

	return opUpdateParams, nil
}
