package call

import (
	"os"
	"os/exec"
	"syscall"
)

func terminateProcess(p *os.Process) error {
	// Killing -pid is not a bug, but Linux magic.
	//
	// In addition to sending a signal to a single PID,
	// kill(2) also supports sending a signal to a Process Group
	// by passing the process group id (PGID ) as a negative number.
	// https://medium.com/@felixge/killing-a-child-process-and-all-of-its-children-in-go-54079af94773
	return syscall.Kill(-p.Pid, syscall.SIGTERM)
}

func setProcessGroupID(cmd *exec.Cmd) {
	cmd.SysProcAttr = &syscall.SysProcAttr{Setpgid: true}
}
