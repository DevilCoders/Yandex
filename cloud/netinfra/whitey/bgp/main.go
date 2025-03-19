package bgp

import (
	"encoding/json"
	"errors"
	"strings"
)

type Peer struct {
	Vrf     string
	Group   string
	Address string
	State   State
	Uptime  uint64
	AFs     []AFInfo
}

type Prefix struct {
	In  uint
	Out uint
}

type AFInfo struct {
	Name   string
	Prefix *Prefix
}

// We are using integers from RFC4273 to define BGP states
// https://datatracker.ietf.org/doc/html/rfc4273#section-4
type State int

const (
	Idle State = iota + 1
	Connect
	Active
	OpenSent
	OpenConfirm
	Established
)

func (s *State) UnmarshalJSON(b []byte) error {
	var state string
	if err := json.Unmarshal(b, &state); err != nil {
		return err
	}
	switch strings.ToLower(state) {
	case "idle":
		*s = Idle
	case "connect":
		*s = Connect
	case "active":
		*s = Active
	case "opensent":
		*s = OpenSent
	case "openconfirm":
		*s = OpenConfirm
	case "established":
		*s = Established
	default:
		return errors.New("Invalid BGP state: " + state)
	}

	return nil
}

func (s State) String() string {
	return [...]string{
		"idle",
		"connect",
		"active",
		"opensent",
		"openconfirm",
		"established",
	}[s]
}
