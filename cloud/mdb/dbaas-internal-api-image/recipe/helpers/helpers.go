package helpers

import (
	"fmt"
	"os"
	"path"

	"a.yandex-team.ru/library/go/core/xerrors"
)

const (
	envNameTmpRoot  = "DBAAS_INTERNAL_API_RECIPE_TMP_ROOT"
	envNameLogsRoot = "DBAAS_INTERNAL_API_RECIPE_LOGS_ROOT"
)

func TmpRootPath(p string) (string, bool) {
	v, ok := os.LookupEnv(envNameTmpRoot)
	if !ok {
		return "", false
	}

	return path.Join(v, p), true
}

func MustTmpRootPath(p string) string {
	root, ok := TmpRootPath(p)
	if !ok {
		panic(fmt.Sprintf("dbaas internal api recipe tmp root path missing in env: %s", envNameTmpRoot))
	}

	return root
}

func LogsRootPath(p string) (string, bool) {
	v, ok := os.LookupEnv(envNameLogsRoot)
	if !ok {
		return "", false
	}

	return path.Join(v, p), true
}

func MustLogsRootPath(p string) string {
	root, ok := LogsRootPath(p)
	if !ok {
		panic(fmt.Sprintf("dbaas internal api recipe logs root path missing in env: %s", envNameLogsRoot))
	}

	return root
}

func CleanupTmpRootPath(p string) error {
	if err := os.RemoveAll(MustTmpRootPath(p)); err != nil {
		return xerrors.Errorf("failed to cleanup tmp root dir: %w", err)
	}
	return nil
}
