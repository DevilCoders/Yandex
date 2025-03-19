package sspillars

import (
	"encoding/json"
	"testing"

	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/secret"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/crypto/mocks"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/sqlserver"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/sqlserver/ssmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/pillars"
)

const (
	testBaseDB     = "test_base_db"
	testBaseDB2    = "test_base_db2"
	testUsr        = "test_user"
	testPasswdData = "test_crypto_data"
)

func TestSqlServer_PillarAddUser(t *testing.T) {
	us := ssmodels.UserSpec{
		Name: testUsr,
		Permissions: []ssmodels.Permission{
			{
				DatabaseName: testBaseDB,
				Roles: []ssmodels.DatabaseRoleType{
					ssmodels.DatabaseRoleSecurityAdmin,
				},
			},
		},
		Password: secret.NewString("some_random_string"),
	}

	ctrl := gomock.NewController(t)
	cryptoMock := mocks.NewMockCrypto(ctrl)

	cryptoMock.EXPECT().Encrypt(gomock.Any())

	pillar := Cluster{
		Data: ClusterData{
			SQLServer: SQLServerData{
				Databases: map[string]DatabaseData{
					testBaseDB: {},
				},
				Users: map[string]UserData{},
			},
		},
	}

	err := pillar.AddUser(us, cryptoMock)

	require.NoError(t, err)
	require.Contains(t, pillar.Data.SQLServer.Users, us.Name)
}

func TestSqlServer_PillarAddExistingUser_Fails(t *testing.T) {
	us := ssmodels.UserSpec{
		Name: testUsr,
		Permissions: []ssmodels.Permission{
			{
				DatabaseName: testBaseDB,
				Roles: []ssmodels.DatabaseRoleType{
					ssmodels.DatabaseRoleSecurityAdmin,
				},
			},
		},
		Password: secret.NewString("some_random_string"),
	}

	ctrl := gomock.NewController(t)
	cryptoMock := mocks.NewMockCrypto(ctrl)

	pillar := Cluster{
		Data: ClusterData{
			SQLServer: SQLServerData{
				Users: map[string]UserData{
					testBaseDB: {},
				},
			},
		},
	}

	require.Error(t, pillar.AddUser(us, cryptoMock))
}

func TestSqlServer_PillarDeleteUser(t *testing.T) {
	pillar := Cluster{
		Data: ClusterData{
			SQLServer: SQLServerData{
				Databases: map[string]DatabaseData{
					testBaseDB: {},
				},
				Users: map[string]UserData{
					testUsr: {},
				},
			},
		},
	}

	err := pillar.DeleteUser(testUsr)

	require.NoError(t, err)
	require.NotContains(t, pillar.Data.SQLServer.Users, testUsr)
}

func TestSqlServer_PillarDeleteNonExistingUser_Fails(t *testing.T) {
	pillar := Cluster{
		Data: ClusterData{
			SQLServer: SQLServerData{
				Databases: map[string]DatabaseData{
					testBaseDB: {},
				},
				Users: map[string]UserData{},
			},
		},
	}

	err := pillar.DeleteUser(testUsr)

	require.Error(t, err)
}

