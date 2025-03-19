package gppillars

import (
	"bytes"
	"encoding/json"
	"fmt"

	"github.com/gofrs/uuid"

	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/crypto"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/greenplum"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/greenplum/gpmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/pillars"
	"a.yandex-team.ru/library/go/core/xerrors"
)

const (
	sshBitSize = 4096

	gpadminPwdLen = 16
	monitorPwdLen = 16

	GPAdminUserName = "gpadmin"
	MonitorUserName = "monitor"
)

var SystemUsers = []string{GPAdminUserName, MonitorUserName}

type Cluster struct {
	Data                  ClusterData `json:"data"`
	disallowUnknownFields bool
}

type ClusterData struct {
	Greenplum         GreenplumData          `json:"greenplum"`
	Pool              *gpmodels.PoolerConfig `json:"pool"`
	S3Bucket          string                 `json:"s3_bucket"`
	ClusterPrivateKey pillars.CryptoKey      `json:"cluster_private_key"`
	ShipLogs          *bool                  `json:"ship_logs,omitempty"`
	Billing           *pillars.Billing       `json:"billing,omitempty"`
	Access            Access                 `json:"access"`
	MDBHealth         *pillars.MDBHealth     `json:"mdb_health,omitempty"`

	S3           json.RawMessage `json:"s3,omitempty"`
	PXF          json.RawMessage `json:"pxf,omitempty"`
	Backup       json.RawMessage `json:"backup,omitempty"`
	Config       json.RawMessage `json:"config,omitempty"`
	MVideo       bool            `json:"mvideo,omitempty"`
	Vacuum       json.RawMessage `json:"vacuum,omitempty"`
	LdapAuth     bool            `json:"ldap_auth,omitempty"`
	DbaasCompute json.RawMessage `json:"dbaas_compute,omitempty"`
	LdapSettings json.RawMessage `json:"ldap_settings,omitempty"`
	AddSettings  json.RawMessage `json:"add_settings,omitempty"`
	Gpsync       json.RawMessage `json:"gpsync,omitempty"`

	Unmanaged json.RawMessage `json:"unmanaged,omitempty"`
}

type Access struct {
	WebSQL       bool `json:"web_sql"`
	DataLens     bool `json:"data_lens"`
	DataTransfer bool `json:"data_transfer,omitempty"`
	Serverless   bool `json:"serverless,omitempty"`
}

type GreenplumData struct {
	Users map[string]UserData `json:"users,omitempty"`

	Config        gpmodels.ClusterConfig        `json:"config,omitempty"`
	MasterConfig  gpmodels.ClusterMasterConfig  `json:"master_config,omitempty"`
	SegmentConfig gpmodels.ClusterSegmentConfig `json:"segment_config,omitempty"`
	ReplCert      json.RawMessage               `json:"repl_cert,omitempty"`
	Segments      map[int]SegmentData           `json:"segments,omitempty"`

	MasterHostCount  int64  `json:"master_host_count"`
	SegmentHostCount int64  `json:"segment_host_count"`
	SegmentInHost    int64  `json:"segment_in_host"`
	AdminUserName    string `json:"admin_user_name"`

	SSHKeys json.RawMessage `json:"ssh_keys,omitempty"`

	Unmanaged json.RawMessage `json:"unmanaged,omitempty"`
}

type UserData struct {
	Password pillars.CryptoKey `json:"password"`
	SID      string            `json:"sid"`
	Create   bool              `json:"create"`
}

type SegmentData struct {
	Primary SegmentPlace  `json:"primary"`
	Mirror  *SegmentPlace `json:"mirror"`
}

type SegmentPlace struct {
	Fqdn       string `json:"fqdn"`
	MountPoint string `json:"mount_point"`
}

