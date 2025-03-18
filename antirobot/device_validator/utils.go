package main

import (
	"bufio"
	"encoding/base64"
	"errors"
	"os"
	"strconv"
	"strings"
)

// This and BoolPtr are needed for protobuf boolean fields.
var TRUE bool = true
var FALSE bool = false

func ParseFingerprint(s string) ([]byte, error) {
	var fingerprint []byte

	byteStrs := strings.Split(s, ":")
	for _, byteStr := range byteStrs {
		b, err := strconv.ParseUint(byteStr, 0x10, 8)
		if err != nil {
			return nil, err
		}

		fingerprint = append(fingerprint, byte(b))
	}

	return fingerprint, nil
}

func GetJSONBytes(value interface{}) ([]byte, error) {
	encodedBytes, ok := value.(string)
	if !ok {
		return nil, errors.New("invalid type")
	}

	bytes, err := base64.StdEncoding.DecodeString(encodedBytes)
	if err != nil {
		return nil, err
	}

	return bytes, nil
}

func ErrorStringPtr(err error) *string {
	if err != nil {
		errString := err.Error()
		return &errString
	}

	return nil
}

func BoolPtr(x bool) *bool {
	if x {
		return &TRUE
	} else {
		return &FALSE
	}
}

func ReadLines(path string) ([]string, error) {
	file, err := os.Open(path)
	if err != nil {
		return []string{}, err
	}

	scanner := bufio.NewScanner(file)
	lines := []string{}

	for i := 0; scanner.Scan(); i += 1 {
		line := scanner.Text()
		if len(line) == 0 {
			continue
		}

		lines = append(lines, line)
	}

	return lines, nil
}
