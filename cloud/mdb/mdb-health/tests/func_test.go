package tests

import (
	"testing"

	"github.com/DATA-DOG/godog"
	"github.com/alicebob/miniredis/v2"

	"a.yandex-team.ru/cloud/mdb/internal/godogutil"
	"a.yandex-team.ru/library/go/test/yatest"
)

var featurePaths = []string{
	yatest.SourcePath("cloud/mdb/mdb-health/tests/features"),
}

func TestMain(m *testing.M) {
	minired, err := miniredis.Run()
	if err != nil {
		panic(err)
	}
	defer minired.Close()
	tc := godogutil.NewTestContext(
		true,
		func(tc *godogutil.TestContext, s *godog.Suite) {
			contextInitializer(tc, s, minired)
		},
		godogutil.WithContextPerScenario(),
		godogutil.WithScenariosAsSubtests(),
	)
	godogutil.RunSuite(featurePaths, tc, m)
}

func contextInitializer(tc *godogutil.TestContext, s *godog.Suite, minired *miniredis.Miniredis) {
	tstc, err := NewTestContext(tc, minired)
	if err != nil {
		panic(err)
	}
	tstc.RegisterSteps(s)
}
