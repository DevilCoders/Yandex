package godogutil_test

import (
	"testing"

	"github.com/DATA-DOG/godog"

	"a.yandex-team.ru/cloud/mdb/internal/godogutil"
)

func TestMakeSuiteFromFeatureMust(t *testing.T) {
	godogutil.MakeSuiteFromFeatureMust(thirdFeatureFile, func(s *godog.Suite) {}, t)
}
