package chpillars

import (
	"encoding/json"

	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/crypto"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/clickhouse/chmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/pillars"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type SubClusterZK struct {
	Data SubClusterZKData `json:"data"`
}

var _ pillars.Marshaler = &SubClusterZK{}

func NewSubClusterZK() *SubClusterZK {
	return &SubClusterZK{
		Data: SubClusterZKData{
			Zookeeper: SubClusterZKServer{
				Users: map[string]ZKUser{},
			},
		},
	}
}

func (zk *SubClusterZK) MarshalPillar() (json.RawMessage, error) {
	raw, err := json.Marshal(zk)
	if err != nil {
		return nil, xerrors.Errorf("failed to marshal zookeeper subcluster pillar: %w", err)
	}

	return raw, err
}

func (zk *SubClusterZK) UnmarshalPillar(raw json.RawMessage) error {
	if err := json.Unmarshal(raw, zk); err != nil {
		return xerrors.Errorf("failed to unmarshal zookeeper subcluster pillar: %w", err)
	}

	return nil
}

type SubClusterZKData struct {
	Zookeeper SubClusterZKServer `json:"zk"`
}

type SubClusterZKServer struct {
	Nodes   map[string]int    `json:"nodes"`
	Users   map[string]ZKUser `json:"users"`
	Version string            `json:"version,omitempty"`
}

type ZKUser struct {
	Password pillars.CryptoKey `json:"password"`
}

func (zk *SubClusterZK) AddNode(fqdn string) (int, error) {
	if _, ok := zk.Data.Zookeeper.Nodes[fqdn]; ok {
		return 0, semerr.FailedPreconditionf("node %q is already in Zookeeper", fqdn)
	}

	if zk.Data.Zookeeper.Nodes == nil {
		zk.Data.Zookeeper.Nodes = make(map[string]int)
	}

	zid := zk.getVacantZid()
	zk.Data.Zookeeper.Nodes[fqdn] = zid
	return zid, nil
}

func (zk *SubClusterZK) DeleteNode(fqdn string) (int, error) {
	if zid, ok := zk.Data.Zookeeper.Nodes[fqdn]; ok {
		delete(zk.Data.Zookeeper.Nodes, fqdn)
		return zid, nil
	}

	return 0, semerr.FailedPreconditionf("unable to find node %q", fqdn)
}

func (zk *SubClusterZK) getVacantZid() int {
	takenZid := map[int]struct{}{}

	for _, zid := range zk.Data.Zookeeper.Nodes {
		takenZid[zid] = struct{}{}
	}

	zid := 1
	for ; true; zid++ {
		if _, ok := takenZid[zid]; !ok {
			break
		}
	}

	return zid
}

func (zk *SubClusterZK) AddUser(cryptoProvider crypto.Crypto, name string, passLen int) error {
	if _, ok := zk.Data.Zookeeper.Users[name]; ok {
		return semerr.AlreadyExistsf("user %s alredy exists", name)
	}

	pass, err := crypto.GenerateEncryptedPassword(cryptoProvider, passLen, nil)
	if err != nil {
		return err
	}

	zk.Data.Zookeeper.Users[name] = ZKUser{Password: pass}
	return nil
}

func (zk *SubClusterZK) AddUsers(cryptoProvider crypto.Crypto) error {
	if err := zk.AddUser(cryptoProvider, chmodels.ZKACLUserSuper, chmodels.PasswordLen); err != nil {
		return err
	}

	if err := zk.AddUser(cryptoProvider, chmodels.ZKACLUserBackup, chmodels.PasswordLen); err != nil {
		return err
	}

	return nil
}
