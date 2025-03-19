package salt

import (
	"strings"

	"a.yandex-team.ru/cloud/mdb/deploy/agent/internal/pkg/commander"
)

const (
	highstate = "state.highstate"
	stateSLS  = "state.sls"
)

func concurrentNotSet(args []string) bool {
	for _, a := range args {
		if strings.ToLower(a) == "concurrent=true" {
			return false
		}
	}

	return true
}

func Conflict(cmd commander.Command, running []commander.Command) bool {
	switch cmd.Name {
	case highstate:
		for _, r := range running {
			if r.Name == highstate || r.Name == stateSLS {
				return true
			}
		}
	case stateSLS:
		for _, r := range running {
			switch r.Name {
			case highstate:
				return true
			case stateSLS:
				if concurrentNotSet(cmd.Args) || concurrentNotSet(r.Args) {
					return true
				}
			}
		}
	}

	for _, r := range running {
		if r.Source.Version != cmd.Source.Version {
			return true
		}
	}

	return false
}
