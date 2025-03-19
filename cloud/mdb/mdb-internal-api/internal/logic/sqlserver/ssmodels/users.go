package ssmodels

import (
	"fmt"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/secret"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/valid"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
)

type DatabaseRoleType string

const (
	DatabaseRoleUnspecified    DatabaseRoleType = ""
	DatabaseRoleOwner          DatabaseRoleType = "db_owner"
	DatabaseRoleSecurityAdmin  DatabaseRoleType = "db_securityadmin"
	DatabaseRoleAccessAdmin    DatabaseRoleType = "db_accessadmin"
	DatabaseRoleBackupOperator DatabaseRoleType = "db_backupoperator"
	DatabaseRoleDDLAdmin       DatabaseRoleType = "db_ddladmin"
	DatabaseRoleDataWriter     DatabaseRoleType = "db_datawriter"
	DatabaseRoleDataReader     DatabaseRoleType = "db_datareader"
	DatabaseRoleDenyDataWriter DatabaseRoleType = "db_denydatawriter"
	DatabaseRoleDenyDataReader DatabaseRoleType = "db_denydatareader"
)

type ServerRoleType string

const (
	ServerRoleUnspecified ServerRoleType = ""
	ServerRoleMdbMonitor  ServerRoleType = "mdb_monitor"
)

type OptionalServerRoles struct {
	ServerRoles []ServerRoleType
	Valid       bool
}

func NewOptionalServerRoles(roles []ServerRoleType) OptionalServerRoles {
	return OptionalServerRoles{ServerRoles: roles, Valid: true}
}

func (r *OptionalServerRoles) Set(roles []ServerRoleType) {
	r.ServerRoles = roles
	r.Valid = true
}

func (r *OptionalServerRoles) Get() ([]ServerRoleType, error) {
	if !r.Valid {
		return nil, optional.ErrMissing
	}
	return r.ServerRoles, nil
}

const (
	SQLServerUserPasswordPattern = `^[^\0\'\"\n\r\t\x1A\\%]*$`
)

var UserNameBlackList = []string{
	"sa",
	"public",
	"sysadmin",
	"securityadmin",
	"serveradmin",
	"setupadmin",
	"processadmin",
	"diskadmin",
	"dbcreator",
	"bulkadmin",
	"AG_LOGIN",
	"NT AUTHORITY\\SYSTEM",
	"NT Service\\*",
	"mdb_monitor",
}

var (
	UserNameValidator     = models.MustUserNameValidator(models.DefaultUserNamePattern, UserNameBlackList)
	UserPasswordValidator = models.MustUserPasswordValidator(SQLServerUserPasswordPattern)
)

func NewSystemsUsersValidator() valid.StringBlacklist {
	return valid.StringBlacklist{Blacklist: UserNameBlackList, Msg: "Invalid user name"}
}

type User struct {
	ClusterID   string
	Name        string
	Permissions []Permission
	ServerRoles []ServerRoleType
}

type UserSpec struct {
	Name        string
	Password    secret.String
	Permissions []Permission
	ServerRoles []ServerRoleType
}
type Permission struct {
	DatabaseName string
	Roles        []DatabaseRoleType
}

// OptionalPermissions holds []Permission value
type OptionalPermissions struct {
	Permissions []Permission
	Valid       bool
}

// NewStrings creates new []string value
func NewOptionalPermissions(v []Permission) OptionalPermissions {
	return OptionalPermissions{Permissions: v, Valid: true}
}

// Set value
func (o *OptionalPermissions) Set(v []Permission) {
	o.Permissions = v
	o.Valid = true
}

// Get value if its set or return an error
func (o *OptionalPermissions) Get() ([]Permission, error) {
	if !o.Valid {
		return nil, optional.ErrMissing
	}

	return o.Permissions, nil
}

func (p Permission) Validate() error {
	roles := make(map[DatabaseRoleType]struct{})
	for _, r := range p.Roles {
		if _, ok := roles[r]; ok {
			return semerr.InvalidInput(fmt.Sprintf("duplicate role %s", r))
		}
	}
	return nil
}
