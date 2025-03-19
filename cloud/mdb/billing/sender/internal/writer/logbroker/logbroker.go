package logbroker

import (
	"context"
	"time"

	"a.yandex-team.ru/cloud/mdb/billing/sender/internal/writer"
	logbrokerWriter "a.yandex-team.ru/cloud/mdb/internal/logbroker/writer"
	"a.yandex-team.ru/cloud/mdb/internal/logbroker/writer/logbroker"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/slices"
)

type LogBrokerWriter struct {
	client logbrokerWriter.Writer
	l      log.Logger
}

var _ writer.Writer = &LogBrokerWriter{}

func New(ctx context.Context, config logbroker.Config, l log.Logger) (writer.Writer, error) {
	lbwriter, err := logbroker.New(
		ctx,
		config,
		logbroker.Logger(l),
		logbroker.WithRetryOnFailure(true),
	)
	if err != nil {
		return nil, xerrors.Errorf("LogBroker initialization failed: %w", err)
	}

	return &LogBrokerWriter{
		client: lbwriter,
		l:      l,
	}, nil
}

func (l *LogBrokerWriter) Write(docs []logbrokerWriter.Doc, timeout time.Duration) error {
	writtenIDs, err := logbrokerWriter.WriteAndWait(l.client, docs, logbrokerWriter.FeedbackTimeout(timeout), logbrokerWriter.Logger(l.l))
	if err != nil {
		return err
	}

	var targetIDs []int64
	for _, doc := range docs {
		targetIDs = append(targetIDs, doc.ID)
	}

	if !slices.EqualUnordered(writtenIDs, targetIDs) {
		return xerrors.Errorf("bad writer response: expected %+v, got %+v", targetIDs, writtenIDs)
	}

	return nil
}
