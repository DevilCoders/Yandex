package operations

import (
	"time"

	"github.com/golang/protobuf/ptypes"

	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/operation"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type Operation struct {
	ID          string
	Description string
	CreatedAt   time.Time
	CreatedBy   string
	ModifiedAt  time.Time
	Done        bool
	Metadata    interface{}

	Result string
	Error  error
}

func FromGRPC(gOp *operation.Operation) (Operation, error) {
	op := Operation{
		ID:          gOp.GetId(),
		Description: gOp.GetDescription(),
		CreatedBy:   gOp.GetCreatedBy(),
		Done:        gOp.GetDone(),
		Metadata:    gOp.GetMetadata(),
	}
	createdAt, err := ptypes.Timestamp(gOp.GetCreatedAt())
	if err != nil {
		return op, xerrors.Errorf("createdAt: %w", err)
	}
	op.CreatedAt = createdAt

	modifiedAt, err := ptypes.Timestamp(gOp.GetModifiedAt())
	if err != nil {
		return op, xerrors.Errorf("modifiedAt: %w", err)
	}
	op.ModifiedAt = modifiedAt

	if opErr := gOp.GetError(); opErr != nil {
		op.Error = xerrors.Errorf("code: %d, message: %s", opErr.GetCode(), opErr.GetMessage())
	}
	if resp := gOp.GetResponse(); resp != nil {
		op.Result = resp.String()
	}

	return op, nil
}
