package reflectutil

import (
	"fmt"
	"reflect"
)

// ReverseMap creates a new map with reversed keys/values of the original map
func ReverseMap(forward interface{}) interface{} {
	rv := reflect.ValueOf(forward)
	rt := rv.Type()
	if rt.Kind() != reflect.Map {
		panic(fmt.Sprintf("provided value is not a map: %+v", rv))
	}

	backward := reflect.MakeMapWithSize(reflect.MapOf(rt.Elem(), rt.Key()), rv.Len())
	i := rv.MapRange()
	for i.Next() {
		backward.SetMapIndex(i.Value(), i.Key())
	}

	return backward.Interface()
}
