package aws

import (
	"context"
	"os"

	"github.com/aws/aws-sdk-go/aws"
	"github.com/aws/aws-sdk-go/aws/credentials/ec2rolecreds"
	"github.com/aws/aws-sdk-go/aws/ec2metadata"
	"github.com/aws/aws-sdk-go/aws/session"
	"github.com/aws/aws-sdk-go/service/kms"

	"a.yandex-team.ru/cloud/mdb/internal/httputil"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

var (
	KeyType = "aws"
)

type Config struct {
	ID string `json:"id" yaml:"id"`
}

type Key struct {
	name       string
	cipherFile string
	keyID      string
	l          log.Logger
	provider   *kms.KMS
}

func (k *Key) GetKey(ctx context.Context) ([]byte, error) {
	data, err := os.ReadFile(k.cipherFile)
	if err != nil {
		return nil, xerrors.Errorf("read cipherFile file %s: %w", k.cipherFile, err)
	}

	ctx = ctxlog.WithFields(ctx, log.String("cipherFile", k.cipherFile), log.String("keyID", k.keyID))
	ctxlog.Debug(ctx, k.l, "decrypt cipherFile with KMS key")

	input := kms.DecryptInput{
		CiphertextBlob: data,
		KeyId:          aws.String(k.keyID),
	}
	res, err := k.provider.DecryptWithContext(ctx, &input)
	if err != nil {
		return nil, xerrors.Errorf("decrypt cipherFile: %w", err)
	}

	ctxlog.Debug(ctx, k.l, "got decrypted data")
	return res.Plaintext, nil
}

func (k *Key) EncryptKey(ctx context.Context, plaintextKey []byte) error {
	ctx = ctxlog.WithFields(ctx, log.String("cipherFile", k.cipherFile), log.String("keyID", k.keyID))
	ctxlog.Debug(ctx, k.l, "encrypt cipherFile with KMS key")
	input := kms.EncryptInput{Plaintext: plaintextKey, KeyId: aws.String(k.keyID)}
	res, err := k.provider.EncryptWithContext(ctx, &input)
	if err != nil {
		return xerrors.Errorf("encrypt cipherFile: %w", err)
	}

	err = os.WriteFile(k.cipherFile, res.CiphertextBlob, os.FileMode(0600))
	if err != nil {
		return xerrors.Errorf("write encrypted cipherFile: %w", err)
	}

	return nil
}

func (k *Key) Name() string {
	return k.name
}

func NewKey(
	keyName string,
	cipherFile string,
	cfg Config,
	l log.Logger,
	transport httputil.TransportConfig,
) (*Key, error) {
	httpClient, err := httputil.NewClient(httputil.ClientConfig{Name: "mdb-disklock", Transport: transport}, l)
	if err != nil {
		return nil, xerrors.Errorf("create http client: %w", err)
	}

	awsCfg := aws.NewConfig().WithHTTPClient(httpClient.Client)
	sess, err := session.NewSession(awsCfg)
	if err != nil {
		return nil, xerrors.Errorf("create aws session: %w", err)
	}

	creds := ec2rolecreds.NewCredentials(sess)

	metadata := ec2metadata.New(sess)
	region, err := metadata.Region()
	if err != nil {
		return nil, xerrors.Errorf("get aws region: %w", err)
	}

	provider := kms.New(sess, awsCfg.WithCredentials(creds).WithRegion(region))

	return &Key{
		name:       keyName,
		cipherFile: cipherFile,
		keyID:      cfg.ID,
		l:          l,
		provider:   provider,
	}, nil
}
