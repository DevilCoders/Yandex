package main

import (
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/app/mdb"
	mongoperfdb "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/mongodb/perfdiagdb/mocks"
	myperfdb "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/mysql/perfdiagdb/mocks"
	pgperfdb "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/postgresql/perfdiagdb/mocks"
	flogsdb "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logsdb/file"
)

func main() {
	mdb.Run(mdb.AppComponents{
		LogsDB:          &flogsdb.Backend{Path: ""},
		PgPerfDiagDB:    pgperfdb.NewMockBackend(nil),
		MyPerfDiagDB:    myperfdb.NewMockBackend(nil),
		MongoPerfDiagDB: mongoperfdb.NewMockBackend(nil),
	})
}
