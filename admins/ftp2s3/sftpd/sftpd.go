package sftpd

import (
	"flag"

	"a.yandex-team.ru/admins/ftp2s3/internal/sftpd"
)

func Main() {
	flag.Parse()
	sftpd.Serve()
}
