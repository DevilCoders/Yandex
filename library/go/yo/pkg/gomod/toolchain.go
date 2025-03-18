package gomod

import (
	"bytes"
	"encoding/json"
	"fmt"
	"os"
	"os/exec"
	"sync"

	"a.yandex-team.ru/library/go/yatool"
)

var (
	goBinaryPath  string
	toolchainOnce sync.Once
	goEnvErr      error
	goEnv         map[string]string
	goEnvOnce     sync.Once
)

func GoBinary() string {
	arcadiaGoPath := func() (string, error) {
		yaPath, err := yatool.Ya()
		if err != nil {
			return "", err
		}

		cmd := exec.Command(yaPath, "tool", "go", "--print-path")
		cmd.Stderr = os.Stderr
		goPathOut, err := cmd.Output()
		if err != nil {
			return "", err
		}

		goBinaryPath = string(bytes.TrimSpace(goPathOut))
		if _, err = os.Stat(goBinaryPath); err != nil {
			return "", err
		}

		return goBinaryPath, nil
	}

	toolchainOnce.Do(func() {
		var err error
		goBinaryPath, err = arcadiaGoPath()
		if err != nil {
			_, _ = fmt.Fprintf(os.Stderr, "yo: failed to get arcadia go path: %v\n", err)
			_, _ = fmt.Fprintln(os.Stderr, "yo: used system go")
			goBinaryPath = "go"
		}
	})

	return goBinaryPath
}

func GoEnv(key string) (string, error) {
	goEnvOnce.Do(initGoEnv)
	return goEnv[key], goEnvErr
}

func initGoEnv() {
	cmd := exec.Command(GoBinary(), "env", "-json")
	cmd.Stderr = os.Stderr
	goenvOut, err := cmd.Output()
	if err != nil {
		goEnvErr = err
		return
	}

	goEnvErr = json.Unmarshal(goenvOut, &goEnv)
}
