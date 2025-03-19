package shipments

import (
	"fmt"
	"strings"
)

func ShExecIfPresent(script string, args ...string) string {
	argsString := strings.Join(args, " ")
	return fmt.Sprintf("if test -f %s ; then %s %s; fi", script, script, argsString)
}