func TestSqlServer_UpdateUser(t *testing.T) {
	newRoles := []ssmodels.DatabaseRoleType{
		ssmodels.DatabaseRoleDDLAdmin,
		ssmodels.DatabaseRoleDataWriter,
	}
	newRoles2 := []ssmodels.DatabaseRoleType{
		ssmodels.DatabaseRoleSecurityAdmin,
	}

	upUserArgs := sqlserver.UserArgs{
		Name:     testUsr,
		Password: optional.NewOptionalPassword(secret.NewString("myNewAmazingPasswd")),
		Permissions: ssmodels.OptionalPermissions{
			Permissions: []ssmodels.Permission{
				{
					DatabaseName: testBaseDB,
					Roles:        newRoles,
				},
				{
					DatabaseName: testBaseDB2,
					Roles:        newRoles2,
				},
			},
			Valid: true,
		},
	}

	pillar := Cluster{
		Data: ClusterData{
			SQLServer: SQLServerData{
				Databases: map[string]DatabaseData{
					testBaseDB:  {},
					testBaseDB2: {},
				},
				Users: map[string]UserData{
					testUsr: {
						Databases: map[string]UserDatabaseData{
							testBaseDB: {
								Roles: []ssmodels.DatabaseRoleType{
									ssmodels.DatabaseRoleDenyDataWriter,
									ssmodels.DatabaseRoleAccessAdmin,
									ssmodels.DatabaseRoleDDLAdmin,
								},
							},
						},
						Password: pillars.CryptoKey{Data: testPasswdData},
					},
				},
			},
		},
	}

	ctrl := gomock.NewController(t)
	cryptoMock := mocks.NewMockCrypto(ctrl)

	cryptoMock.EXPECT().Encrypt(gomock.Any())
	err := pillar.UpdateUser(cryptoMock, upUserArgs)

	require.NoError(t, err)
	require.Equal(t, newRoles, pillar.Data.SQLServer.Users[testUsr].Databases[testBaseDB].Roles)
	require.Equal(t, newRoles2, pillar.Data.SQLServer.Users[testUsr].Databases[testBaseDB2].Roles)
}

func TestSqlServer_UpdateUser_OnlyPassword(t *testing.T) {

	oldRoles := []ssmodels.DatabaseRoleType{
		ssmodels.DatabaseRoleDenyDataWriter,
		ssmodels.DatabaseRoleAccessAdmin,
		ssmodels.DatabaseRoleDDLAdmin,
	}

	pillar := Cluster{
		Data: ClusterData{
			SQLServer: SQLServerData{
				Databases: map[string]DatabaseData{
					testBaseDB: {},
				},
				Users: map[string]UserData{
					testUsr: {
						Databases: map[string]UserDatabaseData{
							testBaseDB: {Roles: oldRoles},
						},
						Password: pillars.CryptoKey{Data: testPasswdData},
					},
				},
			},
		},
	}

	upUserArgs := sqlserver.UserArgs{
		Name:        testUsr,
		Password:    optional.NewOptionalPassword(secret.NewString("coolsecurepasswd")),
		Permissions: ssmodels.OptionalPermissions{},
	}

	ctrl := gomock.NewController(t)
	cryptoMock := mocks.NewMockCrypto(ctrl)

	cryptoMock.EXPECT().Encrypt(gomock.Any())
	err := pillar.UpdateUser(cryptoMock, upUserArgs)

	require.NotEqual(t, testPasswdData, pillar.Data.SQLServer.Users[testUsr].Password.Data)
	require.NoError(t, err)
}

func TestSqlServer_UpdateUser_OnlyPassword_FailsWithIncorrectPassword(t *testing.T) {
	oldRoles := []ssmodels.DatabaseRoleType{
		ssmodels.DatabaseRoleDenyDataWriter,
		ssmodels.DatabaseRoleAccessAdmin,
		ssmodels.DatabaseRoleDDLAdmin,
	}

	pillar := Cluster{
		Data: ClusterData{
			SQLServer: SQLServerData{
				Databases: map[string]DatabaseData{
					testBaseDB: {},
				},
				Users: map[string]UserData{
					testUsr: {
						Databases: map[string]UserDatabaseData{
							testBaseDB: {Roles: oldRoles},
						},
						Password: pillars.CryptoKey{Data: testPasswdData},
					},
				},
			},
		},
	}

	upUserArgs := sqlserver.UserArgs{
		Name:        testUsr,
		Password:    optional.NewOptionalPassword(secret.NewString("12")),
		Permissions: ssmodels.OptionalPermissions{},
	}

	ctrl := gomock.NewController(t)
	cryptoMock := mocks.NewMockCrypto(ctrl)

	require.Error(t, pillar.UpdateUser(cryptoMock, upUserArgs))
	require.Equal(t, testPasswdData, pillar.Data.SQLServer.Users[testUsr].Password.Data)
}

