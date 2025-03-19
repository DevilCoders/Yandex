package godogutil

import (
	"fmt"
	"os"
	"path"
	"strconv"
	"strings"
	"testing"

	"github.com/DATA-DOG/godog"

	"a.yandex-team.ru/cloud/mdb/internal/app/signals"
	"a.yandex-team.ru/library/go/test/yatest"
)

// StopOnFailure define whenever we should not start new tests after the first test failure
func StopOnFailure() bool {
	for _, args := range os.Args[1:] {
		if args == "-failfast" {
			return true
		}
	}
	// sadly, but it looks like
	// that 'ya make --fail-fast' doesn't proxy -failfast to `go test`
	if _, ok := os.LookupEnv("GODOG_STOP_ON_FAILURE"); ok {
		return true
	}
	return false
}

// MakeSuiteFromFeatureMust
func MakeSuiteFromFeatureMust(pathToFeatureFile string, contextInitializer func(s *godog.Suite), t *testing.T) {
	featuresList := ListFeaturesMust([]string{pathToFeatureFile})
	if len(featuresList) != 1 {
		t.Fatalf("Got unexpected features count: %d", len(featuresList))
	}
	feature := featuresList[0]
	for _, scenario := range feature.Scenarios {
		t.Run(scenario.Name, func(t *testing.T) {
			status := godog.RunWithOptions(
				"",
				contextInitializer,
				godog.Options{
					Format:        "pretty",
					NoColors:      true,
					Strict:        true,
					StopOnFailure: StopOnFailure(),
					Paths:         []string{feature.Path + ":" + strconv.Itoa(scenario.LineNo)},
				})

			if status > 0 {
				t.Fatalf("scenario %s [from feature %s]. Exit with code %d", scenario.Name, feature.Name, status)
			}
		})
	}
}

const (
	envNameTags         = "GODOG_TAGS"
	envNameFeatureTags  = "GODOG_FEATURE_TAGS"
	envNameFeaturePaths = "GODOG_FEATURE_PATHS"
	envNameHoldOn       = "GODOG_HOLD_ON"
)

const (
	envHoldOnValueExit  = "exit"
	envHoldOnValueError = "error"
)

var holdOnValues = map[string]struct{}{
	envHoldOnValueExit:  {},
	envHoldOnValueError: {},
}

func RunSuite(featurePaths []string, tc *TestContext, m *testing.M) {
	featurePaths = FeaturePaths(featurePaths)
	featureTags := os.Getenv(envNameFeatureTags)
	tags := os.Getenv(envNameTags)
	ListTests(featurePaths, featureTags)

	if len(tags) == 0 || len(featureTags) == 0 {
		tags = tags + featureTags
	} else {
		tags = featureTags + " && " + tags
	}

	status := godog.RunWithOptions("godog", func(s *godog.Suite) {
		tc.godogInit(s)
	}, godog.Options{
		Strict:        tc.strict,
		StopOnFailure: StopOnFailure(),
		NoColors:      true,
		Tags:          tags,
		Format:        "pretty",
		Paths:         featurePaths,
	})

	if st := m.Run(); st > status {
		status = st
	}

	HoldOn(status)

	os.Exit(status)
}

func holdOnMode() (string, bool) {
	v, ok := os.LookupEnv(envNameHoldOn)
	if !ok {
		return "", false
	}

	v = strings.ToLower(v)
	if _, ok := holdOnValues[v]; !ok {
		panic(fmt.Sprintf("invalid %q value: %s", envNameHoldOn, v))
	}

	return v, true
}

// HoldOn checks if we are required to hold execution and wait
func HoldOn(status int) {
	mode, ok := holdOnMode()
	if !ok {
		return
	}

	switch mode {
	case envHoldOnValueExit:
	case envHoldOnValueError:
		if status == 0 {
			return
		}
	}

	signals.WaitForStop()
}

func HoldOnError() {
	mode, ok := holdOnMode()
	if !ok {
		return
	}

	if mode != envHoldOnValueError {
		return
	}

	signals.WaitForStop()
}

// FeaturePaths returns either supplied list of paths or whatever is set in envNameFeaturePaths delimited by ';'
// This works ONLY on local machines and paths MUST be absolute
func FeaturePaths(paths []string) []string {
	v, ok := os.LookupEnv(envNameFeaturePaths)
	if !ok {
		return paths
	}

	return SplitFeaturePaths(v)
}

func SplitFeaturePaths(unsplit string) []string {
	paths := strings.Split(unsplit, ";")
	for i := range paths {
		if !path.IsAbs(paths[i]) {
			paths[i] = yatest.SourcePath(paths[i])
		}
	}

	return paths
}
