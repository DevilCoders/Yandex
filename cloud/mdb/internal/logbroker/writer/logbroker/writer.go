package logbroker

import (
	"context"
	"crypto/tls"
	"crypto/x509"
	"io/ioutil"
	"sync"
	"time"

	"github.com/prometheus/client_golang/prometheus"

	"a.yandex-team.ru/cloud/mdb/internal/compute/iam"
	"a.yandex-team.ru/cloud/mdb/internal/compute/iam/grpc"
	"a.yandex-team.ru/cloud/mdb/internal/crypto"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil"
	"a.yandex-team.ru/cloud/mdb/internal/logbroker/writer"
	"a.yandex-team.ru/kikimr/public/sdk/go/persqueue"
	"a.yandex-team.ru/kikimr/public/sdk/go/persqueue/log/corelogadapter"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/log/nop"
	"a.yandex-team.ru/library/go/core/xerrors"
	ydb_qloud "a.yandex-team.ru/library/go/yandex/ydb/auth/qloud"
)

type sendingDoc struct {
	ID        int64
	CreatedAt time.Time
	WrittenAt time.Time
}

type writerStats struct {
	Sent          prometheus.Counter
	RecvSuccess   prometheus.Counter
	RecvIssues    prometheus.Counter
	LBLatencyMS   prometheus.Histogram
	DocLatencySec prometheus.Histogram
}

type writerWrapper struct {
	lbWriter    persqueue.Writer
	l           log.Logger
	sending     []sendingDoc
	done        chan writer.WriteResponse
	sendingLock sync.Mutex
	stats       writerStats
}

type WriterOptions struct {
	L              log.Logger
	RetryOnFailure bool
}

func Logger(l log.Logger) Option {
	return func(args *WriterOptions) {
		args.L = l
	}
}

func WithRetryOnFailure(retry bool) Option {
	return func(args *WriterOptions) {
		args.RetryOnFailure = retry
	}
}

type Option func(*WriterOptions)

func initAuth(ctx context.Context, cfg Config, l log.Logger) (ydb.Credentials, error) {
	configs := 0
	if len(cfg.IAM.ServiceAccount.ID) > 0 {
		configs++
	}
	if len(cfg.OAuth.Token.Unmask()) > 0 {
		configs++
	}
	if len(cfg.TVM.Token.Unmask()) > 0 {
		configs++
	}
	if configs != 1 {
		return nil, xerrors.New("One and only one of oath.token, tvm.token and iam.service_account.id should be defined")
	}
	if len(cfg.IAM.ServiceAccount.ID) > 0 {
		client, err := grpc.NewTokenServiceClient(
			ctx,
			cfg.IAM.TokenService.Endpoint,
			cfg.IAM.TokenService.UserAgent,
			grpcutil.DefaultClientConfig(),
			&grpcutil.PerRPCCredentialsStatic{},
			l,
		)
		if err != nil {
			return nil, xerrors.Errorf("failed to initialize token service client %w", err)
		}
		return client.ServiceAccountCredentials(iam.ServiceAccount{
			ID:    cfg.IAM.ServiceAccount.ID,
			KeyID: cfg.IAM.ServiceAccount.KeyID.Unmask(),
			Token: []byte(cfg.IAM.ServiceAccount.PrivateKey.Unmask()),
		}), nil
	} else if len(cfg.OAuth.Token.Unmask()) > 0 {
		return ydb.AuthTokenCredentials{AuthToken: cfg.OAuth.Token.Unmask()}, nil
	} else {
		provider, err := ydb_qloud.NewQloudCredentialsFor(
			ctx,
			cfg.TVM.Token.Unmask(),
			cfg.TVM.ClientAlias,
			cfg.TVM.LogBrokerAlias,
			cfg.TVM.Port,
			l,
		)
		if err != nil {
			return nil, xerrors.Errorf("Unable to initialize TVM provider for LogBroker client: %w", err)
		}
		return provider, nil
	}
}

func initTLS(ctx context.Context, cfg Config) (*tls.Config, error) {
	if cfg.TLS.RootCAFile == "" {
		return nil, nil
	}
	data, err := ioutil.ReadFile(cfg.TLS.RootCAFile)
	if err != nil {
		return nil, xerrors.Errorf("failed to read ca_file: %w", err)
	}
	certs, err := crypto.CertificatesFromPem(data)
	if err != nil {
		return nil, xerrors.Errorf("failed to parse perm in ca_file: %w", err)
	}
	pool := x509.NewCertPool()
	for _, cert := range certs {
		pool.AddCert(cert)
	}
	return &tls.Config{RootCAs: pool}, nil
}

var stats writerStats
var statsOnce sync.Once

