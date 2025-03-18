//

// m3 - a traffic daemon, (a) listing bgp (b) making
// routes and rules for (1) mpls transits (2) udp tunnels

// [1] https://st.yandex-team.ru/TRAFFIC-11002

package main

import (
	"os"

	"a.yandex-team.ru/cdn/m3/cmd"
)

const (
	ExitCodeUnspecified = 1
)

var (
	Version = "dev"
	Commit  = "none"
	Date    = "unknown"
)

func main() {
	cmd.SetVersion(Version)
	cmd.SetDate(Date)

	err := cmd.Execute()
	if err != nil {
		os.Exit(ExitCodeUnspecified)
	}
}
