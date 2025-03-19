package mongomodels

import (
	"a.yandex-team.ru/cloud/mdb/internal/secret"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/slices"
)

var (
	SystemUsers  = []string{"admin", "monitor", "root"}
	ValidDBRoles = map[string][]string{
		"local":  {},
		"config": {},
		"admin":  {"mdbMonitor", "mdbShardingManager"},
		"*":      {"read", "readWrite", "mdbDbAdmin"},
	}
	DefaultDBRoles = map[string][]string{
		"local":        {},
		"config":       {},
		"admin":        {},
		"mdb_internal": {},
		"*":            {"readWrite"},
	}
)

type User struct {
	ClusterID   string
	Name        string
	Permissions []Permission
}

type UserSpec struct {
	Name        string
	Password    secret.String
	Permissions []Permission
}

type Permission struct {
	DatabaseName string
	Roles        []string
}

func (us *UserSpec) Validate() error {
	if err := ValidateUserName(us.Name); err != nil {
		return err
	}

	var dbs []string
	for _, perm := range us.Permissions {
		if err := validateUserPermission(perm); err != nil {
			return err
		}
		if slices.ContainsString(dbs, perm.DatabaseName) {
			return xerrors.Errorf("User '%s' access to the database '%s' is already defined", us.Name,
				perm.DatabaseName)
		}

		dbs = append(dbs, perm.DatabaseName)
	}

	return nil
}

func (us *UserSpec) SetDefaultPermissions(dbs []Database) {
	var perms []Permission
	for _, db := range dbs {
		defaultRoles, ok := DefaultDBRoles[db.Name]
		if !ok && !isSystemDatabase(db.Name) {
			defaultRoles = ValidDBRoles["*"]
		}

		perm := Permission{
			DatabaseName: db.Name,
			Roles:        defaultRoles,
		}
		perms = append(perms, perm)
	}

	us.Permissions = perms
}

func ValidateUserName(name string) error {
	if isSystemUser(name) {
		return xerrors.Errorf("invalid user name %q", name)
	}

	// TODO: validate via regexp and string len
	// TODO: create and use traits for validation
	return nil
}

func validateUserPermission(permission Permission) error {
	// Roles were not provided: later in code we set default perms to readWrite.
	// Check if this DB is not among a list of system databases to prevent an elevated access.

	dbname := permission.DatabaseName
	roles := permission.Roles
	if roles == nil {
		return xerrors.Errorf("invalid permission combination: empty roles list for DB '%s'", dbname)
	}

	validRoles, ok := ValidDBRoles[dbname]
	if !ok && !isSystemDatabase(dbname) {
		validRoles = ValidDBRoles["*"]
	}
	for _, role := range roles {
		if !slices.ContainsString(validRoles, role) {
			return xerrors.Errorf("invalid permission combination: cannot assign '%s' to DB '%s'",
				role, dbname)
		}
	}
	return nil
}

func isSystemUser(name string) bool {
	return slices.ContainsString(SystemUsers, name)
}
