package saltapi

import (
	"context"
	"fmt"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/secret"
	"a.yandex-team.ru/library/go/core/log"
)

//go:generate ../../scripts/mockgen.sh Auth,Authenticator,Secrets,Client,SaltUtil,State,Test,Config

// Known EAuth types
const (
	EAuthPAM = "pam"
)

// Credentials for salt-api authentication
type Credentials struct {
	User     string        `yaml:"user"`
	Password secret.String `yaml:"password"`
	EAuth    string        `yaml:"eauth"`
}

// String implements fmt.Stringer
func (cr *Credentials) String() string {
	return fmt.Sprintf("%s/******(%s)", cr.User, cr.EAuth)
}

// Tokens holds salt-api authentication tokens
type Tokens struct {
	// Session authentication token
	Session Token
	// EAuth authentication token
	EAuth Token
}

// Token is one salt-api authentication token
type Token struct {
	// Value holds authentication token
	Value string
	// ExpiresAt is when the token expires
	ExpiresAt time.Time
}

// Authenticator provides means to authenticate salt-api credentials
type Authenticator interface {
	AuthSessionToken(ctx context.Context) (Token, error)
	AuthEAuthToken(ctx context.Context, timeout time.Duration) (Token, error)

	// AutoAuth automatically authenticates and renews tokens when needed. Usually run in a separate goroutine.
	// reAuthInterval argument specifies period of re-authentication attempts in case of error.
	// authAttemptTimeout argument specifies timeout for authentication calls to salt-api.
	AutoAuth(ctx context.Context, reAuthInterval, authAttemptTimeout time.Duration, l log.Logger)
}

// Secrets provides all authentication secrets
type Secrets interface {
	// Credentials used when creating this Auth
	Credentials() Credentials

	// Tokens retrieved using credentials
	Tokens() Tokens

	// Invalidate authentication tokens
	Invalidate()
}

// Auth combines Authenticator and Secrets interfaces
type Auth interface {
	Authenticator
	Secrets
}

// Client provides general interface for salt-api
type Client interface {
	// NewAuth creates object used for authentication
	NewAuth(creds Credentials) Auth

	// Ping checks if salt master is alive
	Ping(ctx context.Context, sec Secrets) error

	// Minions returns all known minions
	Minions(ctx context.Context, sec Secrets) ([]Minion, error)

	// Run arbitrary command
	Run(ctx context.Context, sec Secrets, timeout time.Duration, target, fun string, args ...string) error
	// AsyncRun is same as Run but asynchronously
	AsyncRun(ctx context.Context, sec Secrets, timeout time.Duration, target, fun string, args ...string) (string, error)

	// SaltUtil returns interface for salt.modules.saltutil
	SaltUtil() SaltUtil

	// State returns interface for salt.modules.state
	State() State

	// Test returns interface for salt.modules.test
	Test() Test

	// Config returns interface for salt.modules.config
	Config() Config
}

// SaltUtil provides interface for salt.modules.saltutil
type SaltUtil interface {
	SyncAll(ctx context.Context, sec Secrets, timeout time.Duration, target string) error
	AsyncSyncAll(ctx context.Context, sec Secrets, timeout time.Duration, target string) (string, error)
	IsRunning(ctx context.Context, sec Secrets, timeout time.Duration, target, fun string) (map[string][]RunningFunc, error)
	AsyncIsRunning(ctx context.Context, sec Secrets, timeout time.Duration, target, state string) (string, error)
	FindJob(ctx context.Context, sec Secrets, timeout time.Duration, target, jid string) (map[string]RunningFunc, error)
	AsyncFindJob(ctx context.Context, sec Secrets, timeout time.Duration, target, jid string) (string, error)
	TermJob(ctx context.Context, sec Secrets, timeout time.Duration, target, jid string) error
	AsyncTermJob(ctx context.Context, sec Secrets, timeout time.Duration, target, jid string) (string, error)
	KillJob(ctx context.Context, sec Secrets, timeout time.Duration, target, jid string) error
	AsyncKillJob(ctx context.Context, sec Secrets, timeout time.Duration, target, jid string) (string, error)
}

type RunningFunc struct {
	Function   string   `json:"fun"`
	Arg        []string `json:"arg"`
	JobID      string   `json:"jid"`
	ProcessID  uint64   `json:"pid"`
	Return     string   `json:"ret"`
	Target     string   `json:"tgt"`
	TargetType string   `json:"tgt_type"`
	User       string   `json:"user"`
}

func (rf RunningFunc) IsLocalRun() bool {
	return rf.Target == "salt-call"
}

// State provides interface for salt.modules.state
type State interface {
	Running(ctx context.Context, sec Secrets, timeout time.Duration, target string) (map[string][]string, error)
	Highstate(ctx context.Context, sec Secrets, timeout time.Duration, target string, args ...string) error
	AsyncHighstate(ctx context.Context, sec Secrets, timeout time.Duration, target string, args ...string) (string, error)
}

// Test provides interface for salt.modules.test
type Test interface {
	Ping(ctx context.Context, sec Secrets, timeout time.Duration, target string) (map[string]bool, error)
	AsyncPing(ctx context.Context, sec Secrets, timeout time.Duration, target string) (string, error)
}

// Config provides interface for salt.modules.config
type Config interface {
	WorkerThreads(ctx context.Context, sec Secrets, timeout time.Duration) (int32, error)
}

// Minion describes salt minion
type Minion struct {
	FQDN       string
	MasterFQDN string
	Accepted   bool
}
