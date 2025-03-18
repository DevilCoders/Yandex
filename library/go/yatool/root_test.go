package yatool_test

import (
	"io/ioutil"
	"os"
	"path/filepath"
	"testing"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/library/go/yatool"
)

func TestFindRootInRoot(t *testing.T) {
	expectedRoot, err := filepath.Abs(filepath.Join("testdata", "mini_arcadia"))
	if !assert.NoError(t, err) {
		return
	}

	actualRoot, err := yatool.FindArcadiaRoot(expectedRoot)
	if !assert.NoError(t, err) {
		return
	}

	assert.Equal(t, expectedRoot, actualRoot)
}

func TestFindRootInNestedProject(t *testing.T) {
	expectedRoot, err := filepath.Abs(filepath.Join("testdata", "mini_arcadia"))
	if !assert.NoError(t, err) {
		return
	}

	projectPath, err := filepath.Abs(filepath.Join("testdata", "mini_arcadia", "test", "nested"))
	if !assert.NoError(t, err) {
		return
	}

	actualRoot, err := yatool.FindArcadiaRoot(projectPath)
	if !assert.NoError(t, err) {
		return
	}

	assert.Equal(t, expectedRoot, actualRoot)
}

func TestFindRootFail(t *testing.T) {
	fakeArcadia, err := ioutil.TempDir("", "arc-root")
	if !assert.NoError(t, err) {
		return
	}

	defer func() {
		_ = os.RemoveAll(fakeArcadia)
	}()

	_, err = yatool.FindArcadiaRoot(fakeArcadia)
	assert.Error(t, err)
}
