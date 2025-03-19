package types

import (
	"database/sql/driver"

	"google.golang.org/protobuf/proto"

	"a.yandex-team.ru/cloud/dataplatform/api/connman"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type ConnectionParams struct {
	*connman.ConnectionParams
	data []byte
}

func (p *ConnectionParams) GetData() []byte {
	return p.data
}

func (p *ConnectionParams) SetData(data []byte) {
	p.data = data
}

func (p *ConnectionParams) GetProtoMessage() proto.Message {
	if p.ConnectionParams == nil {
		p.ConnectionParams = new(connman.ConnectionParams)
	}
	return p.ConnectionParams
}

func (p *ConnectionParams) Value() (driver.Value, error) {
	return p.data, nil
}

func (p *ConnectionParams) Scan(src interface{}) error {
	data, ok := src.([]byte)
	if !ok {
		return xerrors.Errorf(errUnableToCastSourceToBytes, src)
	}

	p.data = data
	return nil
}
