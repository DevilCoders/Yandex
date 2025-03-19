package view

import (
	"context"

	"google.golang.org/protobuf/reflect/protoreflect"

	"a.yandex-team.ru/cloud/dataplatform/api/connman"
	"a.yandex-team.ru/cloud/dataplatform/connman/internal/encryption"
	"a.yandex-team.ru/cloud/dataplatform/connman/internal/protoutil"
	"a.yandex-team.ru/cloud/dataplatform/connman/internal/storage/types"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type Viewer interface {
	PrepareConnectionView(ctx context.Context, connection *types.Connection, view connman.ConnectionView) (*connman.Connection, error)
}

type viewer struct {
	decryptor encryption.LockboxDecryptor
}

func NewViewer(decryptor encryption.LockboxDecryptor) Viewer {
	return &viewer{decryptor: decryptor}
}

func (v *viewer) PrepareConnectionView(ctx context.Context, connection *types.Connection, view connman.ConnectionView) (*connman.Connection, error) {
	connectionProto := connection.ToProto()
	err := v.prepareConnectionParamsView(ctx, connectionProto.Params, view)
	if err != nil {
		return nil, xerrors.Errorf("unable to prepare connection params view: %w", err)
	}
	return connectionProto, nil
}

func (v *viewer) prepareConnectionParamsView(ctx context.Context, params *connman.ConnectionParams, view connman.ConnectionView) error {
	switch view {
	case connman.ConnectionView_CONNECTION_VIEW_UNSPECIFIED, connman.ConnectionView_CONNECTION_VIEW_BASIC:
		protoutil.ClearSensitiveData(params.ProtoReflect())
	case connman.ConnectionView_CONNECTION_VIEW_SENSITIVE:
		err := v.decryptConnectionParams(ctx, params)
		if err != nil {
			return xerrors.Errorf("unable to decrypt connection params password: %w", err)
		}
	default:
		return xerrors.Errorf("unsupported view type: %v", view)
	}
	return nil
}

func (v *viewer) decryptConnectionParams(ctx context.Context, params *connman.ConnectionParams) error {
	passwords := getConnectionParamsPasswords(params)
	for _, x := range passwords {
		switch value := x.Value.(type) {
		case *connman.Password_Raw:
		case *connman.Password_Lockbox:
			raw, err := v.decryptor.Decrypt(ctx, value.Lockbox)
			if err != nil {
				return xerrors.Errorf("unable to decrypt password with lockbox: %w", err)
			}
			x.Value = &connman.Password_Raw{Raw: raw}
		default:
			return xerrors.Errorf("unsupported password type: %T", value)
		}
	}
	return nil
}

func getConnectionParamsPasswords(params *connman.ConnectionParams) []*connman.Password {
	var result []*connman.Password
	var messageCallback protoutil.MessageCallback = func(message protoreflect.Message) bool {
		switch x := message.Interface().(type) {
		case *connman.Password:
			result = append(result, x)
			return false
		}
		return true
	}
	protoutil.Iterate(params.ProtoReflect(), messageCallback)
	return result
}
