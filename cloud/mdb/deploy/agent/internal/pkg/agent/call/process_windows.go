package call

import (
	"os"
	"os/exec"
)

func terminateProcess(p *os.Process) error {
	return p.Kill()
}

func setProcessGroupID(cmd *exec.Cmd) {
}
