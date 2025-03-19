package mysqlutil

import (
	"fmt"
	"strconv"
	"strings"
)

func calcBinlog(bl string, inc int) string {
	errMsg := fmt.Sprintf("unexpected binlog name: %s", bl)
	idx := strings.LastIndexByte(bl, byte('.'))
	if idx < 2 || idx >= len(bl)-1 {
		panic(errMsg)
	}
	idx++ // first digit in suffix
	num, err := strconv.Atoi(bl[idx:])
	if err != nil {
		panic(errMsg)
	}
	if num == 0 && inc < 0 {
		return bl
	}
	return fmt.Sprintf("%s%06d", bl[:idx], num+inc)
}

func NextBinlog(bl string) string {
	return calcBinlog(bl, 1)
}

func PrevBinlog(bl string) string {
	return calcBinlog(bl, -1)
}
