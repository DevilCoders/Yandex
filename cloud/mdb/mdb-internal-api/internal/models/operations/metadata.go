package operations

import (
	"github.com/golang/protobuf/proto"
)

// Metadata interface for types representing operations metadata
type Metadata interface {
	// Build proto message from metadata
	Build(op Operation) proto.Message
}
