#!/bin/bash


get_mon_url () {
	local ret=$(curl -w "\n%{http_code}\n" -m 1 -s -k -H "Host: $1" https://localhost/monitor/"$2")
	local res=$(echo "$ret" | head -1)
	local code=$(echo "$ret" | tail -1)

	if [[ "$code" -ne "200" ]]; then
		echo
	else
		echo "$res"
	fi
}

mon () {
	local metric=$(echo "books.$1" | sed 's/\//-/g')

	sleep 1

	echo "$metric $2"
}

date_lag () {
	local date=$(date -d "$1" +'%s')
	local cur_date=$(date +'%s')

	echo $((cur_date - date))
}

val () {
	local ret=$(get_mon_url "$1" "$2")

	if [[ "$ret" ]]; then
		mon "$2" "$ret"
	fi
}

lag () {
	local ret=$(get_mon_url "$1" "$2")

	if [[ "$ret" ]] ; then
		local lag=$(date_lag "$ret");
		mon "$2" "$lag"
	fi
}

val "books-parser01d.books.yandex.net" "parse-queue-length"
val "crawler.books.yandex.net"         "freeze-metadata/count"
val "crawler.books.yandex.net"         "error-checkpoints/litres"
val "crawler.books.yandex.net"         "books-count"
val "crawler.books.yandex.net"         "removed-books-count"
val "crawler.books.yandex.net"         "authors-count"
val "crawler.books.yandex.net"         "checkpoints-count"
val "crawler.books.yandex.net"         "dumps-count"
val "crawler.books.yandex.net"         "dumps-length"
val "crawler.books.yandex.net"         "fails-count"
val "crawler.books.yandex.net"         "fails-length"
val "crawler.books.yandex.net"         "task-queue-length"
val "crawler.books.yandex.net"         "toplevel-task-queue-length"
val "crawler.books.yandex.net"         "available-books-count"

lag "crawler.books.yandex.net"         "last-success-download/litres"
lag "crawler.books.yandex.net"         "last-success-upload"
