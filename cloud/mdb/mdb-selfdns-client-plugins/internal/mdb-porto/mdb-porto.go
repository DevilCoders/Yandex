package mdbporto

import (
	"fmt"
	"log"
	"os"

	fqdn "a.yandex-team.ru/cloud/mdb/internal/fqdn/impl"
	portoapi "a.yandex-team.ru/infra/tcp-sampler/pkg/porto"
	"a.yandex-team.ru/library/go/core/xerrors"
)

const (
	UserFQDNSuffix    = "db.yandex.net"
	ControlFQDNSuffix = "mdb.yandex.net"
)

func Run() {
	porto, err := portoapi.Connect()
	if err != nil {
		log.Fatal(xerrors.Errorf("can't connect to porto socket: %w", err))
	}

	hostname, err := os.Hostname()
	if err != nil {
		log.Fatal(xerrors.Errorf("can't get hostname: %w", err))
	}

	ipProperty, err := porto.GetProperty(hostname, "ip")
	if err != nil {
		log.Fatal(xerrors.Errorf("can't get ip property from porto: %w", err))
	}

	formatter := Formatter{
		fqdnConverter: fqdn.NewConverter("", UserFQDNSuffix, ControlFQDNSuffix),
	}

	formattedRecords, err := formatter.GetFormattedRecordsFromIPs(ipProperty, hostname)
	if err != nil {
		log.Fatal(xerrors.Errorf("can't get formatted records: %w", err))
	}

	fmt.Print(formattedRecords)
}
