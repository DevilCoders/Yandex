package errcheck

import "fmt"

type t struct{}

func (x t) a() error {
	fmt.Println("this method returns an error")
	//line myfile.txt:100
	fmt.Println("this method also returns an error")
	return nil
}

type u struct {
	t t
}
