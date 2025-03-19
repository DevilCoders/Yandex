package sqlutil_test

import (
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
)

func TestGetColumnsFromStruct(t *testing.T) {
	type Foo struct {
		Tagged   string `db:"tagged_name"`
		UnTagged int
	}
	require.Equal(
		t,
		sqlutil.GetColumnsFromStruct(Foo{}),
		[]string{"tagged_name", "UnTagged"},
	)
}

func TestGetColumnNamesShouldPanicsForEmptyStruct(t *testing.T) {
	type BadStruct struct{}
	require.Panics(t, func() { sqlutil.GetColumnsFromStruct(BadStruct{}) })
}

func TestGetColumnsFromStructPanicsForUnsupportedType(t *testing.T) {
	require.Panics(t, func() { sqlutil.GetColumnsFromStruct(42) })
}

func TestNewStmt(t *testing.T) {
	type Cloud struct {
		ID            string
		Name          string
		ClustersCount int `db:"clusters_count"`
	}
	require.Equal(
		t,
		sqlutil.Stmt{
			Name:  "GetClouds",
			Query: "SELECT ID, Name, clusters_count FROM (SELECT * FROM dbaas.clouds) w",
		},
		sqlutil.NewStmt("GetClouds", "SELECT * FROM dbaas.clouds", Cloud{}),
	)
}
