package ftpd

import (
	"flag"

	"a.yandex-team.ru/admins/ftp2s3/internal/ftpd"
)

func Main() {
	flag.Parse()
	ftpd.Serve()
}
