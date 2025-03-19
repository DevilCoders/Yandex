package datatransfer

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/internal/ready"
)

type DataTransferService interface {
	ready.Checker

	ResolveFolderID(ctx context.Context, transfers []string) ([]string, error)
}
