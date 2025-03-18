//go:build !arcadia
// +build !arcadia

package certifi

// forced system cert pool when library is not built by ya make.
func init() {
	underYaMake = false
}