func TestSqlServer_UpdateUser_OnlyPermission(t *testing.T) {
	oldRoles := []ssmodels.DatabaseRoleType{
		ssmodels.DatabaseRoleDenyDataWriter,
		ssmodels.DatabaseRoleAccessAdmin,
		ssmodels.DatabaseRoleDDLAdmin,
	}

	pillar := Cluster{
		Data: ClusterData{
			SQLServer: SQLServerData{
				Databases: map[string]DatabaseData{
					testBaseDB: {},
				},
				Users: map[string]UserData{
					testUsr: {
						Databases: map[string]UserDatabaseData{
							testBaseDB: {Roles: oldRoles},
						},
						Password: pillars.CryptoKey{Data: testPasswdData},
					},
				},
			},
		},
	}

	newRoles := []ssmodels.DatabaseRoleType{
		ssmodels.DatabaseRoleDDLAdmin,
		ssmodels.DatabaseRoleDataWriter,
	}

	upUserArgs := sqlserver.UserArgs{
		Name: testUsr,
		Permissions: ssmodels.OptionalPermissions{
			Permissions: []ssmodels.Permission{{DatabaseName: testBaseDB, Roles: newRoles}},
			Valid:       true,
		},
	}

	ctrl := gomock.NewController(t)
	cryptoMock := mocks.NewMockCrypto(ctrl)

	require.NoError(t, pillar.UpdateUser(cryptoMock, upUserArgs))
	require.Equal(t, newRoles, pillar.Data.SQLServer.Users[testUsr].Databases[testBaseDB].Roles)
	require.Equal(t, testPasswdData, pillar.Data.SQLServer.Users[testUsr].Password.Data)
}

func TestSqlServer_UpdateNonExistingUser_Fails(t *testing.T) {
	upUserArgs := sqlserver.UserArgs{
		Name:     "NoSuchUsr",
		Password: optional.NewOptionalPassword(secret.NewString("myNewAmazingPasswd")),
		Permissions: ssmodels.OptionalPermissions{
			Permissions: []ssmodels.Permission{},
			Valid:       true,
		},
	}

	pillar := Cluster{
		Data: ClusterData{
			SQLServer: SQLServerData{
				Databases: map[string]DatabaseData{
					testBaseDB: {},
				},
				Users: map[string]UserData{
					testUsr: {
						Databases: map[string]UserDatabaseData{
							testBaseDB: {
								Roles: []ssmodels.DatabaseRoleType{
									ssmodels.DatabaseRoleDataReader,
								},
							},
						},
					},
				},
			},
		},
	}

	ctrl := gomock.NewController(t)
	cryptoMock := mocks.NewMockCrypto(ctrl)

	require.Error(t, pillar.UpdateUser(cryptoMock, upUserArgs))
	require.Equal(t, []ssmodels.DatabaseRoleType{
		ssmodels.DatabaseRoleDataReader,
	}, pillar.Data.SQLServer.Users[testUsr].Databases[testBaseDB].Roles)
}

func TestSqlServer_UpdateUserNonExistingDatabase_Fails(t *testing.T) {
	upUserArgs := sqlserver.UserArgs{
		Name:     testUsr,
		Password: optional.NewOptionalPassword(secret.NewString("myNewAmazingPasswd")),
		Permissions: ssmodels.OptionalPermissions{
			Permissions: []ssmodels.Permission{
				{
					DatabaseName: testBaseDB,
				},
				{
					DatabaseName: testBaseDB2,
				},
				{
					DatabaseName: "no such database",
				},
			},
			Valid: true,
		},
	}

	pillar := Cluster{
		Data: ClusterData{
			SQLServer: SQLServerData{
				Databases: map[string]DatabaseData{
					testBaseDB:  {},
					testBaseDB2: {},
				},
				Users: map[string]UserData{
					testUsr: {},
				},
			},
		},
	}

	ctrl := gomock.NewController(t)
	cryptoMock := mocks.NewMockCrypto(ctrl)

	require.Error(t, pillar.UpdateUser(cryptoMock, upUserArgs))
}

