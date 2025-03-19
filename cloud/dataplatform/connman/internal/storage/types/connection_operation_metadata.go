package types

import (
	"database/sql/driver"

	"google.golang.org/protobuf/proto"

	"a.yandex-team.ru/cloud/dataplatform/api/connman"
	"a.yandex-team.ru/library/go/core/xerrors"
)

const (
	errUnknownConnectionOperationMetadataType = "unknown connection operation metadata type: %T"
)

type ConnectionOperationMetadata struct {
	*connman.ConnectionOperationMetadata
	data []byte
}

func (m *ConnectionOperationMetadata) GetData() []byte {
	return m.data
}

func (m *ConnectionOperationMetadata) SetData(data []byte) {
	m.data = data
}

func (m *ConnectionOperationMetadata) GetProtoMessage() proto.Message {
	if m.ConnectionOperationMetadata == nil {
		m.ConnectionOperationMetadata = new(connman.ConnectionOperationMetadata)
	}
	return m.ConnectionOperationMetadata
}

func (m *ConnectionOperationMetadata) Value() (driver.Value, error) {
	return m.data, nil
}

func (m *ConnectionOperationMetadata) Scan(src interface{}) error {
	data, ok := src.([]byte)
	if !ok {
		return xerrors.Errorf(errUnableToCastSourceToBytes, src)
	}
	m.data = data
	return nil
}

func (m *ConnectionOperationMetadata) Description() (string, error) {
	switch m.Type.(type) {
	case *connman.ConnectionOperationMetadata_Create:
		return "Create", nil
	case *connman.ConnectionOperationMetadata_Update:
		return "Update", nil
	case *connman.ConnectionOperationMetadata_Delete:
		return "Delete", nil
	default:
		return "", xerrors.Errorf(errUnknownConnectionOperationMetadataType, m.Type)
	}
}

func (m *ConnectionOperationMetadata) ProtoMessage() (proto.Message, error) {
	switch x := m.Type.(type) {
	case *connman.ConnectionOperationMetadata_Create:
		return x.Create, nil
	case *connman.ConnectionOperationMetadata_Update:
		return x.Update, nil
	case *connman.ConnectionOperationMetadata_Delete:
		return x.Delete, nil
	default:
		return nil, xerrors.Errorf(errUnknownConnectionOperationMetadataType, m.Type)
	}
}
