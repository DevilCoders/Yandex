package functest

import (
	"testing"

	"a.yandex-team.ru/cloud/mdb/internal/godogutil"
	"a.yandex-team.ru/library/go/test/yatest"
)

var featurePaths = []string{
	yatest.SourcePath("cloud/mdb/mdb-search-producer/functest/features"),
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
