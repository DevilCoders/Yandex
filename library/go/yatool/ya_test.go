package yatool_test

import (
	"io/ioutil"
	"os"
	"path/filepath"
	"testing"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/library/go/yatool"
)

func TestYaPathInRoot(t *testing.T) {
	expectedYa, err := filepath.Abs(filepath.Join("testdata", "mini_arcadia", "ya"))
	if !assert.NoError(t, err) {
		return
	}

	actualYa, err := yatool.FindYa(filepath.Join("testdata", "mini_arcadia"))
	if !assert.NoError(t, err) {
		return
	}

	assert.Equal(t, expectedYa, actualYa)
}

func TestYaPathInNestedProject(t *testing.T) {
	expectedYaPath, err := filepath.Abs(filepath.Join("testdata", "mini_arcadia", "ya"))
	if !assert.NoError(t, err) {
		return
	}

	projectPath, err := filepath.Abs(filepath.Join("testdata", "mini_arcadia", "test", "nested"))
	if !assert.NoError(t, err) {
		return
	}

	actualYaPath, err := yatool.FindYa(projectPath)
	if !assert.NoError(t, err) {
		return
	}

	assert.Equal(t, expectedYaPath, actualYaPath)
}

func TestYaPathFail(t *testing.T) {
	fakeArcadia, err := ioutil.TempDir("", "arc-root")
	if !assert.NoError(t, err) {
		return
	}

	defer func() {
		_ = os.RemoveAll(fakeArcadia)
	}()

	_, err = yatool.FindYa(fakeArcadia)
	assert.Error(t, err)
}
