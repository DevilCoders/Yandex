package godogutil_test

import (
	"testing"

	"github.com/DATA-DOG/godog"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/godogutil"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func TestListFeatures(t *testing.T) {
	t.Run("return features for features", func(t *testing.T) {
		features, err := godogutil.ListFeatures([]string{featuresPath}, "")
		require.NoError(t, err)
		require.Lenf(t, features, 3, "we have 3 features here")
	})
	t.Run("return feature for direct path to feature", func(t *testing.T) {
		features, err := godogutil.ListFeatures([]string{oneFeaturePath}, "")
		require.NoError(t, err)
		require.Lenf(t, features, 1, "we have 1 features here")
	})
	t.Run("return all features from list", func(t *testing.T) {
		features, err := godogutil.ListFeatures(
			[]string{
				featuresPath,
				oneFeaturePath,
				twoFeaturesPath,
				firstFeatureFile,
				secondFeatureFile,
				thirdFeatureFile,
			}, "")
		require.NoError(t, err)
		require.Lenf(t, features, 3, "we have 3 features here")
	})
	t.Run("return error for non-existent path", func(t *testing.T) {
		_, err := godogutil.ListFeatures([]string{nonExistentPath}, "")
		require.Error(t, err)
	})
	t.Run("return one scenario for feature with one scenario", func(t *testing.T) {
		f, err := godogutil.ListFeatures([]string{secondFeatureFile}, "")
		require.NoError(t, err)
		require.Len(t, f, 1)
		require.Equal(
			t,
			[]godogutil.ScenarioDefinition{
				{
					Name:   "First scenario in second feature",
					LineNo: 3,
				},
			},
			f[0].Scenarios,
		)
	})
	t.Run("return 2 scenarios for feature with 2 scenarios", func(t *testing.T) {
		f, err := godogutil.ListFeatures([]string{firstFeatureFile}, "")
		require.NoError(t, err)
		require.Len(t, f, 1)
		require.Equal(
			t,
			[]godogutil.ScenarioDefinition{
				{
					Name:   "First scenario in first feature",
					LineNo: 4,
				},
				{
					Name:   "Second scenario in first feature",
					LineNo: 8,
				},
			},
			f[0].Scenarios,
		)
	})
	t.Run("return empty scenarios for empty feature", func(t *testing.T) {
		f, err := godogutil.ListFeatures([]string{thirdFeatureFile}, "")
		require.NoError(t, err)
		require.Len(t, f, 1)
		require.Empty(t, f[0].Scenarios)
	})
	t.Run("return feature with @ok tag", func(t *testing.T) {
		features, err := godogutil.ListFeatures([]string{featuresPath}, "@ok")
		require.NoError(t, err)
		require.Lenf(t, features, 1, "we have 1 feature here")
	})
	t.Run("return features without @not_ok tag", func(t *testing.T) {
		features, err := godogutil.ListFeatures([]string{featuresPath}, "~@not_ok")
		require.NoError(t, err)
		require.Lenf(t, features, 2, "we have 2 features here")
	})
	t.Run("return features with @not_ok or @ok tag", func(t *testing.T) {
		features, err := godogutil.ListFeatures([]string{featuresPath}, "@not_ok,@ok")
		require.NoError(t, err)
		require.Lenf(t, features, 2, "we have 2 features here")
	})
	t.Run("return feature without tags", func(t *testing.T) {
		features, err := godogutil.ListFeatures([]string{featuresPath}, "~@not_ok && ~@ok")
		require.NoError(t, err)
		require.Lenf(t, features, 1, "we have 1 feature here")
	})
}

func TestListFeaturesMust(t *testing.T) {
	t.Run("return features for features", func(t *testing.T) {
		require.NotEmpty(t, godogutil.ListFeaturesMust([]string{featuresPath}))
	})
	t.Run("panics on error for bad feature paths", func(t *testing.T) {
		require.Panics(t, func() { godogutil.ListFeaturesMust([]string{nonExistentPath}) })
	})
}

type testsContext struct {
	runCount int
}

func (t *testsContext) iRunning() error {
	t.runCount++
	return nil
}

func (t *testsContext) iShouldBeExecutedAt(num int) error {
	if t.runCount != num {
		return xerrors.Errorf("Expected I executed at %d position, by current position is %d", num, t.runCount)
	}
	return nil
}

func FeatureContext(s *godog.Suite) {
	tc := testsContext{}
	s.Step(`^I running$`, tc.iRunning)
	s.Step(`^I should be executed at "(\d+)"$`, tc.iShouldBeExecutedAt)
}

// Create test for each feature file
func TestGodogSampleIntegration(t *testing.T) {
	features := godogutil.ListFeaturesMust([]string{featuresPath})

	for _, ff := range features {
		t.Run(ff.Name, func(t *testing.T) {

			status := godog.RunWithOptions(
				"",
				func(s *godog.Suite) { FeatureContext(s) },
				godog.Options{
					Format:   "pretty",
					NoColors: false,
					Paths:    []string{ff.Path},
					Strict:   true, // we wan fail on not implement steps
				})

			if status > 0 {
				t.Fatalf("%q %s feature fail. With exit code %d", ff.Name, ff.Path, status)
			}
		})
	}

}
