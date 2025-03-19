package chpillars

import (
	"reflect"
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/clickhouse/chmodels"
)

func TestFormatPermissions(t *testing.T) {
	permissions := []chmodels.Permission{
		{DatabaseName: "testdb"},
	}
	databases := map[string]Database{
		"testdb": {},
	}
	require.Equal(t, FormatPermissions(permissions), databases)
}

func TestUserPermissionsFromPillar(t *testing.T) {
	databases := map[string]Database{
		"testdb": {},
	}
	permissions := []chmodels.Permission{
		{DatabaseName: "testdb"},
	}
	require.Equal(t, UserPermissionsFromPillar(databases), permissions)
}

func TestUserSettingsFields(t *testing.T) {
	userSettings := reflect.TypeOf(chmodels.UserSettings{})
	userSettingsPillar := reflect.TypeOf(UserSettings{})

	require.Equal(t, userSettings.NumField(), userSettingsPillar.NumField())

	userSettingsFields := make(map[string]struct{})
	userSettingsPillarFields := make(map[string]struct{})

	for i := 0; i < userSettings.NumField(); i++ {
		userSettingsFields[userSettings.Field(i).Name] = struct{}{}
	}
	for i := 0; i < userSettingsPillar.NumField(); i++ {
		userSettingsPillarFields[userSettingsPillar.Field(i).Name] = struct{}{}
	}

	for fieldName := range userSettingsFields {
		_, ok := userSettingsPillarFields[fieldName]
		require.True(t, ok, "field %s from chmodels.UserSettings must be present in chpillars.UserSettings", fieldName)
	}
	for fieldName := range userSettingsPillarFields {
		_, ok := userSettingsFields[fieldName]
		require.True(t, ok, "field %s from chpillars.UserSettings must be present in chmodels.UserSettings", fieldName)
	}
}
