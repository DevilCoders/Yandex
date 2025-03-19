package services

import (
	"encoding/json"
	"io/ioutil"
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/deploy/api/pkg/models"
)

func getExpected(t *testing.T, name string) string {
	body, err := ioutil.ReadFile("testdata/" + name)
	require.NoError(t, err)
	return string(body)
}

func Test_formStartUpgradeComment(t *testing.T) {
	got, err := formStartUpgradeComment(startUpgradeContext{
		Service: serviceUpgradeContext{
			Name: "worker",
			Env:  "porto-test",
			UI:   "https://ui.db",
		},
		Shipment: models.Shipment{
			ID:    42,
			FQDNs: []string{"man.db", "sas.db"},
		},
	})
	require.NoError(t, err)
	require.Equal(t, getExpected(t, "start_upgrade.wiki"), got.Text)
}

func Test_formFinishUpgradeComment(t *testing.T) {
	t.Run("Without changes", func(t *testing.T) {
		got, err := formFinishUpgradeComment(finishUpdateContext{
			Service: serviceUpgradeContext{
				Name: "worker",
				Env:  "porto-test",
				UI:   "https://ui.db",
			},
			Shipment: models.Shipment{
				ID:     42,
				FQDNs:  []string{"man.db", "sas.db"},
				Status: models.ShipmentStatusDone,
			},
			Changes: nil,
		})
		require.NoError(t, err)
		require.Equal(t, getExpected(t, "finish_upgrade_without_changes.wiki"), got.Text)
	})

	t.Run("With changes", func(t *testing.T) {
		got, err := formFinishUpgradeComment(finishUpdateContext{
			Service: serviceUpgradeContext{
				Name: "report",
				Env:  "porto-test",
				UI:   "https://ui.db",
			},
			Shipment: models.Shipment{
				ID:     42,
				FQDNs:  []string{"man.db", "sas.db"},
				Status: models.ShipmentStatusDone,
			},
			Changes: map[string]map[string]string{
				"man.db": {
					"mdb-maintenance-pkgs": "mdb-maintenance: {'new': '1.8243985', 'old': '1.8239801'}",
				},
				"sas.db": {
					"mdb-katan-pkgs": "mdb-katan: {'new': '1.8100164', 'old': '1.8067935'}",
				},
			},
		})
		require.NoError(t, err)
		require.Equal(t, getExpected(t, "finish_upgrade_with_changes.wiki"), got.Text)
	})
}

func Test_prettyChange(t *testing.T) {
	tests := []struct {
		name    string
		change  string
		comment string
		want    string
	}{
		{
			"package update",
			`{"mdb-katan": {"new": "1.8100164", "old": "1.8067935"}}`,
			"1 targeted package was installed/updated. The following packages were already installed: yazk-flock",
			"mdb-katan updated 1.8067935 -> 1.8100164",
		},
		{
			"package install",
			`{"mdb-katan": {"new": "1.8100164"}}`,
			"1 targeted package was installed/updated. The following packages were already installed: yazk-flock",
			"mdb-katan=1.8100164 installed",
		},
		{
			"package uninstall",
			`{"mdb-katan": {"old": "1.8067935"}}`,
			"",
			"mdb-katan=1.8067935 uninstalled",
		},
		{
			"supervised",
			`{"mdb-katan": "Restarting service: mdb-katan"}`,
			"Restarting service: mdb-katan",
			"Restarting service: mdb-katan",
		},
		{
			"system service restart",
			`{"mdb-ping-salt-master": true}`,
			"Service restarted",
			"Service restarted: mdb-ping-salt-master",
		},
		{
			"return as-is for unsupported",
			`{"pid": 759063, "stderr": "", "stdout": "", "retcode": 0}}`,
			`Command "chown -R monitor.monitor /etc/monrun && sudo -u monitor monrun --gen-jobs || true" run`,
			`{"pid": 759063, "stderr": "", "stdout": "", "retcode": 0}}`,
		},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			require.Equal(t, tt.want, prettyChange(json.RawMessage(tt.change), tt.comment))
		})
	}
}
