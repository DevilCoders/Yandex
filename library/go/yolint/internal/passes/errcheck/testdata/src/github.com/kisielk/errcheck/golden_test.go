package errcheck

import (
	"bytes"
	"crypto/sha256"
	"fmt"
	"io/ioutil"
	"math/rand"
	mrand "math/rand"
)

func a() error {
	fmt.Println("this function returns an error")
	return nil
}

func b() (int, error) {
	fmt.Println("this function returns an int and an error")
	return 0, nil
}

func c() int {
	fmt.Println("this function returns an int")
	return 7
}

func rec() {
	defer func() {
		recover() // want `unhandled potential recover error`
		_ = recover()
	}()
	defer recover() // want `unhandled potential recover error`
}

type MyError string

func (e MyError) Error() string {
	return string(e)
}

func customError() error {
	return MyError("an error occurred")
}

func customConcreteError() MyError {
	return MyError("an error occurred")
}

func customConcreteErrorTuple() (int, MyError) {
	return 0, MyError("an error occurred")
}

type MyPointerError string

func (e *MyPointerError) Error() string {
	return string(*e)
}

func customPointerError() *MyPointerError {
	e := MyPointerError("an error occurred")
	return &e
}

func customPointerErrorTuple() (int, *MyPointerError) {
	e := MyPointerError("an error occurred")
	return 0, &e
}

// Test custom excludes
type ErrorMakerInterface interface {
	MakeNilError() error
}
type ErrorMakerInterfaceWrapper interface {
	ErrorMakerInterface
}

func main() {
	// Single error return
	_ = a()
	a() // want `unhandled error`

	// Return another value and an error
	_, _ = b()
	b() // want `unhandled error`

	// Return a custom error type
	_ = customError()
	customError() // want `unhandled error`

	// Return a custom concrete error type
	_ = customConcreteError()
	customConcreteError()
	_, _ = customConcreteErrorTuple()
	customConcreteErrorTuple()

	// Return a custom pointer error type
	_ = customPointerError()
	customPointerError()
	_, _ = customPointerErrorTuple()
	customPointerErrorTuple()

	// Method with a single error return
	x := t{}
	_ = x.a()
	x.a() // want `unhandled error`

	// Method call on a struct member
	y := u{x}
	_ = y.t.a()
	y.t.a() // want `unhandled error`

	m1 := map[string]func() error{"a": a}
	_ = m1["a"]()
	m1["a"]() // want `unhandled error`

	// Additional cases for assigning errors to blank identifier
	z, _ := b()
	_, w := a(), 5

	// Assign non error to blank identifier
	_ = c()

	_ = z + w // Avoid complaints about unused variables

	// Type assertions
	var i interface{}
	s1 := i.(string)    // ASSERT
	s1 = i.(string)     // ASSERT
	s2, _ := i.(string) // ASSERT
	s2, _ = i.(string)  // ASSERT
	s3, ok := i.(string)
	s3, ok = i.(string)
	switch s4 := i.(type) {
	case string:
		_ = s4
	}
	_, _, _, _ = s1, s2, s3, ok

	// Goroutine
	go a() // want `unhandled error`
	defer a()

	b1 := bytes.Buffer{}
	b2 := &bytes.Buffer{}
	b1.Write(nil)
	b2.Write(nil)
	rand.Read(nil)
	mrand.Read(nil)

	sha256.New().Write([]byte{})

	ioutil.ReadFile("main.go") // want `unhandled error`

	var emiw ErrorMakerInterfaceWrapper
	emiw.MakeNilError() // want `unhandled error`
}
