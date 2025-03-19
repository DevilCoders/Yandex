package mongopillars

import (
	"encoding/json"

	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/crypto"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/mongodb/mongomodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/pillars"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/slices"
)

type Cluster struct {
	Data ClusterData `json:"data"`
}

var _ pillars.Marshaler = &Cluster{}

func (p Cluster) Validate() error {
	return nil
}

func (p *Cluster) MarshalPillar() (json.RawMessage, error) {
	raw, err := json.Marshal(p)
	if err != nil {
		return nil, xerrors.Errorf("failed to marshal mongodb cluster pillar: %w", err)
	}

	return raw, err
}

func (p *Cluster) UnmarshalPillar(raw json.RawMessage) error {
	if err := json.Unmarshal(raw, p); err != nil {
		return xerrors.Errorf("failed to unmarshal mongodb cluster pillar: %w", err)
	}

	return nil
}

func (p *Cluster) Database(cid, name string) (mongomodels.Database, error) {
	if _, ok := p.Data.MongoDB.DBs[name]; !ok {
		return mongomodels.Database{}, semerr.NotFoundf("database %q not found", name)
	}

	return DatabaseFromPillar(cid, name), nil
}

func (p *Cluster) Databases(cid string) []mongomodels.Database {
	var dbs []mongomodels.Database
	for dbname := range p.Data.MongoDB.DBs {
		dbs = append(dbs, DatabaseFromPillar(cid, dbname))
	}

	return dbs
}

func (p *Cluster) AddDatabase(spec mongomodels.DatabaseSpec) error {
	for name := range p.Data.MongoDB.DBs {
		if name == spec.Name {
			return semerr.AlreadyExistsf("database %q already exists", name)
		}
	}

	p.Data.MongoDB.DBs[spec.Name] = nil
	return nil
}

func (p *Cluster) User(cid, name string) (mongomodels.User, error) {
	userdata, ok := p.Data.MongoDB.Users[name]
	if !ok {
		return mongomodels.User{}, semerr.NotFoundf("user %q not found", name)
	}

	return UserFromPillar(cid, name, userdata), nil
}

func (p *Cluster) Users(cid string, internal bool) []mongomodels.User {
	var users []mongomodels.User
	for name, data := range p.Data.MongoDB.Users {
		if data.Internal && !internal {
			continue
		}
		users = append(users, UserFromPillar(cid, name, data))
	}

	return users
}

func (p *Cluster) CreateUser(spec mongomodels.UserSpec, crypto crypto.Crypto) error {
	for name := range p.Data.MongoDB.Users {
		if name == spec.Name {
			return semerr.AlreadyExistsf("user %q already exists", name)
		}
	}

	var existingDBs []string
	for db := range p.Data.MongoDB.DBs {
		existingDBs = append(existingDBs, db)
	}
	existingDBs = append(existingDBs, mongomodels.SystemDatabases...)

	userPillarDBs := make(map[string][]string)
	for _, perm := range spec.Permissions {
		if !slices.ContainsString(existingDBs, perm.DatabaseName) {
			return semerr.NotFoundf("database %q not found", perm.DatabaseName)
		}
		userPillarDBs[perm.DatabaseName] = perm.Roles
	}

	ud := UserData{
		DBs:      userPillarDBs,
		Internal: false,
		Services: nil, // TODO: set services
	}
	var err error
	ud.Password, err = crypto.Encrypt([]byte(spec.Password.Unmask()))
	if err != nil {
		return xerrors.Errorf("failed to encrypt user %q password: %w", spec.Name, err)
	}
	p.Data.MongoDB.Users[spec.Name] = ud
	return nil
}

func (p *Cluster) DeleteUser(name string) error {
	_, ok := p.Data.MongoDB.Users[name]
	if !ok {
		return semerr.NotFoundf("user %q not found", name)
	}

	delete(p.Data.MongoDB.Users, name)
	return nil
}

type ClusterData struct {
	MongoDB DBMSData `json:"mongodb"`
}

type DBMSData struct {
	DBs   map[string]json.RawMessage `json:"databases"`
	Users map[string]UserData        `json:"users"`
}

type UserData struct {
	DBs      map[string][]string `json:"dbs"`
	Internal bool                `json:"internal"`
	Password pillars.CryptoKey   `json:"password"`
	Services []string            `json:"services"`
}

func DatabaseFromPillar(cid, name string) mongomodels.Database {
	return mongomodels.Database{Name: name, ClusterID: cid}
}

func UserFromPillar(cid, name string, data UserData) mongomodels.User { // TODO: rename PillarToUser?
	var perms []mongomodels.Permission
	for dbname, dbperms := range data.DBs {
		perms = append(perms, mongomodels.Permission{
			DatabaseName: dbname,
			Roles:        dbperms,
		})
	}
	return mongomodels.User{
		ClusterID:   cid,
		Name:        name,
		Permissions: perms,
	}
}
