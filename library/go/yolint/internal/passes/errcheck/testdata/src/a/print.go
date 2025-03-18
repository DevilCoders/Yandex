package a

import (
	"bytes"
	"fmt"
	"hash"
	"strings"
)

func printFalsePositives() {
	var buf bytes.Buffer
	var str strings.Builder
	var h hash.Hash

	fmt.Fprintf(&buf, "ok")
	fmt.Fprintf(&str, "ok")
	fmt.Fprintf(h, "ok")

	fmt.Fprint(&buf, "ok")
	fmt.Fprint(&str, "ok")
	fmt.Fprint(h, "ok")

	fmt.Fprintln(&buf, "ok")
	fmt.Fprintln(&str, "ok")
	fmt.Fprintln(h, "ok")
}