func NewCluster() *Cluster {
	var pillar Cluster
	pillar.Data.Greenplum.Users = make(map[string]UserData)
	pillar.Data.Greenplum.Segments = make(map[int]SegmentData)
	pillar.Data.Greenplum.Config.SegmentMirroringEnable = true
	pillar.Data.Pool = &gpmodels.PoolerConfig{}
	return &pillar
}

func NewClusterWithVersion(v string) *Cluster {
	c := NewCluster()
	c.Data.Greenplum.Config.Version = v
	return c
}

func (c *Cluster) Validate() error {
	return nil
}

func (c *Cluster) MarshalPillar() (json.RawMessage, error) {
	raw, err := json.Marshal(c)
	if err != nil {
		return nil, xerrors.Errorf("marshal greenplum cluster pillar: %w", err)
	}
	return raw, err
}

func (c *Cluster) DisallowUnknownFields() {
	c.disallowUnknownFields = true
}

func (c *Cluster) UnmarshalPillar(raw json.RawMessage) error {
	dec := json.NewDecoder(bytes.NewReader(raw))

	if c.disallowUnknownFields {
		dec.DisallowUnknownFields()
	}

	if err := dec.Decode(c); err != nil {
		return xerrors.Errorf("unmarshal greenplum cluster pillar: %w", err)
	}
	return nil
}

func (c *Cluster) UpdateUserPassword(password string, cr crypto.Crypto) error {
	username := c.Data.Greenplum.AdminUserName
	var err error
	ud, ok := c.Data.Greenplum.Users[username]
	if !ok {
		return xerrors.Errorf("admin user %q is not present in pillar, can't update password", username)
	}
	ud.Password, err = cr.Encrypt([]byte(password))
	if err != nil {
		return xerrors.Errorf("encrypt user %q password: %w", username, err)
	}
	c.Data.Greenplum.Users[username] = ud
	return nil
}

func (c *Cluster) AddUser(name string, password string, cr crypto.Crypto) error {
	if name == "" {
		return semerr.InvalidInputf("user name is empty")
	}

	for _, dName := range greenplum.SystemUserNames {
		if dName == name {
			return semerr.InvalidInputf("%q is the system user name", name)
		}
	}

	if _, ok := c.Data.Greenplum.Users[name]; ok {
		return semerr.AlreadyExistsf("user %q already exists", name)
	}

	ud := UserData{}
	ud.Create = true
	var err error
	ud.Password, err = cr.Encrypt([]byte(password))
	if err != nil {
		return xerrors.Errorf("encrypt user %q password: %w", name, err)
	}
	sid, err := uuid.NewGen().NewV4()
	if err != nil {
		return xerrors.Errorf("generate uuid: %w", err)
	}
	ud.SID = fmt.Sprintf("0x%x", sid.Bytes())
	c.Data.Greenplum.Users[name] = ud
	return nil
}

func (c *Cluster) GenerateGPAdminUser(cr crypto.Crypto) error {
	password, err := crypto.GenerateEncryptedPassword(cr, gpadminPwdLen, nil)
	if err != nil {
		return err
	}

	ud := UserData{
		Create:   false,
		Password: password,
	}
	sid, err := uuid.NewGen().NewV4()
	if err != nil {
		return xerrors.Errorf("generate uuid: %w", err)
	}
	ud.SID = fmt.Sprintf("0x%x", sid.Bytes())
	c.Data.Greenplum.Users[GPAdminUserName] = ud

	return nil
}

func (c *Cluster) GenerateMonitorUser(cr crypto.Crypto) error {
	password, err := crypto.GenerateEncryptedPassword(cr, monitorPwdLen, nil)
	if err != nil {
		return err
	}

	ud := UserData{
		Create:   true,
		Password: password,
	}
	sid, err := uuid.NewGen().NewV4()
	if err != nil {
		return xerrors.Errorf("generate uuid: %w", err)
	}
	ud.SID = fmt.Sprintf("0x%x", sid.Bytes())
	c.Data.Greenplum.Users[MonitorUserName] = ud

	return nil
}
