package producer_test

import (
	"context"
	"fmt"
	"testing"
	"time"

	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/require"

	writer_mock "a.yandex-team.ru/cloud/mdb/internal/logbroker/writer/dummy"
	"a.yandex-team.ru/cloud/mdb/internal/metadb"
	metadb_mocks "a.yandex-team.ru/cloud/mdb/internal/metadb/mocks"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-event-producer/internal/producer"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/nop"
	"a.yandex-team.ru/library/go/core/log/zap"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func TestNew(t *testing.T) {
	ctrl := gomock.NewController(t)
	newMockedProducer := func(config producer.Config) (*producer.Producer, error) {

		return producer.New(&nop.Logger{}, metadb_mocks.NewMockMetaDB(ctrl), writer_mock.New(), config)
	}
	t.Run("DefaultConfig is OK", func(t *testing.T) {
		pr, err := newMockedProducer(producer.DefaultConfig())
		require.NoError(t, err)
		require.NotNil(t, pr)
	})
	for _, invalidLimit := range []int64{0, -1} {
		t.Run(fmt.Sprintf("Error for invalid Limit: %d", invalidLimit), func(t *testing.T) {
			cfg := producer.DefaultConfig()
			cfg.Limit = invalidLimit
			_, err := newMockedProducer(cfg)
			require.Error(t, err)
		})
	}
	for _, invalidLimit := range []int{0, -1} {
		t.Run(fmt.Sprintf("Error for invalid IterationsLimit: %d", invalidLimit), func(t *testing.T) {
			cfg := producer.DefaultConfig()
			cfg.IterationsLimit = invalidLimit
			_, err := newMockedProducer(cfg)
			require.Error(t, err)
		})
	}
	t.Run("invalid Sleeps range", func(t *testing.T) {
		cfg := producer.DefaultConfig()
		cfg.Sleeps.From.Duration = time.Minute * 2
		cfg.Sleeps.To.Duration = time.Minute
		_, err := newMockedProducer(cfg)
		require.Error(t, err)
	})
}

const (
	onStartEvents = "OnUnsentWorkerQueueStartEvents"
	onDoneEvents  = "OnUnsentWorkerQueueDoneEvents"
)

type producerDeps struct {
	config producer.Config
	mdb    *metadb_mocks.MockMetaDB
	writer *writer_mock.Writer
}

func (p producerDeps) OnStartEvents(events []metadb.WorkerQueueEvent, err error) *gomock.Call {
	return p.mdb.EXPECT().OnUnsentWorkerQueueStartEvents(gomock.Any(), p.config.Limit, gomock.Any()).DoAndReturn(
		func(ctx context.Context, _ int64, handler metadb.OnWorkerQueueEventsHandler) (int, error) {
			if err != nil {
				return 0, err
			}
			seenIds, err := handler(ctx, events)
			return len(seenIds), err
		})
}

func (p producerDeps) OnDoneEvents(events []metadb.WorkerQueueEvent, err error) *gomock.Call {
	return p.mdb.EXPECT().OnUnsentWorkerQueueDoneEvents(gomock.Any(), p.config.Limit, gomock.Any()).DoAndReturn(
		func(ctx context.Context, _ int64, handler metadb.OnWorkerQueueEventsHandler) (int, error) {
			if err != nil {
				return 0, err
			}
			seenIds, err := handler(ctx, events)
			return len(seenIds), err
		})
}

func newPrMocks(t *testing.T) producerDeps {
	config := producer.DefaultConfig()
	config.Sleeps.From.Duration = time.Millisecond
	config.Sleeps.To.Duration = 100 * time.Millisecond
	config.IterationsLimit = 3
	ctrl := gomock.NewController(t)

	return producerDeps{config: config, mdb: metadb_mocks.NewMockMetaDB(ctrl), writer: writer_mock.New()}
}

const (
	evData    = `{"event_metadata": {}}`
	badEvData = `<xml/>`
)

func TestProducer_SendEvents(t *testing.T) {
	var noEvents []metadb.WorkerQueueEvent
	oneEvent := []metadb.WorkerQueueEvent{{ID: 1, Data: evData}}
	badEvent := []metadb.WorkerQueueEvent{{ID: 42, Data: badEvData}}
	fiveEvents := []metadb.WorkerQueueEvent{
		{ID: 1, Data: evData},
		{ID: 2, Data: evData},
		{ID: 3, Data: evData},
		{ID: 4, Data: evData},
		{ID: 5, Data: evData},
	}
	newPr := func() (*producer.Producer, producerDeps) {
		mocks := newPrMocks(t)
		logger, _ := zap.New(zap.KVConfig(log.DebugLevel))
		pr, _ := producer.New(logger, mocks.mdb, mocks.writer, mocks.config)
		return pr, mocks
	}
	t.Run("test do nothing when no new events", func(t *testing.T) {
		pr, mk := newPr()
		mk.OnStartEvents(noEvents, nil).Times(1)
		mk.OnDoneEvents(noEvents, nil).Times(1)

		err := pr.SendEvents(context.Background())
		require.NoError(t, err)
	})
	t.Run("mark ids as sent", func(t *testing.T) {
		pr, mk := newPr()
		mk.OnStartEvents(oneEvent, nil).Times(1)
		mk.OnStartEvents(noEvents, nil).Times(1)
		mk.OnDoneEvents([]metadb.WorkerQueueEvent{}, nil).Times(2)

		err := pr.SendEvents(context.Background())
		require.NoError(t, err)
	})
	t.Run("mark ids as done", func(t *testing.T) {
		pr, mk := newPr()
		mk.OnStartEvents(noEvents, nil).Times(1)
		mk.OnDoneEvents(noEvents, nil).Times(1)

		err := pr.SendEvents(context.Background())
		require.NoError(t, err)
	})
	t.Run("don't give up on temporary meta errors", func(t *testing.T) {
		pr, mk := newPr()
		mk.OnStartEvents(noEvents, semerr.Unavailable("unavailable"))
		mk.OnDoneEvents(noEvents, semerr.Unavailable("unavailable"))

		err := pr.SendEvents(context.Background())
		require.NoError(t, err)
	})
	t.Run("don't give up when receive feedback error for part of documents", func(t *testing.T) {
		pr, mk := newPr()
		mk.writer.FeedbackErrors[fiveEvents[2].ID] = xerrors.New("Feedback error")
		mk.OnStartEvents(fiveEvents, nil).AnyTimes()
		mk.OnDoneEvents(noEvents, nil).AnyTimes()

		err := pr.SendEvents(context.Background())
		require.NoError(t, err)
	})

	t.Run("don't give up when some writes fails", func(t *testing.T) {
		pr, mk := newPr()
		mk.writer.WriteErrors[fiveEvents[2].ID] = xerrors.New("Write error")
		mk.OnStartEvents(fiveEvents, nil).AnyTimes()
		mk.OnDoneEvents(noEvents, nil).AnyTimes()

		err := pr.SendEvents(context.Background())
		require.NoError(t, err)
	})
	t.Run("give up if all chunk write fails", func(t *testing.T) {
		pr, mk := newPr()
		mk.writer.WriteErrors[oneEvent[0].ID] = xerrors.New("Write error")
		mk.OnStartEvents(oneEvent, nil)

		err := pr.SendEvents(context.Background())
		require.Error(t, err)
	})
	t.Run("give up on unexpected errors in start-sent", func(t *testing.T) {
		pr, mk := newPr()
		mk.OnStartEvents(noEvents, xerrors.New("Test error"))

		err := pr.SendEvents(context.Background())
		require.Error(t, err)
	})
	t.Run("give up on unexpected errors in done-sent", func(t *testing.T) {
		pr, mk := newPr()
		mk.OnStartEvents(noEvents, nil)
		mk.OnDoneEvents(noEvents, xerrors.New("Test error"))

		err := pr.SendEvents(context.Background())
		require.Error(t, err)
	})
	t.Run("exit when exceed IterationsLimit", func(t *testing.T) {
		pr, mk := newPr()
		mk.OnStartEvents(oneEvent, nil).Times(mk.config.IterationsLimit)
		mk.OnDoneEvents(oneEvent, nil).Times(mk.config.IterationsLimit)

		err := pr.SendEvents(context.Background())
		require.NoError(t, err)
	})
	t.Run("fail on bad start events", func(t *testing.T) {
		pr, mk := newPr()
		mk.OnStartEvents(badEvent, nil)

		err := pr.SendEvents(context.Background())
		require.Error(t, err)
	})
	t.Run("fail on bad done events", func(t *testing.T) {
		pr, mk := newPr()
		mk.OnStartEvents(noEvents, nil)
		mk.OnDoneEvents(badEvent, nil)

		err := pr.SendEvents(context.Background())
		require.Error(t, err)
	})
}
