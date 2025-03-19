package tests

import (
	"testing"

	"github.com/DATA-DOG/godog"

	"a.yandex-team.ru/cloud/mdb/internal/dbteststeps"
	"a.yandex-team.ru/cloud/mdb/internal/godogutil"
	"a.yandex-team.ru/library/go/test/yatest"
)

var featurePaths = []string{
	yatest.SourcePath("cloud/mdb/mdb-vpc/db/tests/features"),
}

func TestMain(m *testing.M) {
	tc := godogutil.NewTestContext(
		true,
		contextInitializer,
		godogutil.WithContextPerScenario(),
		godogutil.WithScenariosAsSubtests(),
	)
	godogutil.RunSuite(featurePaths, tc, m)
}

func contextInitializer(tc *godogutil.TestContext, s *godog.Suite) {
	dbst, err := dbteststeps.New(
		tc,
		dbteststeps.Params{
			DBName:      "vpcdb",
			SingleFile:  "vpcdb.sql",
			ArcadiaPath: "cloud/mdb/mdb-vpc/db",
			DataSchema:  "vpc",
		})
	if err != nil {
		panic(err)
	}
	dbteststeps.RegisterSteps(dbst, s)
}
