package producer

import (
	"context"
	"math/rand"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/encodingutil"
	"a.yandex-team.ru/cloud/mdb/internal/logbroker/writer"
	"a.yandex-team.ru/cloud/mdb/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/pgutil/pgerrors"
	"a.yandex-team.ru/cloud/mdb/mdb-event-producer/internal/events"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type SleepsConfig struct {
	From encodingutil.Duration `yaml:"from" json:"from"`
	To   encodingutil.Duration `yaml:"to" json:"to"`
}

type Config struct {
	Limit           int64        `yaml:"limit" json:"limit"`
	Sleeps          SleepsConfig `yaml:"sleeps" json:"sleeps"`
	IterationsLimit int          `yaml:"iterations_limit" json:"iterations_limit"`
}

func DefaultConfig() Config {
	return Config{
		Limit: 10,
		Sleeps: SleepsConfig{
			From: encodingutil.Duration{Duration: time.Second},
			To:   encodingutil.Duration{Duration: 5 * time.Second},
		},
		IterationsLimit: 100,
	}
}

var ErrMalformedConfig = xerrors.NewSentinel("config is malformed")

func (config Config) Validate() error {
	if config.Limit <= 0 {
		return xerrors.Errorf("Limit %d should be positive: %w", config.Limit, ErrMalformedConfig)
	}
	if config.IterationsLimit <= 0 {
		return xerrors.Errorf("IterationsLimit %d should be positive: %w", config.IterationsLimit, ErrMalformedConfig)
	}
	if config.Sleeps.From.Duration >= config.Sleeps.To.Duration {
		return xerrors.Errorf("Sleeps.From should be less then Sleeps.To (%+v): %w", config.Sleeps, ErrMalformedConfig)
	}
	return nil
}

// Producer send event from metadb worker_queue to
type Producer struct {
	mdb    metadb.MetaDB
	writer writer.Writer
	logger log.Logger
	config Config
}

func New(logger log.Logger, mdb metadb.MetaDB, writer writer.Writer, config Config) (*Producer, error) {
	if err := config.Validate(); err != nil {
		return nil, err
	}
	return &Producer{
		mdb:    mdb,
		writer: writer,
		logger: logger,
		config: config,
	}, nil
}

func (pr *Producer) sendToLB(events []events.LBEvent, eventsKind string) ([]int64, error) {
	writerDocs := make([]writer.Doc, len(events))
	for i := 0; i < len(events); i++ {
		writerDocs[i] = writer.Doc{
			ID:   events[i].ID,
			Data: events[i].Data,
		}
	}
	return writer.WriteAndWait(pr.writer, writerDocs, writer.Logger(log.With(pr.logger, log.String("event-kind", eventsKind))))
}

func (pr *Producer) sendStart(ctx context.Context) (int, error) {
	return pr.mdb.OnUnsentWorkerQueueStartEvents(ctx, pr.config.Limit, func(ctx context.Context, newEvents []metadb.WorkerQueueEvent) ([]int64, error) {
		lbEvents, err := events.FormatEvents(newEvents, events.StartStatus)
		if err != nil {
			return nil, err
		}
		return pr.sendToLB(lbEvents, "start")
	})
}

func (pr *Producer) sendDone(ctx context.Context) (int, error) {
	return pr.mdb.OnUnsentWorkerQueueDoneEvents(ctx, pr.config.Limit, func(ctx context.Context, newEvents []metadb.WorkerQueueEvent) ([]int64, error) {
		lbEvents, err := events.FormatEvents(newEvents, events.DoneStatus)
		if err != nil {
			return nil, err
		}
		return pr.sendToLB(lbEvents, "done")
	})
}

// SendEvents send start, then done events.
// Exit if no events found
func (pr *Producer) SendEvents(ctx context.Context) error {
	for iteration := 1; iteration <= pr.config.IterationsLimit; iteration++ {
		startSent, err := pr.sendStart(ctx)
		if err != nil {
			if !pgerrors.IsTemporary(err) {
				return err
			}
			pr.logger.Warnf("got temporary error while sending start events: %s", err)
		}
		doneSent, err := pr.sendDone(ctx)
		if err != nil {
			if !pgerrors.IsTemporary(err) {
				return err
			}
			pr.logger.Warnf("got temporary error while sending done events: %s", err)
		}
		if startSent == 0 && doneSent == 0 {
			pr.logger.Infof("at %d iteration I did nothing. Go to sleep.", iteration)
			return nil
		}
	}
	pr.logger.Infof("looks like iterations limit (%d) reach. Go to sleep.", pr.config.IterationsLimit)
	return nil
}

func (pr *Producer) sleepTime() time.Duration {
	sleepTime := rand.Int63n(int64(pr.config.Sleeps.To.Duration - pr.config.Sleeps.From.Duration))
	return time.Duration(sleepTime) + pr.config.Sleeps.To.Duration
}

func (pr *Producer) Run(ctx context.Context) error {
	timer := time.NewTimer(0)
	for {
		select {
		case <-ctx.Done():
			pr.logger.Info("context done. exiting")
			return nil
		case <-timer.C:
			pr.logger.Debug("I wakeup by timer")
			if err := pr.SendEvents(ctx); err != nil {
				pr.logger.Errorf("got unexpected error: %s. I give up.", err)
				return err
			}
			timer.Reset(pr.sleepTime())
		}
	}
}
