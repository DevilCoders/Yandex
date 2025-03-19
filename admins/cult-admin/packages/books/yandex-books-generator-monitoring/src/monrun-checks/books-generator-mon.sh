#!/bin/bash

get_local_url () {
	local res=$(curl -w "\n%{http_code}\n" -m 5 -s -k -H "Host: $1" https://localhost/"$2")

	local code=$(echo "$res" | tail -1)

	if [[ $code -ne "200" ]]; then
		echo "2; $1/$2: Error $code"
		exit 1
	else
		echo "$res" | head -n1
	fi
}

date_lag () {
	local date=$(date -d "$1" +'%s')

	local cur_date=$(date +'%s')

	echo $((cur_date - date))
}

lag_excess () {
	local host="$1"
	local knob="$2"
	local time="$3"
	local name="$4"

	res=$(get_local_url "$1" "$2") || { echo "$res"; exit; }

	lag=$(date_lag "$res")

	if [[ $lag -gt $time ]]; then
		echo "2;$name: lag too big ($lag sec)"
	else
		echo "0;$name: lag ok ($lag sec)"
	fi
}

size_excess () {
	local host="$1"
	local knob="$2"
	local size="$3"
	local name="$4"

	res=$(get_local_url "$1" "$2") || { echo "$res"; exit; }

	if [[ $res -gt $size ]]; then
		echo "2;$name: count is $res"
	else
		echo "0;$name: count is $res"
	fi
}

queue_size () {
	local host="$1"
	local knob="$2"
	local name="$3"

	local path="/tmp/books-generator-mon-$name"

	res=$(get_local_url "$1" "$2") || { echo "$res"; exit; }

	if [ ! -f $path ]; then
		echo "$res" > $path
		echo "1;$name: queue size = $res; Init ..."
		exit 0
	fi

	res_prev=$(cat "$path")
	res_prev=${res_prev%\\n}
	echo "$res" > $path

	diff=$(($res-$res_prev))

	if [[ $diff -gt 0 ]]; then
		echo "1;$name: queue size = $res; diff = $diff > 0"
	else
		echo "0;$name: queue size = $res; diff = $diff <= 0"
	fi
}


case "$1" in
	"lag_litres")
		lag_excess "crawler.books.yandex.net" \
			"monitor/last-success-download/litres" \
			$((60*15+60*3)) \
			"lag_litres"
		exit 0 ;;
	"lag_litres_upload")
		lag_excess "crawler.books.yandex.net" \
			"monitor/last-success-upload" \
			$((60*15+60*3)) \
			"lag_litres_upload"
		exit 0 ;;
	"parse_queue")
		size_excess "books-parser01d.books.yandex.net" \
			"monitor/parse-queue-length" \
			0 \
			"parse_queue"
		exit 0 ;;
	"freeze-metadata")
		size_excess "crawler.books.yandex.net" \
			"monitor/freeze-metadata/count" \
			1 \
			"freeze-metadata"
		exit 0 ;;
	"error-checkpoints")
		size_excess "crawler.books.yandex.net" \
			"monitor/error-checkpoints/litres" \
			1 \
			"error-checkpoints"
		exit 0 ;;
	"task-queue")
		queue_size "crawler.books.yandex.net" \
			"monitor/task-queue-length" \
			"task-queue"
		exit 0 ;;
	"toplevel-task-queue")
		queue_size "crawler.books.yandex.net" \
			"monitor/toplevel-task-queue-length" \
			"toplevel-task-queue"
		exit 0 ;;
esac
