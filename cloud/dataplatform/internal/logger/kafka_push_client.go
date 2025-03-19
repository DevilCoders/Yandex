package logger

import (
	"bytes"
	"context"
	"crypto/tls"
	"crypto/x509"
	"os"
	"sync"

	"github.com/segmentio/kafka-go"
	"github.com/segmentio/kafka-go/sasl/scram"
	zp "go.uber.org/zap"
	"go.uber.org/zap/zapcore"

	"a.yandex-team.ru/kikimr/public/sdk/go/persqueue"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type KafkaConfig struct {
	Broker      string
	Topic       string
	User        string
	Password    string
	TLSFile     string
	TLSInsecure bool
}

type kafkaPusher struct {
	producer *kafka.Writer
}

func (p *kafkaPusher) Write(data []byte) (int, error) {
	msg := kafka.Message{
		Value: copySlice(data),
	}
	if err := p.producer.WriteMessages(context.Background(), msg); err != nil {
		return 0, err
	}
	return len(data), nil
}

func NewKafkaLogger(cfg *KafkaConfig) (*zap.Logger, error) {
	mechanism, err := scram.Mechanism(scram.SHA512, cfg.User, cfg.Password)
	if err != nil {
		return nil, err
	}
	var tlsCfg *tls.Config
	if cfg.TLSFile != "" {
		cp, err := x509.SystemCertPool()
		if err != nil {
			return nil, err
		}
		if !cp.AppendCertsFromPEM([]byte(cfg.TLSFile)) {
			return nil, xerrors.Errorf("credentials: failed to append certificates")
		}
		tlsCfg = &tls.Config{
			RootCAs: cp,
		}
	} else if cfg.TLSInsecure {
		tlsCfg = &tls.Config{
			InsecureSkipVerify: true,
		}
	}
	pr := &kafka.Writer{
		Addr:      kafka.TCP(cfg.Broker),
		Topic:     cfg.Topic,
		Async:     true,
		Balancer:  &kafka.LeastBytes{},
		Transport: &kafka.Transport{TLS: tlsCfg, SASL: mechanism},
	}

	f := kafkaPusher{
		producer: pr,
	}
	syncLb := zapcore.AddSync(&f)
	stdOut := zapcore.AddSync(os.Stdout)
	defaultPriority := zp.LevelEnablerFunc(func(lvl zapcore.Level) bool {
		return lvl >= zapcore.InfoLevel
	})
	lbEncoder := zapcore.NewJSONEncoder(zap.JSONConfig(log.InfoLevel).EncoderConfig)
	stdErrCfg := zap.CLIConfig(log.InfoLevel).EncoderConfig
	stdErrCfg.EncodeLevel = zapcore.CapitalColorLevelEncoder
	stdErrEncoder := zapcore.NewConsoleEncoder(stdErrCfg)
	lbCore := zapcore.NewTee(
		zapcore.NewCore(stdErrEncoder, stdOut, defaultPriority),
		zapcore.NewCore(lbEncoder, zapcore.Lock(syncLb), defaultPriority),
	)
	Log.Info("WriterInit Kafka logger", log.Any("topic", cfg.Topic), log.Any("instance", cfg.Broker))
	return &zap.Logger{
		L: zp.New(
			lbCore,
			zp.AddCaller(),
			zp.AddCallerSkip(1),
			zp.AddStacktrace(zp.WarnLevel),
		),
	}, nil
}

type kafkaPushClient struct {
	producer persqueue.Writer
	rw       sync.Mutex
	buf      bytes.Buffer
}

func (f *kafkaPushClient) Write(p []byte) (n int, err error) {
	f.rw.Lock()
	defer f.rw.Unlock()
	f.buf.Write(p)
	n = len(p)
	if f.buf.Len() > 1024 {
		combined := f.buf.Bytes()
		if string(combined)[f.buf.Len()-1] != '\n' {
			return
		}
		err = f.producer.Write(&persqueue.WriteMessage{Data: copyBytes(combined)})
		if err != nil {
			return
		}
		f.buf.Reset()
	}
	return
}