func TestSqlServer_GrantDatabase(t *testing.T) {

	perm := ssmodels.Permission{
		DatabaseName: testBaseDB,
		Roles: []ssmodels.DatabaseRoleType{
			ssmodels.DatabaseRoleDDLAdmin,
			ssmodels.DatabaseRoleDataWriter,
			ssmodels.DatabaseRoleSecurityAdmin,
		},
	}

	pillar := Cluster{
		Data: ClusterData{
			SQLServer: SQLServerData{
				Databases: map[string]DatabaseData{
					testBaseDB:  {},
					testBaseDB2: {},
				},
				Users: map[string]UserData{
					testUsr: {
						Password: pillars.CryptoKey{Data: testPasswdData},
						Databases: map[string]UserDatabaseData{
							testBaseDB: {
								Roles: []ssmodels.DatabaseRoleType{
									ssmodels.DatabaseRoleDataReader,
								},
							},
						},
					},
				},
			},
		},
	}

	ctrl := gomock.NewController(t)
	cryptoMock := mocks.NewMockCrypto(ctrl)

	require.NoError(t, pillar.GrantPermissions(testUsr, perm, cryptoMock))
	require.Equal(t, []ssmodels.DatabaseRoleType{
		ssmodels.DatabaseRoleDataReader,
		ssmodels.DatabaseRoleDDLAdmin,
		ssmodels.DatabaseRoleDataWriter,
		ssmodels.DatabaseRoleSecurityAdmin,
	}, pillar.Data.SQLServer.Users[testUsr].Databases[testBaseDB].Roles)
	require.Equal(t, testPasswdData, pillar.Data.SQLServer.Users[testUsr].Password.Data)
}

func TestSqlServer_GrantDatabaseIncorrectDatabase_Fails(t *testing.T) {
	perm := ssmodels.Permission{
		DatabaseName: "nosuchdb",
		Roles: []ssmodels.DatabaseRoleType{
			ssmodels.DatabaseRoleDDLAdmin,
			ssmodels.DatabaseRoleDataWriter,
			ssmodels.DatabaseRoleSecurityAdmin,
		},
	}

	pillar := Cluster{
		Data: ClusterData{
			SQLServer: SQLServerData{
				Databases: map[string]DatabaseData{
					testBaseDB:  {},
					testBaseDB2: {},
				},
				Users: map[string]UserData{
					testUsr: {},
				},
			},
		},
	}

	ctrl := gomock.NewController(t)
	cryptoMock := mocks.NewMockCrypto(ctrl)

	require.Error(t, pillar.GrantPermissions(testUsr, perm, cryptoMock))
}

func TestSqlServer_RevokeDatabase(t *testing.T) {
	perm := ssmodels.Permission{
		DatabaseName: testBaseDB,
		Roles: []ssmodels.DatabaseRoleType{
			ssmodels.DatabaseRoleDDLAdmin,
		},
	}

	pillar := Cluster{
		Data: ClusterData{
			SQLServer: SQLServerData{
				Databases: map[string]DatabaseData{
					testBaseDB:  {},
					testBaseDB2: {},
				},
				Users: map[string]UserData{
					testUsr: {
						Password: pillars.CryptoKey{Data: testPasswdData},
						Databases: map[string]UserDatabaseData{
							testBaseDB: {
								Roles: []ssmodels.DatabaseRoleType{
									ssmodels.DatabaseRoleDataReader,
									ssmodels.DatabaseRoleDDLAdmin,
									ssmodels.DatabaseRoleDataWriter,
									ssmodels.DatabaseRoleSecurityAdmin,
								},
							},
						},
					},
				},
			},
		},
	}

	ctrl := gomock.NewController(t)
	cryptoMock := mocks.NewMockCrypto(ctrl)

	require.NoError(t, pillar.RevokePermissions(testUsr, perm, cryptoMock))
	require.Equal(t, []ssmodels.DatabaseRoleType{
		ssmodels.DatabaseRoleDataReader,
		ssmodels.DatabaseRoleDataWriter,
		ssmodels.DatabaseRoleSecurityAdmin,
	}, pillar.Data.SQLServer.Users[testUsr].Databases[testBaseDB].Roles)
	require.Equal(t, testPasswdData, pillar.Data.SQLServer.Users[testUsr].Password.Data)
}

