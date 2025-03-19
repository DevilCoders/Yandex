package main

import (
	"testing"

	"github.com/stretchr/testify/assert"
)

type stateToPrioTestCase struct {
	name       string
	inHost     host
	inState    hostState
	config     Config
	neededPrio priority
}

var defConfig = Config{"DC2", 1, 10, 5, 1.0, 8081, 1, nil}

var stateToPrioTestCases = []stateToPrioTestCase{
	{
		"Alive master",
		host{
			hostInfo{"shard01-dc1.pgcheck.net", "", "DC1", 0, 0},
			hostState{},
			hostPrio{},
			hostAux{},
		},
		hostState{true, true, 0, 0.08},
		defConfig,
		0,
	},
	{
		"Dead host",
		host{
			hostInfo{"shard01-dc1.pgcheck.net", "", "DC1", 0, 0},
			hostState{},
			hostPrio{},
			hostAux{},
		},
		hostState{false, false, 0, 0},
		defConfig,
		100,
	},
	{
		"Alive replica in other DC",
		host{
			hostInfo{"shard01-dc1.pgcheck.net", "", "DC1", 0, 0},
			hostState{},
			hostPrio{},
			hostAux{},
		},
		hostState{true, false, 0, 0.08},
		defConfig,
		24,
	},
	{
		"Alive replica in same DC",
		host{
			hostInfo{"shard01-dc1.pgcheck.net", "", "DC1", 0, 0},
			hostState{},
			hostPrio{},
			hostAux{},
		},
		hostState{true, false, 0, 0.08},
		Config{"DC1", 1, 10, 5, 1.0, 8081, 1, nil},
		14,
	},
	{
		"Alive replica with replication lag",
		host{
			hostInfo{"shard01-dc1.pgcheck.net", "", "DC1", 0, 0},
			hostState{},
			hostPrio{},
			hostAux{},
		},
		hostState{true, false, 10, 0.08},
		defConfig,
		34,
	},
	{
		"Alive overloaded replica",
		host{
			hostInfo{"shard01-dc1.pgcheck.net", "", "DC1", 0, 0},
			hostState{},
			hostPrio{},
			hostAux{},
		},
		hostState{true, false, 0, 0.9},
		defConfig,
		65,
	},
	{
		"Alive delayed and overloaded replica",
		host{
			hostInfo{"shard01-dc1.pgcheck.net", "", "DC1", 0, 0},
			hostState{},
			hostPrio{},
			hostAux{},
		},
		hostState{true, false, 50, 0.9},
		defConfig,
		115,
	},
}

func TestStateToPrio(t *testing.T) {
	for _, test := range stateToPrioTestCases {
		t.Run(test.name, func(t *testing.T) {
			prio := stateToPrio(&test.inHost, &test.inState, test.config)
			assert.Equal(t, int(test.neededPrio), int(prio), "Wrong priority")
		})
	}
}
