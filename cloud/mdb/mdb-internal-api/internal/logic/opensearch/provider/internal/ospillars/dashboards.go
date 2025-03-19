package ospillars

import (
	"encoding/json"
)

type Dashboards struct {
	Enabled bool            `json:"enabled"`
	Common  json.RawMessage `json:"common,omitempty"` // used for adding any configuration params to cluster adhoc
}