func TestSqlServer_RevokeDatabaseIncorrectDatabase_Fails(t *testing.T) {
	perm := ssmodels.Permission{
		DatabaseName: "nosuchdb",
		Roles: []ssmodels.DatabaseRoleType{
			ssmodels.DatabaseRoleDDLAdmin,
			ssmodels.DatabaseRoleDataWriter,
			ssmodels.DatabaseRoleSecurityAdmin,
		},
	}

	pillar := Cluster{
		Data: ClusterData{
			SQLServer: SQLServerData{
				Databases: map[string]DatabaseData{
					testBaseDB:  {},
					testBaseDB2: {},
				},
				Users: map[string]UserData{
					testUsr: {},
				},
			},
		},
	}

	ctrl := gomock.NewController(t)
	cryptoMock := mocks.NewMockCrypto(ctrl)

	require.Error(t, pillar.RevokePermissions(testUsr, perm, cryptoMock))
}

func TestSqlServer_CreateDatabase_Works(t *testing.T) {
	db := ssmodels.DatabaseSpec{
		Name: testBaseDB,
	}
	pillar := Cluster{
		Data: ClusterData{
			SQLServer: SQLServerData{
				Databases: map[string]DatabaseData{},
				Users: map[string]UserData{
					testUsr: {},
				},
			},
		},
	}

	require.NoError(t, pillar.AddDatabase(db))
	require.Contains(t, pillar.Data.SQLServer.Databases, testBaseDB)
}

func TestSqlServer_AddExistingDatabase_Fails(t *testing.T) {
	db := ssmodels.DatabaseSpec{
		Name: testBaseDB,
	}
	pillar := Cluster{
		Data: ClusterData{
			SQLServer: SQLServerData{
				Databases: map[string]DatabaseData{
					testBaseDB: {},
				},
				Users: map[string]UserData{
					testUsr: {},
				},
			},
		},
	}

	require.Error(t, pillar.AddDatabase(db))
	require.Contains(t, pillar.Data.SQLServer.Databases, testBaseDB)
}

func TestSqlServer_DeleteDatabase_Works(t *testing.T) {
	roles := []ssmodels.DatabaseRoleType{
		ssmodels.DatabaseRoleDenyDataWriter,
		ssmodels.DatabaseRoleAccessAdmin,
		ssmodels.DatabaseRoleDDLAdmin,
	}

	pillar := Cluster{
		Data: ClusterData{
			SQLServer: SQLServerData{
				Databases: map[string]DatabaseData{
					testBaseDB:  {},
					testBaseDB2: {},
				},
				Users: map[string]UserData{
					testUsr: {
						Password: pillars.CryptoKey{Data: testPasswdData},
						Databases: map[string]UserDatabaseData{
							testBaseDB: {
								Roles: roles,
							},
							testBaseDB2: {},
						},
					},
				},
			},
		},
	}

	require.NoError(t, pillar.DeleteDatabase(testBaseDB))
	require.NotContains(t, pillar.Data.SQLServer.Databases, testBaseDB)
	require.NotContains(t, pillar.Data.SQLServer.Users[testUsr].Databases, testBaseDB)
}

func TestSqlServer_DeleteNonExistingDatabase_Fails(t *testing.T) {
	pillar := Cluster{
		Data: ClusterData{
			SQLServer: SQLServerData{
				Databases: map[string]DatabaseData{},
				Users: map[string]UserData{
					testUsr: {},
				},
			},
		},
	}

	require.Error(t, pillar.DeleteDatabase("wrifoiwejfoiwejfoiwejfiowe"))
}

func TestSqlServer_PreserveReplCert(t *testing.T) {
	data := `{"data":{"sqlserver":{
		"version": {"major_human": "2016sp2std"},
		"config": {
			"optimize_for_ad_hoc_workloads": true
		},
		"repl_cert": {"cert.key": "FOOBAR"}
	}}}`
	pillar := NewCluster()
	err := pillar.UnmarshalPillar(json.RawMessage(data))
	require.NoError(t, err, "failed to unmarshal test pillar json")
	var replCert struct {
		Key string `json:"cert.key"`
	}
	require.NoError(t, json.Unmarshal(pillar.Data.SQLServer.ReplCert, &replCert))
	require.Equal(t, "FOOBAR", replCert.Key)
	data2, err := pillar.MarshalPillar()
	require.NoError(t, err, "failed to marshal test pillar json")
	require.Contains(t, string(data2), "FOOBAR", "repl_cert key missed from test pillar")
}
