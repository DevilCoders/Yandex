package dockerprocess

import (
	"bytes"
	"encoding/json"
	"fmt"
	"io/ioutil"
	"strings"

	errors "golang.org/x/xerrors"
)

// takes a local seccomp daemon, reads the file contents for sending to the daemon
// copy from https://github.com/docker/cli/blob/8ef8547eb6934b28497d309d21e280bcd25145f5/cli/command/container/opts.go#L839-L864
func parseSecurityOpts(securityOpts []string) ([]string, error) {
	for key, opt := range securityOpts {
		con := strings.SplitN(opt, "=", 2)
		if len(con) == 1 && con[0] != "no-new-privileges" {
			if strings.Contains(opt, ":") {
				con = strings.SplitN(opt, ":", 2)
			} else {
				return securityOpts, errors.Errorf("Invalid --security-opt: %q", opt)
			}
		}
		if con[0] == "seccomp" && con[1] != "unconfined" {
			f, err := ioutil.ReadFile(con[1])
			if err != nil {
				return securityOpts, errors.Errorf("opening seccomp profile (%s) failed: %v", con[1], err)
			}
			b := bytes.NewBuffer(nil)
			if err := json.Compact(b, f); err != nil {
				return securityOpts, errors.Errorf("compacting json for seccomp profile (%s) failed: %v", con[1], err)
			}
			securityOpts[key] = fmt.Sprintf("seccomp=%s", b.Bytes())
		}
	}

	return securityOpts, nil
}

// parseSystemPaths checks if `systempaths=unconfined` security option is set,
// and returns the `MaskedPaths` and `ReadonlyPaths` accordingly. An updated
// list of security options is returned with this option removed, because the
// `unconfined` option is handled client-side, and should not be sent to the
// daemon.
// copy from https://github.com/docker/cli/blob/8ef8547eb6934b28497d309d21e280bcd25145f5/cli/command/container/opts.go#L871-L883
func parseSystemPaths(securityOpts []string) (filtered, maskedPaths, readonlyPaths []string) {
	filtered = securityOpts[:0]
	for _, opt := range securityOpts {
		if opt == "systempaths=unconfined" {
			maskedPaths = []string{}
			readonlyPaths = []string{}
		} else {
			filtered = append(filtered, opt)
		}
	}

	return filtered, maskedPaths, readonlyPaths
}
