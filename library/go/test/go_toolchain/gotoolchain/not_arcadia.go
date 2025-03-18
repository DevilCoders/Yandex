//go:build !arcadia
// +build !arcadia

package gotoolchain

func Setup(setEnv func(k, v string) error) error {
	// Assume go toolchain is available inside go test.
	return nil
}
