package sspillars

import (
	"encoding/json"
	"fmt"
	"sort"

	"github.com/gofrs/uuid"
	"github.com/valyala/fastjson"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/x/encoding/versionedjson"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/crypto"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/sqlserver"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/sqlserver/ssmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/pillars"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/slices"
)

const (
	SysadminUserName      = "sa"
	SysadminSID           = "0x01"
	AdministratorUserName = "Administrator"
)

type Cluster struct {
	Data ClusterData `json:"data"`
}

type ClusterData struct {
	S3                *S3                `json:"s3,omitempty"`
	Access            AccessData         `json:"access"`
	Windows           WindowsData        `json:"windows"`
	SQLServer         SQLServerData      `json:"sqlserver"`
	S3Bucket          string             `json:"s3_bucket"`
	ClusterPrivateKey pillars.CryptoKey  `json:"cluster_private_key"`
	ShipLogs          *bool              `json:"ship_logs,omitempty"`
	Billing           *pillars.Billing   `json:"billing,omitempty"`
	MDBHealth         *pillars.MDBHealth `json:"mdb_health,omitempty"`
	ServiceAccountID  string             `json:"service_account_id,omitempty"`
}

var _ pillars.Marshaler = &Cluster{}

func (c *Cluster) Validate() error {
	return nil
}

func (c *Cluster) MarshalPillar() (json.RawMessage, error) {
	raw, err := json.Marshal(c)
	if err != nil {
		return nil, xerrors.Errorf("failed to marshal sqlserver cluster pillar: %w", err)
	}
	return raw, err
}

func (c *Cluster) UnmarshalPillar(raw json.RawMessage) error {
	if err := versionedjson.Parse(raw, c, map[string]versionedjson.ParserFunc{"sqlserverconfig": sqlServerConfigParser}); err != nil {
		return xerrors.Errorf("unmarshal sqlserver cluster pillar: %w", err)
	}
	return nil
}

type S3 struct {
	GPGKey   pillars.CryptoKey `json:"gpg_key"`
	GPGKeyID string            `json:"gpg_key_id"`
}

type AccessData struct {
	DataLens     bool `json:"data_lens"`
	WebSQL       bool `json:"web_sql"`
	DataTransfer bool `json:"data_transfer"`
}

type WindowsData struct {
	Users map[string]OSUserData `json:"users,omitempty"`
}

type OSUserData struct {
	Password pillars.CryptoKey `json:"password"`
}

type SQLServerData struct {
	Version            VersionData              `json:"version" version:"version,sqlserverconfig,allow_missing"`
	Databases          map[string]DatabaseData  `json:"databases,omitempty"`
	Users              map[string]UserData      `json:"users,omitempty"`
	Config             ssmodels.SQLServerConfig `json:"config,omitempty" version:"versioned,sqlserverconfig,allow_missing"`
	ReplCert           json.RawMessage          `json:"repl_cert,omitempty"`
	SQLCollation       string                   `json:"sqlcollation,omitempty"`
	UnreadableReplicas bool                     `json:"unreadable_replicas"`
}

func sqlServerConfigParser(version interface{}, d *fastjson.Value, dst interface{}, parsers versionedjson.Parsers, subparser versionedjson.SubParserFunc) (interface{}, error) {
	v := version.(VersionData)
	switch v.MajorHuman {
	// For possible extension, may be we should use "contains in ssmodels.Versions"
	case ssmodels.Version2016sp2dev, ssmodels.Version2016sp2std, ssmodels.Version2016sp2ent,
		ssmodels.Version2017dev, ssmodels.Version2017std, ssmodels.Version2017ent,
		ssmodels.Version2019dev, ssmodels.Version2019std, ssmodels.Version2019ent:
		var conf ssmodels.ConfigBase
		if d == nil {
			return &conf, nil
		}

		return subparser(d, &conf, parsers)
	default:
		return nil, xerrors.Errorf("unknown version %+v", v)
	}
}

type VersionData struct {
	Major      string `json:"major_num"`
	MajorHuman string `json:"major_human"`
	Edition    string `json:"edition"`
}

type DatabaseData struct {
	// db settings to be done
}

