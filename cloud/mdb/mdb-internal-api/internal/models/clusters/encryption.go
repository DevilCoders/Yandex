package clusters

import (
	"encoding/json"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
)

type Encryption struct {
	Enabled optional.Bool
	Key     json.RawMessage
}
