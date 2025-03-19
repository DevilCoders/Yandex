package tests

import (
	"os"
	"testing"

	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/cloud/billing/go/pkg/selfsku/resolving"
	"a.yandex-team.ru/cloud/billing/go/pkg/selfsku/valids"
	"a.yandex-team.ru/library/go/test/yatest"
)

var (
	dataPath = yatest.SourcePath("cloud/billing/bootstrap/internal")
	dataDir  = os.DirFS(dataPath)
)

func TestItems(t *testing.T) {
	vs := valids.ItemsSuite{DataDir: dataDir}
	suite.Run(t, &vs)
}

func TestBundles(t *testing.T) {
	vs := valids.BundlesSuite{DataDir: dataDir}
	suite.Run(t, &vs)
}

func TestDefinitions(t *testing.T) {
	vs := valids.DefinitionsSuite{DataDir: dataDir}
	suite.Run(t, &vs)
}

func TestResolving(t *testing.T) {
	rs := resolving.ResolvingTestSuite{DataDir: dataDir}
	suite.Run(t, &rs)
}
