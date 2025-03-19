package serialization

import (
	"google.golang.org/protobuf/proto"
)

type Serializable interface {
	GetData() []byte
	SetData(data []byte)
	GetProtoMessage() proto.Message
}
