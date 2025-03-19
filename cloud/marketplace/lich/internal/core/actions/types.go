package actions

type emptyResult struct{}

func (emptyResult) Empty() bool {
	return true
}
