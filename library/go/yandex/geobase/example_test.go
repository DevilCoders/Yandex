package geobase_test

import (
	"fmt"

	"a.yandex-team.ru/library/go/yandex/geobase"
)

func Example() {
	// Create geobase instance once during application startup.
	g, err := geobase.New()
	if err != nil {
		panic(err)
	}

	// Do not forget to destroy C++ object.
	defer g.Destroy()

	r, err := g.GetRegionByIP("77.88.8.8")
	if err != nil {
		panic(err)
	}

	fmt.Printf("you are from %s", r.Name)
}
