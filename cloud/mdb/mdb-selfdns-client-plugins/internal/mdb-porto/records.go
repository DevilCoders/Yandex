package mdbporto

import (
	"fmt"
	"strings"

	"a.yandex-team.ru/cloud/mdb/internal/fqdn"
	"a.yandex-team.ru/cloud/mdb/internal/portoutil/properties"
	"a.yandex-team.ru/library/go/core/xerrors"
)

const (
	version = "0.1"
)

type record struct {
	ip     string
	domain string
}

type Formatter struct {
	fqdnConverter fqdn.Converter
}

func (f *Formatter) GetFormattedRecordsFromIPs(ipProperty, hostname string) (string, error) {
	ips, err := properties.ParseIPProperty(ipProperty)
	if err != nil {
		return "", err
	}

	var records []record

	lenIPs := len(ips)

	switch lenIPs {
	case 1:
		records = append(records, record{
			ip:     ips[0],
			domain: hostname,
		})

	case 2:
		controlFQDN, err := f.fqdnConverter.UnmanagedToManaged(hostname)
		if err != nil {
			return "", xerrors.Errorf("can't generate control FQDN")
		}

		userFQDN, err := f.fqdnConverter.ManagedToUnmanaged(hostname)
		if err != nil {
			return "", xerrors.Errorf("can't generate user FQDN")
		}

		records = append(records, record{
			ip:     ips[0],
			domain: userFQDN,
		})

		records = append(records, record{
			ip:     ips[1],
			domain: controlFQDN,
		})

	default:
		return "", xerrors.Errorf("records count %d more than 2 not supported", lenIPs)
	}

	var b strings.Builder

	for _, r := range records {
		if _, err := fmt.Fprintf(&b, "%s %s %s\n", version, r.domain, r.ip); err != nil {
			return "", xerrors.Errorf("can't write formatted records to string builder: %w", err)
		}
	}

	return b.String(), nil
}
