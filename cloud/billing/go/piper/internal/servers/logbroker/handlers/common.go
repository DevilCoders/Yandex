package handlers

import (
	"context"
	"os"

	"github.com/jonboulle/clockwork"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/core/entities"

	"a.yandex-team.ru/cloud/billing/go/pkg/logbroker/lbtypes"
)

const (
	sourceType = "logbroker-grpc"
)

var hostname = func() string {
	name, err := os.Hostname()
	if err != nil {
		panic(err)
	}
	return name
}()

func filterMessagesBySize(limit int, msg []lbtypes.ReadMessage) (valid, oversized []lbtypes.ReadMessage) {
	if limit <= 0 {
		return msg, nil
	}

	valid = make([]lbtypes.ReadMessage, 0, len(msg))
	for _, m := range msg {
		if m.RawSize() > int(limit) {
			oversized = append(oversized, m)
			continue
		}
		valid = append(valid, m)
	}

	return
}

type commonHandler struct {
	handlerName string
	sourceName  string
	pipeline    string

	clock clockwork.Clock
}

func newCommonHandler(handlerName string, sourceName string, pipeline string) commonHandler {
	return commonHandler{
		handlerName: handlerName,
		sourceName:  sourceName,
		pipeline:    pipeline,
		clock:       clockwork.NewRealClock(),
	}
}

func (h commonHandler) init(ctx context.Context, sourceID lbtypes.SourceID, messages *lbtypes.Messages) (context.Context, func(), entities.ProcessingScope) {
	ctx, cm := tooling.InitContext(ctx, h.pipeline)
	cm.SetSource(string(sourceID))
	cm.SetHandler(h.handlerName)
	cm.InitTracing(h.pipeline)

	scope := entities.ProcessingScope{
		SourceName: h.sourceName,
		SourceType: sourceType,
		SourceID:   string(sourceID),
		StartTime:  h.clock.Now(),

		Hostname: hostname,
		Pipeline: h.pipeline,

		MaxMessageOffset: messages.LastOffset,
		MinMessageOffset: messages.Messages[0].Offset,
	}
	return ctx, func() { cm.FinishTracing() }, scope
}
