package golist

import (
	"fmt"
	"os"
	"os/exec"
	"strings"

	"a.yandex-team.ru/library/go/yoimports/pkg/imports/resolver"
)

var _ resolver.Resolver = (*Resolver)(nil)
var goArgs = []string{
	"list", "-e", "-compiled=false", "-test=false", "-export=false", "-deps=false", "-find=true", "-f={{.Name}}|",
	"--",
}

type Resolver struct {
	dir string
}

func NewResolver(opts ...Option) *Resolver {
	var r Resolver
	for _, opt := range opts {
		opt(&r)
	}
	return &r
}

func (r *Resolver) ResolvePackages(paths ...string) ([]resolver.Package, error) {
	cmd := exec.Command("go", append(goArgs, paths...)...)
	cmd.Dir = r.dir

	var stdout, stderr strings.Builder
	cmd.Stdout = &stdout
	cmd.Stderr = &stderr
	cmd.Env = []string{"GOPROXY=off"}
	for _, env := range os.Environ() {
		if strings.HasPrefix(env, "GOPROXY") {
			continue
		}

		cmd.Env = append(cmd.Env, env)
	}

	if err := cmd.Run(); err != nil {
		return nil, fmt.Errorf("go list fail: %v\n%s", err, stderr.String())
	}

	lines := strings.SplitN(stdout.String(), "\n", len(paths))
	if len(lines) != len(paths) {
		return nil, fmt.Errorf("unexpected output: %s", stdout.String())
	}

	out := make([]resolver.Package, len(paths))
	for i, line := range lines {
		out[i] = resolver.Package{
			Name: strings.Trim(line, "|\n\x20\t"),
		}
	}

	return out, nil
}
