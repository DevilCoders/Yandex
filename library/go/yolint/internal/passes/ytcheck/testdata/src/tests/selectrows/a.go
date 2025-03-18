package tests

import (
	"a.yandex-team.ru/yt/go/yt"
)

func f0(yc yt.Client) {
	rows, _ := yc.SelectRows() // want "reader.Err must be checked"

	defer func() {
		_ = rows.Close()
	}()
}

func f1(yc yt.Client) {
	rows, _ := yc.SelectRows()

	defer func() {
		_ = rows.Err()
	}()
}

func f2(yc yt.Client) {
	rows, err := yc.SelectRows() // OK
	if err != nil {
		// handle error
	}
	rowsS := rows
	_ = rowsS.Err()

	rows2, err := yc.SelectRows() // OK
	rowsX2 := rows2
	_ = rowsX2.Err()
	if err != nil {
		// handle error
	}
}

func f5(yc yt.Client) {
	_, err := yc.SelectRows() // want "reader.Err must be checked"
	if err != nil {
		// handle error
	}
}

func f6(yc yt.Client) {
	yc.SelectRows() // want "reader.Err must be checked"
}

func f7(yc yt.Client) {
	rows, _ := yc.SelectRows() // OK
	resCloser := func() error {
		return rows.Err()
	}
	_ = resCloser()
}

func f8(yc yt.Client) {
	rows, _ := yc.SelectRows() // want "reader.Err must be checked"
	_ = func() {
		rows.Close()
	}
}

func f9(yc yt.Client) {
	_ = func() {
		rows, _ := yc.SelectRows() // OK
		rows.Err()
	}
}

func f10(yc yt.Client) {
	rows, _ := yc.SelectRows()
	resCloser := func(tr yt.TableReader) {
		_ = tr.Err()
	}
	resCloser(rows)
}

func f11(yc yt.Client) yt.TableReader {
	rows, _ := yc.SelectRows() // OK
	return rows
}