type UserData struct {
	Password    pillars.CryptoKey           `json:"password"`
	SID         string                      `json:"sid"`
	Databases   map[string]UserDatabaseData `json:"dbs,omitempty"`
	ServerRoles []ssmodels.ServerRoleType   `json:"server_roles,omitempty"`
}

type UserDatabaseData struct {
	Roles []ssmodels.DatabaseRoleType `json:"roles,omitempty"`
}

func NewCluster() *Cluster {
	var pillar Cluster
	pillar.Data.SQLServer.Databases = make(map[string]DatabaseData)
	pillar.Data.SQLServer.Users = make(map[string]UserData)
	pillar.Data.Windows.Users = make(map[string]OSUserData)
	return &pillar
}

func NewClusterWithVersion(v VersionData) *Cluster {
	c := NewCluster()
	c.Data.SQLServer.Version = v
	return c
}

func ParseCluster(raw json.RawMessage) (*Cluster, error) {
	var p Cluster
	if err := json.Unmarshal(raw, &p); err != nil {
		return nil, xerrors.Errorf("failed to parse sqlserver pillar: %w", err)
	}

	return &p, nil
}

func (c *Cluster) AddDatabase(ds ssmodels.DatabaseSpec) error {
	err := ds.Validate()
	if err != nil {
		return err
	}
	_, ok := c.Data.SQLServer.Databases[ds.Name]
	if ok {
		return semerr.AlreadyExistsf("database %q already exists", ds.Name)
	}
	c.Data.SQLServer.Databases[ds.Name] = databaseDataFromSpec(ds)
	return nil
}

func (c *Cluster) DeleteDatabase(dbname string) error {
	// do not delete system databases
	if err := (&ssmodels.DatabaseSpec{
		Name: dbname,
	}).Validate(); err != nil {
		return err
	}

	if _, ok := c.Data.SQLServer.Databases[dbname]; !ok {
		return semerr.NotFoundf("database %q not found", dbname)
	}

	for usrName, usr := range c.Data.SQLServer.Users {
		if _, ok := usr.Databases[dbname]; ok {
			delete(c.Data.SQLServer.Users[usrName].Databases, dbname)
		}
	}

	delete(c.Data.SQLServer.Databases, dbname)
	return nil
}

func (c *Cluster) DatabaseNames() []string {
	var dbnames []string
	for name := range c.Data.SQLServer.Databases {
		dbnames = append(dbnames, name)
	}
	return dbnames
}

func (c *Cluster) CreateSystemUsers(cp crypto.Crypto) error {
	password, err := crypto.GenerateEncryptedPassword(cp, crypto.PasswordDefaultLegth, nil)
	if err != nil {
		return err
	}
	c.Data.SQLServer.Users[SysadminUserName] = UserData{
		Password:  password,
		SID:       SysadminSID,
		Databases: make(map[string]UserDatabaseData),
	}

	admPassword, err := crypto.GenerateEncryptedPassword(cp, crypto.PasswordDefaultLegth, nil)
	if err != nil {
		return err
	}
	c.Data.Windows.Users[AdministratorUserName] = OSUserData{
		Password: admPassword,
	}
	return nil
}

func (c *Cluster) AddUser(us ssmodels.UserSpec, cr crypto.Crypto) error {
	err := sqlserver.UserArgsFromUserSpec(us).Validate(c.DatabaseNames())
	if err != nil {
		return err
	}
	_, ok := c.Data.SQLServer.Users[us.Name]
	if ok {
		return semerr.AlreadyExistsf("user %q already exists", us.Name)
	}
	ud := userDataFromSpec(us)
	ud.Password, err = cr.Encrypt([]byte(us.Password.Unmask()))
	if err != nil {
		return xerrors.Errorf("failed to encrypt user %q password: %w", us.Name, err)
	}
	sid, err := uuid.NewGen().NewV4()
	if err != nil {
		return xerrors.Errorf("failed to generate user sid: %w", err)
	}
	ud.SID = fmt.Sprintf("0x%x", sid.Bytes())
	c.Data.SQLServer.Users[us.Name] = ud
	return nil
}

