package uatraits

import "strings"

/*
Helper functions.
*/

func сontainsIgnoreCase(str, substr string) bool {
	return strings.Contains(strings.ToLower(str), strings.ToLower(substr))
}
