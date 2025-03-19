package kinesis

import (
	"context"
	"strconv"
	"time"

	"github.com/aws/aws-sdk-go/aws"
	"github.com/aws/aws-sdk-go/aws/awserr"
	awsCredentials "github.com/aws/aws-sdk-go/aws/credentials"
	awsSession "github.com/aws/aws-sdk-go/aws/session"
	awsKinesis "github.com/aws/aws-sdk-go/service/kinesis"
	"golang.org/x/exp/slices"

	"a.yandex-team.ru/cloud/mdb/billing/sender/internal/writer"
	logbrokerWriter "a.yandex-team.ru/cloud/mdb/internal/logbroker/writer"
	"a.yandex-team.ru/cloud/mdb/internal/secret"
	"a.yandex-team.ru/library/go/core/log"
)

type Config struct {
	Region     string        `json:"region" yaml:"region"`
	Endpoint   string        `json:"endpoint" yaml:"endpoint"`
	AccessKey  secret.String `json:"access_key" yaml:"access_key"`
	SecretKey  secret.String `json:"secret_key" yaml:"secret_key"`
	StreamName string        `json:"stream_name" yaml:"stream_name"`
}

type KinesisWriter struct {
	client *awsKinesis.Kinesis
	cfg    Config
	l      log.Logger
}

var _ writer.Writer = &KinesisWriter{}

func New(ctx context.Context, config Config, l log.Logger) (writer.Writer, error) {
	session, err := awsSession.NewSession(aws.NewConfig().
		WithRegion(config.Region).
		WithCredentials(awsCredentials.NewStaticCredentials(config.AccessKey.Unmask(), config.SecretKey.Unmask(), "")).
		WithEndpoint(config.Endpoint).
		WithLogLevel(aws.LogDebugWithRequestRetries | aws.LogDebugWithRequestErrors).
		WithMaxRetries(0). // Forbid retries to prevent duplicates
		WithLogger(aws.LoggerFunc(func(args ...interface{}) {
			l.Infof("%+v", args...)
		})),
	)
	if err != nil {
		return nil, err
	}

	return &KinesisWriter{
		client: awsKinesis.New(session),
		cfg:    config,
		l:      l,
	}, nil
}

var (
	retriableErrorCodes = []string{
		awsKinesis.ErrCodeInternalFailureException,
		awsKinesis.ErrCodeInvalidArgumentException,
		awsKinesis.ErrCodeLimitExceededException,
		awsKinesis.ErrCodeProvisionedThroughputExceededException,
		awsKinesis.ErrCodeValidationException,
		awsKinesis.ErrCodeResourceNotFoundException,
	}
)

func (w *KinesisWriter) Write(docs []logbrokerWriter.Doc, timeout time.Duration) error {
	ctx, cancel := context.WithTimeout(context.Background(), timeout)
	defer cancel()

	request := &awsKinesis.PutRecordsInput{StreamName: &w.cfg.StreamName}
	for _, doc := range docs {
		request.Records = append(request.Records, &awsKinesis.PutRecordsRequestEntry{
			Data:         doc.Data,
			PartitionKey: aws.String(strconv.FormatInt(doc.ID, 10)),
		})
	}

	_, err := w.client.PutRecordsWithContext(ctx, request)
	if err == nil {
		return nil
	}

	if err, ok := err.(awserr.Error); ok {
		if slices.Contains(retriableErrorCodes, err.Code()) {
			return err
		}
	}

	w.l.Warnf("write batch: %+v", err)
	return nil
}
