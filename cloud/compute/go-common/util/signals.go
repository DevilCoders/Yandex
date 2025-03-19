package util

import (
	"os"
	"os/signal"
	"syscall"
)

func IgnoreSigpipe() {
	/*
		Usual signal.Ignore(syscall.SIGPIPE) not really ignore sigpipe - it is stop notify only.
		https://golang.org/pkg/os/signal/#hdr-SIGPIPE
		https://github.com/cybozu-go/cmd/commit/3c4042e69465851fd5f6c0972467a45008c54545#diff-5b2af823fb9cee642563474e9c5be0e2R11

		For real ignore SIGPIPE the signal must be handled by notify.
	*/

	c := make(chan os.Signal, 1)
	signal.Notify(c, syscall.SIGPIPE)
}
