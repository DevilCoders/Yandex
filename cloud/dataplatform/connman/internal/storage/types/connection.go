package types

import (
	"time"

	"google.golang.org/protobuf/types/known/timestamppb"

	"a.yandex-team.ru/cloud/dataplatform/api/connman"
	"a.yandex-team.ru/transfer_manager/go/pkg/fieldmask"
)

const (
	errUnableToCastSourceToBytes = "unable to cast source to bytes: %T"
	errUnableToUnmarshalJSON     = "unable to unmarshal json: %w"
)

type Connection struct {
	ID          string
	FolderID    string     `db:"folder_id"`
	CreatedAt   time.Time  `db:"created_at"`
	UpdatedAt   *time.Time `db:"updated_at"`
	DeletedAt   *time.Time `db:"deleted_at"`
	Name        string
	Description string
	Labels      Labels
	CreatedBy   string `db:"created_by"`
	Params      *ConnectionParams
}

func (c *Connection) GetUpdatedAt() time.Time {
	if c.UpdatedAt != nil {
		return *c.UpdatedAt
	}
	return c.CreatedAt
}

func FromCreateConnectionRequest(request *connman.CreateConnectionRequest, createdBy string) *Connection {
	return &Connection{
		ID:          GenerateConnectionID(),
		FolderID:    request.FolderId,
		CreatedAt:   time.Time{},
		UpdatedAt:   nil,
		DeletedAt:   nil,
		Name:        request.Name,
		Description: request.Description,
		Labels:      request.Labels,
		CreatedBy:   createdBy,
		Params: &ConnectionParams{
			ConnectionParams: request.Params,
			data:             nil,
		},
	}
}

func FromUpdateConnectionRequest(prev *Connection, request *connman.UpdateConnectionRequest) (*Connection, error) {
	dst := prev.ToProto()
	src := connman.Connection{
		Id:          "",
		FolderId:    "",
		CreatedAt:   nil,
		UpdatedAt:   nil,
		Name:        request.Name,
		Description: request.Description,
		Labels:      request.Labels,
		CreatedBy:   "",
		Params:      request.Params,
	}
	err := fieldmask.Copy(dst.ProtoReflect(), src.ProtoReflect(), request.UpdateMask)
	return &Connection{
		ID:          dst.Id,
		FolderID:    prev.FolderID,
		CreatedAt:   prev.CreatedAt,
		UpdatedAt:   prev.UpdatedAt,
		DeletedAt:   prev.DeletedAt,
		Name:        dst.Name,
		Description: dst.Description,
		Labels:      dst.Labels,
		CreatedBy:   prev.CreatedBy,
		Params: &ConnectionParams{
			ConnectionParams: dst.Params,
			data:             nil,
		},
	}, err
}

func (c *Connection) ToProto() *connman.Connection {
	return &connman.Connection{
		Id:          c.ID,
		FolderId:    c.FolderID,
		CreatedAt:   timestamppb.New(c.CreatedAt),
		UpdatedAt:   timestamppb.New(c.GetUpdatedAt()),
		Name:        c.Name,
		Description: c.Description,
		Labels:      c.Labels,
		CreatedBy:   c.CreatedBy,
		Params:      c.Params.ConnectionParams,
	}
}
