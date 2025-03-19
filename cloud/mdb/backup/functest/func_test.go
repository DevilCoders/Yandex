package functest

import (
	"testing"

	"a.yandex-team.ru/cloud/mdb/internal/godogutil"
	"a.yandex-team.ru/library/go/test/yatest"
)

var featurePaths = []string{
	yatest.SourcePath("cloud/mdb/backup/functest/features/mysql"),
	yatest.SourcePath("cloud/mdb/backup/functest/features/common"),
	yatest.SourcePath("cloud/mdb/backup/functest/features/mongodb"),
	yatest.SourcePath("cloud/mdb/backup/functest/features/postgresql"),
	yatest.SourcePath("cloud/mdb/backup/functest/features/clickhouse"),
}

func TestMain(m *testing.M) {
	tc := godogutil.NewTestContext(
		true,
		ContextInitializer,
		godogutil.WithContextPerScenario(),
		godogutil.WithScenariosAsSubtests(),
	)
	godogutil.RunSuite(featurePaths, tc, m)
}