func (c *Cluster) DeleteUser(name string) error {
	if _, ok := c.Data.SQLServer.Users[name]; !ok {
		return semerr.NotFoundf("user %q not found", name)
	}
	// do not delete system users
	if err := (&sqlserver.UserArgs{
		Name: name,
	}).Validate(c.DatabaseNames()); err != nil {
		return err
	}
	// simply erase from pillar
	delete(c.Data.SQLServer.Users, name)
	return nil
}

func (c *Cluster) UpdateUser(cr crypto.Crypto, args sqlserver.UserArgs) error {
	if _, ok := c.Data.SQLServer.Users[args.Name]; !ok {
		return semerr.NotFoundf("user %q not found", args.Name)
	}
	// do not update system users
	if err := args.Validate(c.DatabaseNames()); err != nil {
		return err
	}

	userData := c.Data.SQLServer.Users[args.Name]

	if args.Permissions.Valid {
		permissions, err := args.Permissions.Get()
		if err != nil {
			return err
		}
		if userData.Databases == nil {
			userData.Databases = make(map[string]UserDatabaseData)
		}
		// delete all old permissions
		userData.Databases = map[string]UserDatabaseData{}
		for _, p := range permissions {
			userData.Databases[p.DatabaseName] = UserDatabaseData{
				Roles: p.Roles,
			}
		}
	}

	if args.ServerRoles.Valid {
		roles, err := args.ServerRoles.Get()
		if err != nil {
			return err
		}
		userData.ServerRoles = roles
	}

	if args.Password.Valid {
		passwd, err := args.Password.Get()
		if err != nil {
			return err
		}
		userData.Password, err = cr.Encrypt([]byte(passwd.Unmask()))
		if err != nil {
			return xerrors.Errorf("failed to encrypt user %q password: %w", args.Name, err)
		}
	}

	c.Data.SQLServer.Users[args.Name] = userData
	return nil
}

func (c *Cluster) GrantPermissions(username string, grantPermission ssmodels.Permission, cr crypto.Crypto) error {
	if _, ok := c.Data.SQLServer.Users[username]; !ok {
		return semerr.NotFoundf("user %q not found", username)
	}

	targetDB := grantPermission.DatabaseName
	if _, ok := c.Data.SQLServer.Databases[targetDB]; !ok {
		return semerr.InvalidInput(fmt.Sprintf("permission refers not existing database: %s", targetDB))
	}
	if _, ok := c.Data.SQLServer.Users[username].Databases[targetDB]; !ok {
		c.Data.SQLServer.Users[username].Databases[targetDB] = UserDatabaseData{}
	}
	permissions := make([]ssmodels.Permission, 0, len(c.Data.SQLServer.Users[username].Databases))

	for currentDB, currentPermissions := range c.Data.SQLServer.Users[username].Databases {

		if currentDB == targetDB {
			permissions = append(permissions, ssmodels.Permission{
				DatabaseName: targetDB,
				Roles:        append(c.Data.SQLServer.Users[username].Databases[targetDB].Roles, grantPermission.Roles...),
			})
			continue
		}
		permissions = append(permissions, ssmodels.Permission{
			DatabaseName: currentDB,
			Roles:        currentPermissions.Roles,
		})
	}

	updArgs := sqlserver.UserArgs{
		Name:     username,
		Password: optional.OptionalPassword{},
		Permissions: ssmodels.OptionalPermissions{
			Permissions: permissions,
			Valid:       true,
		},
	}

	return c.UpdateUser(cr, updArgs)
}

