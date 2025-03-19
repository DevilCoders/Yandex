package valids

type uniqTester map[string]string

func (t uniqTester) check(what, where string) (bool, string) {
	if prev, ok := t[what]; ok {
		return false, prev
	}
	t[what] = where
	return true, ""
}

type stringPair [2]string

var setMark = struct{}{}
