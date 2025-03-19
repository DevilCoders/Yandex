package events

import (
	"context"

	"github.com/golang/protobuf/proto"

	"a.yandex-team.ru/cloud/bitbucket/public-api/yandex/cloud/events"
	as "a.yandex-team.ru/cloud/mdb/internal/compute/accessservice"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
)

//go:generate ../../../../../scripts/mockgen.sh Events

type Events interface {
	NewAuthentication(subject as.Subject) *events.Authentication
	NewAuthorization(subject as.Subject) *events.Authorization
	NewEventMetadata(m proto.Message, op operations.Operation, cloudExtID, folderExtID string) (*events.EventMetadata, error)
	NewRequestMetadata(ctx context.Context) *events.RequestMetadata
	Store(ctx context.Context, event proto.Message, op operations.Operation) error
}
