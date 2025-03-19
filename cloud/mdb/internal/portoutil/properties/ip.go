package properties

import (
	"net"
	"strings"

	"a.yandex-team.ru/library/go/core/xerrors"
)

func ParseIPProperty(ipProperty string) ([]string, error) {
	ips := strings.Split(ipProperty, ";")

	records := make([]string, 0, len(ips))
	for _, ip := range ips {
		parsedProp := strings.Split(ip, " ")
		if len(parsedProp) != 2 {
			return nil, xerrors.Errorf("invalid ipProperty format, must be 'eth0 ip1;eth0 ip2', got %q", ipProperty)
		}
		records = append(records, net.ParseIP(parsedProp[1]).String())
	}

	return records, nil
}
