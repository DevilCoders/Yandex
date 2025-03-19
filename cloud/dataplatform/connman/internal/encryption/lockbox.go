package encryption

import (
	"context"

	"a.yandex-team.ru/cloud/bitbucket/public-api/yandex/cloud/lockbox/v1"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/transfer_manager/go/pkg/yc"
	ycsdk "a.yandex-team.ru/transfer_manager/go/pkg/yc/sdk"
)

type LockboxDecryptor interface {
	Decrypt(ctx context.Context, secretID string) (string, error)
}

type lockboxDecryptor struct {
	client lockbox.PayloadServiceClient
}

func NewLockboxDecryptor(ctx context.Context) (LockboxDecryptor, error) {
	sdk, err := ycsdk.Instance()
	if err != nil {
		return nil, xerrors.Errorf("unable to get yc sdk instance: %w", err)
	}
	client, err := sdk.LockboxPayloadServiceClient(ctx)
	if err != nil {
		return nil, xerrors.Errorf("unable to get lockbox payload service client: %w", err)
	}
	return &lockboxDecryptor{client: client}, nil
}

func (l *lockboxDecryptor) Decrypt(ctx context.Context, secretID string) (string, error) {
	request := &lockbox.GetPayloadRequest{SecretId: secretID}
	payload, err := l.client.Get(yc.WithUserAuth(ctx), request)
	if err != nil {
		return "", xerrors.Errorf("unable to get lockbox secret payload: %w", err)
	}
	if len(payload.Entries) != 1 {
		return "", xerrors.Errorf("payload must have a single entry (actual count is %v)", len(payload.Entries))
	}

	switch value := payload.Entries[0].Value.(type) {
	case *lockbox.Payload_Entry_TextValue:
		return value.TextValue, nil
	case *lockbox.Payload_Entry_BinaryValue:
		return string(value.BinaryValue), nil
	default:
		return "", xerrors.Errorf("unsupported value type: %T", value)
	}
}
