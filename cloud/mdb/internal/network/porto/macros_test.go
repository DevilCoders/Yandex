package porto

import (
	"testing"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/cloud/mdb/internal/racktables"
)

func TestGetNetworksWithMatchedOwners(t *testing.T) {
	macros := racktables.MacrosWithOwners{
		"_DBAASEXTERNALNETS_": []racktables.Owner{
			{Type: "servicerole", Name: "svc_ycsecurity_administration"},
		},
		"_MDB_CONTROLPLANE_PORTO_TEST_NETS_": []racktables.Owner{
			{Type: "user", Name: "velom"},
			{Type: "service", Name: "svc_internalmdb"},
			{Type: "servicerole", Name: "svc_internalmdb_administration"},
		},
		"_MDB_DATAPROC_TEST_NETS_": []racktables.Owner{
			{Type: "service", Name: "svc_dataprocessing"},
			{Type: "service", Name: "svc_ycsecurity"},
		},
	}

	abcSlug := "internalmdb"

	actual := GetNetworksWithMatchedOwners(macros, abcSlug)

	assert.Equal(t, []string{"_MDB_CONTROLPLANE_PORTO_TEST_NETS_"}, actual)
}

func TestGetNetworksWithMatchedOwnersNotMatched(t *testing.T) {
	macros := racktables.MacrosWithOwners{
		"_DBAASEXTERNALNETS_": []racktables.Owner{
			{Type: "servicerole", Name: "svc_ycsecurity_administration"},
		},
		"_MDB_CONTROLPLANE_PORTO_TEST_NETS_": []racktables.Owner{
			{Type: "user", Name: "velom"},
			{Type: "service", Name: "svc_internalmdb"},
			{Type: "servicerole", Name: "svc_internalmdb_administration"},
		},
		"_MDB_DATAPROC_TEST_NETS_": []racktables.Owner{
			{Type: "service", Name: "svc_dataprocessing"},
			{Type: "service", Name: "svc_ycsecurity"},
		},
	}

	abcSlug := "mdbcoreteam"

	actual := GetNetworksWithMatchedOwners(macros, abcSlug)

	assert.Equal(t, []string(nil), actual)
}

func TestGetNetworksWithMatchedOwnersSorted(t *testing.T) {
	macros := racktables.MacrosWithOwners{
		"_MDB_CONTROLPLANE_PORTO_TEST_NETS_": []racktables.Owner{
			{Type: "service", Name: "svc_internalmdb"},
		},
		"_MDB_DATAPROC_TEST_NETS_": []racktables.Owner{
			{Type: "service", Name: "svc_internalmdb"},
		},
		"_DBAASEXTERNALNETS_": []racktables.Owner{
			{Type: "service", Name: "svc_internalmdb"},
		},
	}

	abcSlug := "internalmdb"

	expected := []string{
		"_DBAASEXTERNALNETS_",
		"_MDB_CONTROLPLANE_PORTO_TEST_NETS_",
		"_MDB_DATAPROC_TEST_NETS_",
	}

	actual := GetNetworksWithMatchedOwners(macros, abcSlug)

	assert.Equal(t, expected, actual)
}
