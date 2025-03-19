package internal

import "strings"

type RedisRole string

const (
	RedisRoleMaster = "master"
	RedisRoleSlave  = "slave"
)

// RedisHost is a type to be sure about what host means. Redis host is a pair of hostname and port that divided by colon.
type RedisHost string

// HostnameAndPort splits redis address for host and port
// returned params string, string because right now this method is only used for REPLICAOF redis command that accepts only strings.
func (h RedisHost) HostnameAndPort() (string, string) {
	splitted := strings.Split(string(h), ":")

	return splitted[0], splitted[1]
}

const RedisHostEmpty = RedisHost("")

type AllDBsInfo map[RedisHost]DBInfo

type DBInfo struct {
	Host  RedisHost `json:"host"`
	Role  RedisRole `json:"role"`
	Alive bool      `json:"alive"`
	// LastSuccessfulPing is a Unix tumestamp of last ping on which database replied successfully
	LastSuccessfulPing int64 `json:"last_successful_ping"`
	ReplicaID          string
	ReplicaOffset      int64
	Error              error
}
