package tools

import "fmt"

type StringSet map[string]struct{}

func NewStringSet(v ...string) StringSet {
	result := make(StringSet)
	result.Add(v...)
	return result
}

func CheckUniqStrings(v ...string) (StringSet, error) {
	result := make(StringSet)
	for _, vv := range v {
		if result.Contains(vv) {
			return result, fmt.Errorf("'%s' is duplicated value", vv)
		}
		result[vv] = setMark
	}
	return result, nil
}

func (s StringSet) Add(v ...string) {
	for _, vv := range v {
		s[vv] = setMark
	}
}

func (s StringSet) Remove(v ...string) {
	for _, vv := range v {
		delete(s, vv)
	}
}

func (s StringSet) Contains(value string) bool {
	_, exists := s[value]
	return exists
}

func (s StringSet) Items() []string {
	result := make([]string, 0, len(s))
	for i := range s {
		result = append(result, i)
	}
	return result
}

func (s StringSet) Intersect(other StringSet) StringSet {
	result := NewStringSet()
	for i := range s {
		if other.Contains(i) {
			result.Add(i)
		}
	}
	return result
}

func (s StringSet) Union(other StringSet) StringSet {
	result := NewStringSet()
	for i := range s {
		result.Add(i)
	}
	for i := range other {
		result.Add(i)
	}
	return result
}

func (s StringSet) Subtract(other StringSet) StringSet {
	result := NewStringSet()
	for i := range s {
		if !other.Contains(i) {
			result.Add(i)
		}
	}
	return result
}

var setMark = struct{}{}
