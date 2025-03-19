package producer

import (
	"context"
	"math/rand"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/encodingutil"
	"a.yandex-team.ru/cloud/mdb/internal/logbroker/writer"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/pgutil/pgerrors"
	"a.yandex-team.ru/cloud/mdb/mdb-search-producer/internal/metadb"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type SleepsConfig struct {
	From encodingutil.Duration `yaml:"from" json:"from"`
	To   encodingutil.Duration `yaml:"to" json:"to"`
}

type Config struct {
	Limit           int64        `yaml:"limit" json:"limit"`
	IterationsLimit int          `yaml:"iterations_limit" json:"iterations_limit"`
	Sleeps          SleepsConfig `yaml:"sleeps" json:"sleeps"`
}

func DefaultConfig() Config {
	return Config{
		Limit: 10,
		Sleeps: SleepsConfig{
			From: encodingutil.Duration{Duration: time.Second},
			To:   encodingutil.Duration{Duration: 5 * time.Second},
		},
	}
}

var ErrMalformedConfig = xerrors.NewSentinel("config is malformed")

func (config Config) Validate() error {
	if config.Limit <= 0 {
		return ErrMalformedConfig.Wrap(xerrors.Errorf("Limit should be positive. got %d", config.Limit))
	}
	if config.Sleeps.From.Duration >= config.Sleeps.To.Duration {
		return ErrMalformedConfig.Wrap(xerrors.Errorf("Sleeps.From should be less then Sleeps.To (%+v)", config.Sleeps))
	}
	return nil
}

// Producer send search queue documents from metadb to logbroker
type Producer struct {
	mdb    metadb.MetaDB
	writer writer.Writer
	L      log.Logger
	config Config
}

func New(L log.Logger, mdb metadb.MetaDB, writer writer.Writer, config Config) (*Producer, error) {
	if err := config.Validate(); err != nil {
		return nil, err
	}
	return &Producer{
		mdb:    mdb,
		writer: writer,
		L:      L,
		config: config,
	}, nil
}

func (pr *Producer) sendToLB(_ context.Context, docs []metadb.UnsentSearchDoc) ([]int64, error) {
	writerDocs := make([]writer.Doc, len(docs))
	for i := 0; i < len(docs); i++ {
		writerDocs[i] = writer.Doc{
			ID:        docs[i].QueueID,
			Data:      []byte(docs[i].Doc),
			CreatedAt: docs[i].CreatedAt,
		}
	}
	return writer.WriteAndWait(pr.writer, writerDocs, writer.Logger(pr.L))
}

// SendDocs sent them until got them from search_queue
func (pr *Producer) SendDocs(ctx context.Context) error {
	for {
		sent, err := pr.mdb.OnUnsentSearchQueueDoc(ctx, pr.config.Limit, pr.sendToLB)
		if err != nil {
			if pgerrors.IsTemporary(err) {
				pr.L.Warn("got temporary error", log.Error(err))
				return nil
			}
			return err
		}
		if sent == 0 {
			return nil
		}
	}
}

func (pr *Producer) sleepTime() time.Duration {
	sleepTime := rand.Int63n(int64(pr.config.Sleeps.To.Duration - pr.config.Sleeps.From.Duration))
	return time.Duration(sleepTime) + pr.config.Sleeps.To.Duration
}

func (pr *Producer) Run(ctx context.Context) error {
	timer := time.NewTimer(0)
	for i := 0; ; i++ {
		select {
		case <-ctx.Done():
			pr.L.Info("context done. exiting")
			return nil
		case <-timer.C:
			pr.L.Debug("I wakeup by timer")
			if err := pr.SendDocs(ctx); err != nil {
				pr.L.Errorf("got unexpected error: %s. I give up.", err)
				return err
			}
			timer.Reset(pr.sleepTime())
		}

		if pr.config.IterationsLimit > 0 && i >= pr.config.IterationsLimit {
			pr.L.Infof("exit cause reach IterationsLimit: %d", i)
			return nil
		}
	}
}
