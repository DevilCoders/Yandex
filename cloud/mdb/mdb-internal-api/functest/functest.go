package functest

import (
	"fmt"
	"os"
	"testing"

	"a.yandex-team.ru/cloud/mdb/internal/godogutil"
)

const (
	envNameSetupFeaturePaths = "_SETUP_GODOG_FEATURE_PATHS"
)

func Run(m *testing.M) {
	featurePaths, ok := os.LookupEnv(envNameSetupFeaturePaths)
	if !ok {
		panic(fmt.Sprintf("func tests configuration is invalid: env var %q must be set", envNameSetupFeaturePaths))
	}

	paths := godogutil.SplitFeaturePaths(featurePaths)
	tc := godogutil.NewTestContext(
		true,
		contextInitializer,
		godogutil.WithContextPerScenario(),
		godogutil.WithScenariosAsSubtests(),
	)
	godogutil.RunSuite(paths, tc, m)
}
