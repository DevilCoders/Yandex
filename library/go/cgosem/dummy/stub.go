//go:build !cgo
// +build !cgo

package dummy

import "runtime"

func F() {
	runtime.Gosched()
}
