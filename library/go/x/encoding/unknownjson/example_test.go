package unknownjson_test

import (
	"fmt"

	"a.yandex-team.ru/library/go/x/encoding/unknownjson"
)

func Example() {
	var s struct {
		A int `json:"a"`

		Unknown unknownjson.Store `json:"-" unknown:",store"`
	}

	js := []byte(`{"a":1,"b":2}`)
	if err := unknownjson.Unmarshal(js, &s); err != nil {
		panic(err)
	}

	var err error
	js, err = unknownjson.Marshal(s)
	if err != nil {
		panic(err)
	}

	fmt.Println(string(js))
	// Output:
	// {"a":1,"b":2}
}
