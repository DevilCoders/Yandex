package mysyncutil

import (
	"testing"

	"github.com/stretchr/testify/require"
)

func TestHANCluster(t *testing.T) {
	// shrinked data from real cluster:
	jsonData := `
	{
	  "active_nodes": ["master_host", "slave_host"],
	  "backup_lock": null,
	  "cascade_nodes": null,
	  "ha_nodes": {
		"master_host": null,
		"slave_host": null
	  },
	  "health": {
		"master_host": { },
		"slave_host": { }
	  },
	  "last_switch": { },
	  "low_space": false,
	  "manager": {
		"hostname": "slave_host",
		"pid": 4061868
	  },
	  "master": "master_host",
	  "recovery": null
	}`

	mi, err := readMysyncInfoString(jsonData)
	require.NoError(t, err)

	haNodes, err := mi.HANodes()
	require.NoError(t, err)
	require.Equal(
		t,
		[]string{"master_host", "slave_host"},
		haNodes,
	)

	cascadeNodes, err := mi.CascadeNodes()
	require.NoError(t, err)
	require.Equal(
		t,
		map[string]string{},
		cascadeNodes,
	)
	replicas, err := mi.ExpectedReplicasOf("master_host")
	require.NoError(t, err)
	require.Equal(
		t,
		[]string{"slave_host"},
		replicas,
	)
	replicas, err = mi.ExpectedReplicasOf("slave_host")
	require.NoError(t, err)
	require.Equal(
		t,
		[]string{},
		replicas,
	)
}
func TestCascadeCluster(t *testing.T) {
	jsonData := `
	{
	  "active_nodes": [
		"master",
		"cascade_host"
	  ],
	  "backup_lock": null,
	  "cascade_nodes": {
		"cascade_host": {
		  "stream_from": "master"
		}
	  },
	  "ha_nodes": {
		"master": null
	  },
	  "health": {
		"master": {},
		"cascade_host": {}
	  },
	  "low_space": false,
	  "manager": {},
	  "master": "master"
	}`

	mi, err := readMysyncInfoString(jsonData)
	require.NoError(t, err)

	cascadeNodes, err := mi.CascadeNodes()
	require.NoError(t, err)
	require.Equal(
		t,
		map[string]string{"cascade_host": "master"},
		cascadeNodes,
	)

	replicas, err := mi.ExpectedReplicasOf("master")
	require.NoError(t, err)
	require.Equal(
		t,
		[]string{"cascade_host"},
		replicas,
	)
	replicas, err = mi.ExpectedReplicasOf("cascade_host")
	require.NoError(t, err)
	require.Equal(
		t,
		[]string{},
		replicas,
	)
}

func TestDiscState(t *testing.T) {
	jsonData := `
	{
	  "active_nodes": [],
	  "backup_lock": null,
	  "cascade_nodes": {},
	  "ha_nodes": {},
	  "health": {
		"master": {
			"disk_state": {
				"Total": 21474836480,
				"Used": 2412011520
			}
		}
	  },
	  "low_space": false,
	  "manager": {},
	  "master": "master"
	}`

	mi, err := readMysyncInfoString(jsonData)
	require.NoError(t, err)

	ds, err := mi.DiskState("master")
	require.NoError(t, err)
	require.Equal(t, uint64(21474836480), ds.Total)
	require.Equal(t, uint64(2412011520), ds.Used)
}