func (c *Cluster) RevokePermissions(username string, revokePermission ssmodels.Permission, cr crypto.Crypto) error {
	if _, ok := c.Data.SQLServer.Users[username]; !ok {
		return semerr.NotFoundf("user %q not found", username)
	}

	targetDB := revokePermission.DatabaseName
	if _, ok := c.Data.SQLServer.Users[username].Databases[targetDB]; !ok {
		return semerr.InvalidInput(fmt.Sprintf("permission refers not existing database: %s", targetDB))
	}

	permissions := make([]ssmodels.Permission, 0, len(c.Data.SQLServer.Users[username].Databases))

	for currentDB, currentPermissions := range c.Data.SQLServer.Users[username].Databases {
		if currentDB == targetDB {

			rolesStr := make([]string, 0, len(revokePermission.Roles))
			for _, r := range revokePermission.Roles {
				rolesStr = append(rolesStr, string(r))
			}

			newRoles := make([]ssmodels.DatabaseRoleType, 0)
			for _, p := range c.Data.SQLServer.Users[username].Databases[targetDB].Roles {
				if found := slices.ContainsString(rolesStr, string(p)); found {
					continue
				}
				newRoles = append(newRoles, p)
			}

			permissions = append(permissions, ssmodels.Permission{
				DatabaseName: targetDB,
				Roles:        newRoles,
			})
			continue
		}
		permissions = append(permissions, ssmodels.Permission{
			DatabaseName: currentDB,
			Roles:        currentPermissions.Roles,
		})
	}

	updArgs := sqlserver.UserArgs{
		Name:     username,
		Password: optional.OptionalPassword{},
		Permissions: ssmodels.OptionalPermissions{
			Permissions: permissions,
			Valid:       true,
		},
	}

	return c.UpdateUser(cr, updArgs)
}

func userPermissionsFromPillar(mp map[string]UserDatabaseData) []ssmodels.Permission {
	res := make([]ssmodels.Permission, 0, len(mp))
	dbs := make([]string, 0, len(mp))

	for dbName := range mp {
		dbs = append(dbs, dbName)
	}
	sort.Strings(dbs)
	for _, dbName := range dbs {
		res = append(res, ssmodels.Permission{
			DatabaseName: dbName,
			Roles:        mp[dbName].Roles,
		})
	}
	return res
}

func userFromPillar(cid string, userName string, user UserData) ssmodels.User {
	return ssmodels.User{
		ClusterID:   cid,
		Name:        userName,
		Permissions: userPermissionsFromPillar(user.Databases),
		ServerRoles: user.ServerRoles,
	}
}

func (c *Cluster) User(cid, name string) (ssmodels.User, error) {
	if ud, ok := c.Data.SQLServer.Users[name]; ok {
		return userFromPillar(cid, name, ud), nil
	}
	return ssmodels.User{}, semerr.NotFoundf("user %q not found", name)
}

func (c *Cluster) Users(cid string) []ssmodels.User {
	users := make([]ssmodels.User, 0, len(c.Data.SQLServer.Users))
	for userName, user := range c.Data.SQLServer.Users {
		users = append(users, userFromPillar(cid, userName, user))
	}

	sort.Slice(users, func(i, j int) bool {
		return users[i].Name < users[j].Name
	})

	return users
}

func databaseFromPillar(cid string, userName string, dd DatabaseData) ssmodels.Database {
	return ssmodels.Database{
		ClusterID: cid,
		Name:      userName,
	}
}

func (c *Cluster) Database(cid, dbname string) (ssmodels.Database, error) {
	if dd, ok := c.Data.SQLServer.Databases[dbname]; ok {
		return databaseFromPillar(cid, dbname, dd), nil
	}
	return ssmodels.Database{}, semerr.NotFoundf("database %q not found", dbname)
}

func (c *Cluster) Databases(cid string) []ssmodels.Database {
	var databases []ssmodels.Database

	for dbname, dd := range c.Data.SQLServer.Databases {
		databases = append(databases, databaseFromPillar(cid, dbname, dd))
	}

	sort.Slice(databases, func(i, j int) bool {
		return databases[i].Name < databases[j].Name
	})

	return databases
}

func databaseDataFromSpec(ds ssmodels.DatabaseSpec) DatabaseData {
	return DatabaseData{}
}

func userDataFromSpec(us ssmodels.UserSpec) UserData {
	ud := UserData{
		Databases: make(map[string]UserDatabaseData),
	}
	for _, ps := range us.Permissions {
		ud.Databases[ps.DatabaseName] = UserDatabaseData{
			Roles: ps.Roles,
		}
	}
	ud.ServerRoles = append(ud.ServerRoles, us.ServerRoles...)
	return ud
}
