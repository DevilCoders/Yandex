package functest

import (
	"testing"

	"github.com/DATA-DOG/godog"

	"a.yandex-team.ru/cloud/mdb/internal/dbteststeps"
	"a.yandex-team.ru/cloud/mdb/internal/godogutil"
	"a.yandex-team.ru/library/go/test/yatest"
)

var featurePaths = []string{
	yatest.SourcePath("cloud/mdb/deploydb/tests/features"),
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
			DBName:      "deploydb",
			SingleFile:  "deploy.sql",
			ArcadiaPath: "cloud/mdb/deploydb",
			DataSchema:  "deploy",
		})
	if err != nil {
		panic(err)
	}
	tctx := &testContext{S: dbst}

	dbteststeps.RegisterSteps(dbst, s)

	s.Step(`^"([^"]*)" group$`, tctx.group)
	s.Step(`^open alive "([^"]*)" master in "([^"]*)" group$`, tctx.openAliveMasterInGroup)
	s.Step(`^closed alive "([^"]*)" master in "([^"]*)" group$`, tctx.closedAliveMasterInGroup)
	s.Step(`^"([^"]*)" minion in "([^"]*)" group$`, tctx.minionInGroup)
	s.Step(`^"([^"]*)" shipment on "([^"]*)"$`, tctx.shipmentOn)
}

type testContext struct {
	S *dbteststeps.DBSteps
}
