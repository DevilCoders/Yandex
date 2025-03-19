package pgpillars

import (
	"encoding/json"

	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/pillars"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type Cluster struct {
	Data ClusterData `json:"data"`
}

var _ pillars.Marshaler = &Cluster{}

func (p *Cluster) Validate() error {
	return nil
}

func (p *Cluster) MarshalPillar() (json.RawMessage, error) {
	raw, err := json.Marshal(p)
	if err != nil {
		return nil, xerrors.Errorf("failed to marshal postgresql cluster pillar: %w", err)
	}

	return raw, err
}

func (p *Cluster) UnmarshalPillar(raw json.RawMessage) error {
	if err := json.Unmarshal(raw, p); err != nil {
		return xerrors.Errorf("failed to unmarshal postgresql cluster pillar: %w", err)
	}

	return nil
}

type ClusterData struct {
	PG                PG                `json:"pg"`
	S3                S3                `json:"s3"`
	Backup            json.RawMessage   `json:"backup"`
	Config            json.RawMessage   `json:"config"`
	PGSync            json.RawMessage   `json:"pgsync"`
	UseWalE           bool              `json:"use_wale"`
	UseWalG           bool              `json:"use_walg"`
	S3Bucket          string            `json:"s3_bucket"`
	DBs               Databases         `json:"unmanaged_dbs"`
	UsePGAASProxy     bool              `json:"use_pgaas_proxy"`
	ClusterPrivateKey pillars.CryptoKey `json:"cluster_private_key"`
}

type PG struct {
	Version Version `json:"version"`
}

type Version struct {
	Major      string `json:"major_num"`
	MajorHuman string `json:"major_human"`
}

type S3 struct {
	GPGKey   pillars.CryptoKey `json:"gpg_key"`
	GPGKeyID string            `json:"gpg_key_id"`
}

// Databases of PG pillar
// Transforms JSON 'cool' representation to manageable data types and back
type Databases map[string]DB

func (d *Databases) UnmarshalJSON(data []byte) error {
	var dbs []map[string]DB
	if err := json.Unmarshal(data, &dbs); err != nil {
		return err
	}

	*d = make(Databases, len(dbs))
	for _, db := range dbs {
		for k, v := range db {
			(*d)[k] = v
		}
	}

	return nil
}

func (d Databases) MarshalJSON() ([]byte, error) {
	var dbs []map[string]DB

	for k, v := range d {
		dbs = append(dbs, map[string]DB{k: v})
	}

	return json.Marshal(dbs)
}

type DB struct {
	User       string   `json:"user"`
	LCCtype    string   `json:"lc_ctype"`
	LCCollate  string   `json:"lc_collate"`
	Extensions []string `json:"extensions"`
}
