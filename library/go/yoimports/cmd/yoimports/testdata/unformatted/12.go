package yoimports

import (

	_ "fmt" // std

	// local: start
	_ "a.yandex-team.ru/c/d" // local
	/*
	some
	multiline

	comment (oops)
	 */
	_ "google.golang.org/a/b" // 3rd-party

	_ "google.golang.org/c/d" // 3rd-party
	_ "a.yandex-team.ru/a/b" // local
)
