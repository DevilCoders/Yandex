// TODO: think about code generation for events

package provider

import (
	"context"

	"github.com/golang/protobuf/jsonpb"
	"github.com/golang/protobuf/proto"
	"google.golang.org/grpc/metadata"
	"google.golang.org/grpc/peer"

	"a.yandex-team.ru/cloud/bitbucket/public-api/yandex/cloud/events"
	as "a.yandex-team.ru/cloud/mdb/internal/compute/accessservice"
	"a.yandex-team.ru/cloud/mdb/internal/generator"
	"a.yandex-team.ru/cloud/mdb/internal/requestid"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc"
	logicevents "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/events"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type Events struct {
	metaDB      metadb.Backend
	idGenerator generator.IDGenerator
}

var _ logicevents.Events = &Events{}

func NewEvents(metaDB metadb.Backend, gen generator.IDGenerator) *Events {
	return &Events{metaDB: metaDB, idGenerator: gen}
}

func (*Events) NewAuthentication(subject as.Subject) *events.Authentication {
	ev := &events.Authentication{Authenticated: true}
	if subject.User != nil {
		ev.SubjectType = events.Authentication_YANDEX_PASSPORT_USER_ACCOUNT
		ev.SubjectId = subject.User.ID
	} else if subject.Service != nil {
		ev.SubjectType = events.Authentication_SERVICE_ACCOUNT
		ev.SubjectId = subject.Service.ID
	}

	// We do not handle no info or all info set cases on purpose - they must be handled somewhere above
	return ev
}

func (*Events) NewAuthorization(subject as.Subject) *events.Authorization {
	// TODO: set permissions
	return &events.Authorization{Authorized: true}
}

func (e *Events) NewEventMetadata(m proto.Message, op operations.Operation, cloudExtID, folderExtID string) (*events.EventMetadata, error) {
	id, err := e.idGenerator.Generate()
	if err != nil {
		return nil, xerrors.Errorf("event id not generated: %w", err)
	}

	return &events.EventMetadata{
		EventId:   id,
		EventType: proto.MessageName(m),
		CreatedAt: grpc.TimeToGRPC(op.CreatedAt),
		// TracingContext: nil,        // TODO
		CloudId:  cloudExtID,
		FolderId: folderExtID,
	}, nil
}

func (*Events) NewRequestMetadata(ctx context.Context) *events.RequestMetadata {
	rm := &events.RequestMetadata{
		RequestId: requestid.MustFromContext(ctx),
	}

	// TODO: its a good idea to translate user-agent and client's address into standard 'internal' data types but for now we leave it as is
	// If someone needs to use those things anywhere else, this MUST be translated
	// Set first user-agent available if any
	md, _ := metadata.FromIncomingContext(ctx)
	uas := md.Get("user-agent")
	if len(uas) > 0 {
		rm.UserAgent = uas[0]
	}

	// Set remote address if any
	p, ok := peer.FromContext(ctx)
	if ok {
		rm.RemoteAddress = p.Addr.String()
	}

	// TODO: indempotency
	return rm
}

// Store event in database
func (e *Events) Store(ctx context.Context, event proto.Message, op operations.Operation) error {
	marshaled, err := marshalEvent(event)
	if err != nil {
		return xerrors.Errorf("failed to marshal event %s for op %+v: %w", event, op, err)
	}

	if err = e.metaDB.CreateWorkerQueueEvent(ctx, op.OperationID, []byte(marshaled)); err != nil {
		return xerrors.Errorf("failed to create worker queue event %s for op %+v: %w", event, op, err)
	}

	return nil
}

func marshalEvent(event proto.Message) ([]byte, error) {
	m := jsonpb.Marshaler{EmitDefaults: true, OrigName: true}
	marshaled, err := m.MarshalToString(event)
	if err != nil {
		return nil, err
	}

	return []byte(marshaled), nil
}
