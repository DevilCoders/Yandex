package testutil

import (
	"os"
	"path/filepath"
	"testing"

	"a.yandex-team.ru/library/go/test/go_toolchain/gotoolchain"
	"a.yandex-team.ru/library/go/test/yatest"
)

var (
	GOPATH      string
	ArcadiaRoot string
)

func init() {
	if err := gotoolchain.Setup(os.Setenv); err != nil {
		panic(err)
	}

	// Workaround for https://github.com/golang/go/issues/29341
	if os.Getenv("HOME") == "" {
		_ = os.Setenv("HOME", yatest.WorkPath("fakehome"))
	}

	GOPATH, _ = filepath.Abs("testdata")
	st, err := os.Lstat(GOPATH)
	if err != nil {
		panic(err)
	}

	if st.Mode()&os.ModeSymlink != 0 {
		GOPATH, err = os.Readlink(GOPATH)
		if err != nil {
			panic(err)
		}
	}

	ArcadiaRoot = filepath.Join(GOPATH, "src", "a.yandex-team.ru")

	err = os.Chdir(ArcadiaRoot)
	if err != nil {
		panic(err)
	}
}

func NeedProxyAccess(t *testing.T) {
	_ = os.Setenv("GOPROXY", "http://kirby.sec.yandex.net")
}
