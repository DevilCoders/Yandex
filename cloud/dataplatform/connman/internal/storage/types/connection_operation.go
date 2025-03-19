package types

import (
	"time"

	"google.golang.org/protobuf/types/known/anypb"
	"google.golang.org/protobuf/types/known/timestamppb"

	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/operation"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type ConnectionOperation struct {
	ID           string
	ConnectionID string     `db:"connection_id"`
	CreatedAt    time.Time  `db:"created_at"`
	UpdatedAt    *time.Time `db:"updated_at"`
	CreatedBy    string     `db:"created_by"`
	Done         bool
	Metadata     *ConnectionOperationMetadata
}

func (o *ConnectionOperation) GetUpdatedAt() time.Time {
	if o.UpdatedAt != nil {
		return *o.UpdatedAt
	}
	return o.CreatedAt
}

func (o *ConnectionOperation) ToProto(connection *Connection) (*operation.Operation, error) {
	metadataProto, err := o.Metadata.ProtoMessage()
	if err != nil {
		return nil, xerrors.Errorf("unable to get metadata proto message: %w", err)
	}

	metadataAny, err := anypb.New(metadataProto)
	if err != nil {
		return nil, xerrors.Errorf("unable to create metadata any proto: %w", err)
	}

	description, err := o.Metadata.Description()
	if err != nil {
		return nil, xerrors.Errorf("unable to get description: %w", err)
	}

	var result *operation.Operation_Response
	if connection != nil {
		connectionProto := connection.ToProto()
		connectionAny, err := anypb.New(connectionProto)
		if err != nil {
			return nil, xerrors.Errorf("unable to create connection any proto: %w", err)
		}
		result = &operation.Operation_Response{Response: connectionAny}
	}

	return &operation.Operation{
		Id:          o.ID,
		Description: description,
		CreatedAt:   timestamppb.New(o.CreatedAt),
		CreatedBy:   o.CreatedBy,
		ModifiedAt:  timestamppb.New(o.GetUpdatedAt()),
		Done:        o.Done,
		Metadata:    metadataAny,
		Result:      result,
	}, nil
}