func newStats() writerStats {
	statsOnce.Do(func() {
		st := writerStats{
			Sent: prometheus.NewCounter(prometheus.CounterOpts{
				Name: "writer_documents_sent",
				Help: "A counter of documents sent to LogBroker.",
			}),
			RecvSuccess: prometheus.NewCounter(prometheus.CounterOpts{
				Name: "writer_documents_receive_success",
				Help: "A counter of documents for that we got success feedback.",
			}),
			RecvIssues: prometheus.NewCounter(prometheus.CounterOpts{
				Name: "writer_documents_receive_errors",
				Help: "A counter of documents for that we got issues.",
			}),
			LBLatencyMS: prometheus.NewHistogram(
				prometheus.HistogramOpts{
					Name:    "writer_logbroker_latency_milliseconds",
					Help:    "A histogram of latencies for logbroker write-receive cycle.",
					Buckets: []float64{.005, .5, 1, 5, 15, 30, 90, 180, 360, 720, 1500, 3600, 7200},
				},
			),
			DocLatencySec: prometheus.NewHistogram(
				prometheus.HistogramOpts{
					Name:    "writer_document_latency_seconds",
					Help:    "A histogram of latencies for document delivery to logbroker.",
					Buckets: []float64{.0001, .005, .5, 1, 5, 15, 30, 90, 180, 360, 720, 1500, 3600},
				},
			),
		}

		prometheus.MustRegister(
			st.Sent, st.RecvSuccess, st.RecvIssues,
			st.LBLatencyMS, st.DocLatencySec,
		)

		stats = st
	})
	return stats
}

// New construct new writer using config
func New(ctx context.Context, cfg Config, opts ...Option) (writer.Writer, error) {
	args := &WriterOptions{L: &nop.Logger{}}
	for _, optSetter := range opts {
		optSetter(args)
	}
	lbAuth, err := initAuth(ctx, cfg, args.L)
	if err != nil {
		return nil, err
	}
	lbTLS, err := initTLS(ctx, cfg)
	if err != nil {
		return nil, err
	}
	lbWriter := persqueue.NewWriter(
		persqueue.WriterOptions{
			Endpoint:       cfg.Endpoint,
			TLSConfig:      lbTLS,
			Credentials:    lbAuth,
			Logger:         corelogadapter.New(args.L),
			Database:       cfg.Database,
			Topic:          cfg.Topic,
			SourceID:       []byte(cfg.SourceID),
			Codec:          persqueue.Gzip,
			RetryOnFailure: args.RetryOnFailure,
			MaxMemory:      int(cfg.MaxMemory),
		},
	)
	wrapper := writerWrapper{
		lbWriter: lbWriter,
		l:        args.L,
		sending:  make([]sendingDoc, 0), done: make(chan writer.WriteResponse),
		stats: newStats(),
	}
	if err := wrapper.start(ctx); err != nil {
		return nil, err
	}
	return &wrapper, nil
}

// Start initializes writer
func (w *writerWrapper) start(ctx context.Context) error {
	ctxlog.Debugf(ctx, w.l, "Try start writer %+v", w.lbWriter)

	writerStart, err := w.lbWriter.Init(ctx)
	if err != nil {
		return err
	}
	ctxlog.Debugf(ctx, w.l, "LB client started %+v", writerStart)
	go w.recvResponses()

	return nil
}

func (w *writerWrapper) Write(id int64, message []byte, createdAt time.Time) error {
	msg := &persqueue.WriteMessage{
		Data: message,
	}
	msg.WithSeqNo(uint64(id))
	err := w.lbWriter.Write(msg)
	if err != nil {
		return err
	}

	w.sendingLock.Lock()
	w.sending = append(w.sending, sendingDoc{ID: id, WrittenAt: time.Now(), CreatedAt: createdAt})
	w.sendingLock.Unlock()

	w.stats.Sent.Add(1)

	return nil
}

func (w *writerWrapper) popSending() sendingDoc {
	w.sendingLock.Lock()
	if len(w.sending) == 0 {
		w.l.Fatal("Sending queue is empty. That should not happen")
	}
	sd := w.sending[0]
	w.sending = w.sending[1:]
	w.sendingLock.Unlock()
	return sd
}

func (w *writerWrapper) WriteResponses() <-chan writer.WriteResponse {
	return w.done
}

func (w *writerWrapper) recvResponses() {
	for rsp := range w.lbWriter.C() {
		sd := w.popSending()
		switch m := rsp.(type) {
		case *persqueue.Ack:
			w.l.Debugf("got Ack. Doc: %+v, Ack{SeqNo: %d, Offset: %d, AlreadyWritten: %t}", sd, m.SeqNo, m.Offset, m.AlreadyWritten)
			w.done <- writer.WriteResponse{ID: sd.ID}

			w.stats.RecvSuccess.Add(1)
			w.stats.LBLatencyMS.Observe(float64(time.Since(sd.WrittenAt) / time.Millisecond))
			if !sd.CreatedAt.IsZero() {
				w.stats.DocLatencySec.Observe(float64(time.Since(sd.CreatedAt) / time.Second))
			}
		case *persqueue.Issue:
			w.l.Infof("got error for ID %d: %v", sd.ID, m.Err)
			w.done <- writer.WriteResponse{ID: sd.ID, Err: m.Err}

			w.stats.RecvIssues.Add(1)
		}
	}
	close(w.done)
}
