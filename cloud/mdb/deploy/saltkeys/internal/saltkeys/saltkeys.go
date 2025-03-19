package saltkeys

import (
	"fmt"

	"a.yandex-team.ru/library/go/core/xerrors"
)

//go:generate ../../../../scripts/mockgen.sh Keys

var (
	ErrKeyMismatch = xerrors.NewSentinel("minion key mismatch")
)

// StateID is minion's status (accepted, rejected, etc)
type StateID int

// Known minion states
const (
	Pre StateID = iota + 1
	Accepted
	Rejected
	Denied
)

// State describes minion state
type State struct {
	Name    string
	KeyPath string
}

// States lists all known minion states
var States = map[StateID]State{
	Pre:      {Name: "Pre", KeyPath: "/etc/salt/pki/master/minions_pre"},
	Accepted: {Name: "Accepted", KeyPath: "/etc/salt/pki/master/minions"},
	Rejected: {Name: "Rejected", KeyPath: "/etc/salt/pki/master/minions_rejected"},
	Denied:   {Name: "Denied", KeyPath: "/etc/salt/pki/master/minions_denied"},
}

// State returns this state's description
func (s StateID) State() State {
	si, ok := States[s]
	if !ok {
		panic(fmt.Sprintf("unknown salt key state: %d", s))
	}

	return si
}

// String implements Stringer
func (s StateID) String() string {
	return s.State().Name
}

// KeyPath returns location of minion public keys for this state
func (s StateID) KeyPath() string {
	return s.State().KeyPath
}

// Keys is an interface for managing minion keys known to local salt master
type Keys interface {
	Accept(fqdn string) error
	Reject(fqdn string) error
	Delete(fqdn string) error
	List(s StateID) ([]string, error)
	Key(fqdn string, s StateID) (string, error)
}

// Minions returns list of known minions with their states
func Minions(keys Keys, states ...StateID) (map[string][]StateID, error) {
	minions := make(map[string][]StateID)

	for _, s := range states {
		fqdns, err := keys.List(s)
		if err != nil {
			return nil, xerrors.Errorf("failed to retrieve minions with state %q: %w", s, err)
		}

		for _, fqdn := range fqdns {
			minions[fqdn] = append(minions[fqdn], s)
		}
	}

	return minions, nil
}
