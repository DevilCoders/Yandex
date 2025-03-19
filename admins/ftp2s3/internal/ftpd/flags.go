package ftpd

import (
	"encoding/json"
	"flag"
	"fmt"
	"strconv"
	"strings"

	"github.com/fclairamb/ftpserver/server"

	"a.yandex-team.ru/library/go/core/xerrors"
)

type ftpdFlags struct {
	Host              string
	Port              string
	PublicIP          string
	PassivePortRange  PortRangeFlag
	ConnectionTimeout int
	IdleTimeout       int
	TLSCert           string
	TLSKey            string
	Passwd            string
	LogLevel          string
}

// Flags are flags for ftpd
var Flags = ftpdFlags{}

func (f ftpdFlags) String() string {
	data, _ := json.Marshal(Flags)
	return string(data)
}

func init() {
	flag.StringVar(&Flags.Host, "host", "[::]", "Host to bind FTP server to")
	flag.StringVar(&Flags.Port, "port", "2121", "Port to bind FTP server to")
	flag.StringVar(&Flags.PublicIP, "public-ip", "", "Public IP to expose")

	flag.IntVar(&Flags.ConnectionTimeout, "connection-timeout", 5, "Maximum time to establish passive or active transfer connections (seconds)")
	flag.IntVar(&Flags.IdleTimeout, "idle-timeout", 300, "Maximum inactivity time before disconnecting (seconds)")

	flag.StringVar(&Flags.TLSCert, "tls-cert", "", "PEM-encoded TLS certificate file")
	flag.StringVar(&Flags.TLSKey, "tls-key", "", "PEM-encoded TLS private key")

	flag.StringVar(&Flags.Passwd, "passwd", "passwd.json", "File with users information")
	flag.StringVar(&Flags.LogLevel, "log-level", "error", "Enable logging (error|warn|info|debug)")

	flag.Var(&Flags.PassivePortRange, "passive-port-range", "Range of a ports that passive DTs should listen on")
}

type PortRangeFlag struct {
	Range *server.PortRange
}

func (rf *PortRangeFlag) String() string {
	if rf.Range == nil {
		return ""
	}
	return fmt.Sprintf("%d-%d", rf.Range.Start, rf.Range.End)
}

func (rf *PortRangeFlag) Set(portRange string) error {
	if len(portRange) > 0 {
		var result server.PortRange

		portRange := strings.Split(portRange, "-")
		if len(portRange) != 2 {
			return xerrors.New("Incorrect port range")
		}

		var err error
		result.Start, err = strconv.Atoi(strings.TrimSpace(portRange[0]))
		if err != nil {
			return xerrors.Errorf("start port range: %v", err)
		}
		result.End, err = strconv.Atoi(strings.TrimSpace(portRange[1]))
		if err != nil {
			return xerrors.Errorf("end port range: %v", err)
		}

		if result.Start >= result.End {
			return xerrors.New("Incorrect port range: start >= end")
		}
		rf.Range = &result
	}

	return nil
}
