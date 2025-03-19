package util

import (
	"bytes"
	"encoding/binary"
	"fmt"
	"io"
)

// ReadBigEndian BigEndian Reader shortcut
func ReadBigEndian(r io.Reader, v interface{}) error {
	return binary.Read(r, binary.BigEndian, v)
}

// ReadASet reads a number of fields
func ReadASet(r io.Reader, fields ...interface{}) error {
	for _, field := range fields {
		err := binary.Read(r, binary.BigEndian, field)
		if err != nil {
			return err
		}
	}
	return nil
}

func StreamToByte(stream io.Reader) []byte {
	buf := new(bytes.Buffer)
	_, err := buf.ReadFrom(stream)
	if err != nil {
		fmt.Printf("StreamToByte(): %v", err)
	}
	return buf.Bytes()
}

func StreamToString(stream io.Reader) string {
	buf := new(bytes.Buffer)
	_, err := buf.ReadFrom(stream)
	if err != nil {
		fmt.Printf("StreamToString(): %v", err)
	}
	return buf.String()
}
