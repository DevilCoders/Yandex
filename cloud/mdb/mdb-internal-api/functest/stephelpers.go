package functest

import (
	"strconv"
	"strings"

	"a.yandex-team.ru/library/go/core/xerrors"
)

// Casts strValue to type of actualValue
func getTypedValue(strValue string, actualValue interface{}) (interface{}, error) {
	switch actualValue.(type) {
	default:
		return strValue, nil
	case int:
		return strconv.Atoi(strValue)
	case float32:
		return strconv.ParseFloat(strValue, 32)
	case float64:
		return strconv.ParseFloat(strValue, 64)
	case bool:
		return strconv.ParseBool(strValue)
	}
}

// Fills value inside map of maps (parsed json structure)
func fillValueAtPath(expected map[string]interface{}, path string, value interface{}) {
	tokens := strings.Split(path, ".")

	if tokens[0] == "$" {
		tokens = tokens[1:]
	}

	currElement := expected
	lastIndex := len(tokens) - 1
	for i, t := range tokens {
		index, t, err := getArrayIndex(t)

		isScalar := err != nil

		// Create new elements if absent
		if _, ok := currElement[t]; !ok {
			if isScalar {
				currElement[t] = make(map[string]interface{})
			} else {
				sliceOfMaps := make([]interface{}, index+1)
				for j := 0; j <= index; j++ {
					sliceOfMaps[j] = make(map[string]interface{})
				}
				currElement[t] = sliceOfMaps
			}
		}

		// Fill value or get next element
		if isScalar {
			if i == lastIndex {
				currElement[t] = value
			} else {
				currElement = currElement[t].(map[string]interface{})
			}
		} else {
			if i == lastIndex {
				currElement[t].([]interface{})[index] = value
			} else {
				currElement = currElement[t].([]interface{})[index].(map[string]interface{})
			}
		}
	}
}

func getArrayIndex(token string) (int, string, error) {
	from := strings.Index(token, "[")
	till := strings.Index(token, "]")

	if from <= 0 || till == -1 {
		return -1, token, xerrors.Errorf("token %s is not an array", token)
	}

	index, err := strconv.Atoi(token[from+1 : till])

	return index, token[:from], err
}
