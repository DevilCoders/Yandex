package services

import "fmt"

type Health struct {
	Type    Type
	Status  Status
	Role    Role
	Metrics map[string]string
	// TODO: replica type
}

type Type int

const (
	TypeUnknown Type = iota
	TypeSQLServer
	TypeClickHouse
	TypeZooKeeper
	TypeGreenplumMasterServer
	TypeGreenplumSegmentServer
	TypeKafka
	TypeKafkaConnect
	TypeWindowsWitnessNode
	TypeRedisNonsharded
	TypeRedisSharded
	TypeRedisSentinel
)

type Status int

const (
	StatusUnknown Status = iota
	StatusDead
	StatusAlive
)

var mapStatusToString = map[Status]string{
	StatusUnknown: "Unknown",
	StatusDead:    "Dead",
	StatusAlive:   "Alive",
}

func (s Status) String() string {
	str, ok := mapStatusToString[s]
	if !ok {
		panic(fmt.Sprintf("invalid service status %d", s))
	}

	return str
}

type Role int

const (
	RoleUnknown Role = iota
	RoleMaster
	RoleReplica
	RoleWitness
)

var mapRoleToString = map[Role]string{
	RoleUnknown: "Unknown",
	RoleMaster:  "Master",
	RoleReplica: "Replica",
	RoleWitness: "Witness",
}

func (s Role) String() string {
	str, ok := mapRoleToString[s]
	if !ok {
		panic(fmt.Sprintf("invalid service role %d", s))
	}

	return str
}
