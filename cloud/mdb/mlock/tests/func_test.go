package tests

import (
	"testing"

	"github.com/DATA-DOG/godog"

	"a.yandex-team.ru/cloud/mdb/internal/godogutil"
	"a.yandex-team.ru/library/go/test/yatest"
)

var featurePaths = []string{
	yatest.SourcePath("cloud/mdb/mlock/tests/features"),
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
	pair, err := NewTestContext(tc)
	if err != nil {
		panic(err)
	}
	pair.RegisterSteps(s)
}
