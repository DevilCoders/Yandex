package espillars

import (
	"encoding/json"

	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/pillars"
)

type Kibana struct {
	Enabled       bool               `json:"enabled"`
	EncryptionKey *pillars.CryptoKey `json:"encryption_key,omitempty"`
	SavedObjects  struct {
		EncryptionKey *pillars.CryptoKey `json:"encryption_key,omitempty"`
	} `json:"saved_objects"`
	Reporting struct {
		EncryptionKey *pillars.CryptoKey `json:"encryption_key,omitempty"`
	} `json:"reporting"`
	Common json.RawMessage `json:"common,omitempty"` // used for adding any configuration params to cluster adhoc
}
