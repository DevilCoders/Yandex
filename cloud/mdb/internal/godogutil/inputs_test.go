package godogutil_test

import (
	"path"

	"a.yandex-team.ru/library/go/test/yatest"
)

var (
	featuresPath      = yatest.SourcePath("cloud/mdb/internal/godogutil/features")
	oneFeaturePath    = yatest.SourcePath("cloud/mdb/internal/godogutil/features/one")
	twoFeaturesPath   = yatest.SourcePath("cloud/mdb/internal/godogutil/features/two")
	nonExistentPath   = yatest.SourcePath("cloud/mdb/utils/pkg/godougutil/Not/Existed/Path/Why/That/Should/Be/Present?")
	firstFeatureFile  = path.Join(oneFeaturePath, "first.feature")
	secondFeatureFile = path.Join(twoFeaturesPath, "second.feature")
	thirdFeatureFile  = path.Join(twoFeaturesPath, "third.feature")
)
