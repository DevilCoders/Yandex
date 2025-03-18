package nilness

func Category() bool {
	var test []int
	if test == nil { // want `nilness: tautological condition: nil == nil`
		return true
	}
	return false
}
