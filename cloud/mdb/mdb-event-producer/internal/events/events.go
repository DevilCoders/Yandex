package events

import (
	"time"

	"github.com/valyala/fastjson"

	"a.yandex-team.ru/cloud/mdb/internal/metadb"
	"a.yandex-team.ru/library/go/core/xerrors"
)

// LBEvent is a event that we sending to LogBroker
// ID is our internal ID - matches to worker_queue_events.event_id
type LBEvent struct {
	ID   int64
	Data []byte
}

const (
	eventMetadataNode = "event_metadata"
	StartStatus       = "STARTED"
	DoneStatus        = "DONE"
)

// FormatEvent create LBEvent from metadb.WorkerQueueEvent
func FormatEvent(wqe metadb.WorkerQueueEvent, status string) (LBEvent, error) {
	if status != StartStatus && status != DoneStatus {
		return LBEvent{}, xerrors.Errorf("unsupported status %q. Should be one of %q, %q", status, StartStatus, DoneStatus)
	}
	jsonArena := fastjson.Arena{}
	jsonEv, err := fastjson.Parse(wqe.Data)
	if err != nil {
		return LBEvent{}, xerrors.Errorf("fail to parse event: %w", err)
	}
	jsonEv.Set("event_status", jsonArena.NewString(status))
	metadataNode := jsonEv.GetObject(eventMetadataNode)
	if metadataNode == nil {
		return LBEvent{}, xerrors.Errorf("%q node is missing", eventMetadataNode)
	}
	// google/protobuf/timestamp.proto: A proto3 JSON serializer should always use UTC
	metadataNode.Set("created_at", jsonArena.NewString(wqe.CreatedAt.UTC().Format(time.RFC3339)))
	evBytes := jsonEv.MarshalTo(nil)

	return LBEvent{
		ID:   wqe.ID,
		Data: evBytes,
	}, nil
}

// FormatEvents ...
func FormatEvents(wqEvents []metadb.WorkerQueueEvent, status string) ([]LBEvent, error) {
	events := make([]LBEvent, len(wqEvents))
	for i, ev := range wqEvents {
		ev, err := FormatEvent(ev, status)
		if err != nil {
			return nil, err
		}
		events[i] = ev
	}
	return events, nil
}
