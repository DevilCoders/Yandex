package main

import (
	"flag"
	"log"
	"net/http"
	"strconv"
	"time"

	"a.yandex-team.ru/library/go/yandex/unistat"
	"a.yandex-team.ru/library/go/yandex/unistat/aggr"
)

type server struct {
	latency  *unistat.Histogram
	requests *unistat.Numeric
	errors   *unistat.Numeric
}

func newServer() *server {
	serv := &server{
		latency: unistat.NewHistogram("latency", 0,
			aggr.Histogram(),
			[]float64{0, 25, 50, 75, 100, 150, 200, 250, 500, 1000}),
		requests: unistat.NewNumeric("requests", 1, aggr.Counter(), unistat.Sum),
		errors:   unistat.NewNumeric("errors", 1, aggr.Counter(), unistat.Sum),
	}

	unistat.Register(serv.latency)
	unistat.Register(serv.requests)
	unistat.Register(serv.errors)

	return serv
}

func (serv *server) UnistatHandle(rw http.ResponseWriter, req *http.Request) {
	defer unistat.MeasureMicrosecondsSince(serv.latency, time.Now())
	defer serv.requests.Update(1)

	bytes, err := unistat.MarshalJSON()
	if err != nil {
		http.Error(rw, err.Error(), http.StatusInternalServerError)
		defer serv.errors.Update(1)
		return
	}

	rw.Header().Set("Content-Type", "application/json; charset=utf-8")
	if _, err := rw.Write(bytes); err != nil {
		log.Print("write:", err)
	}
}

func main() {
	var listenPort int
	flag.IntVar(&listenPort, "port", 8080, "Listen port")
	flag.Parse()

	serv := newServer()

	http.HandleFunc("/unistat", serv.UnistatHandle)
	log.Fatal(http.ListenAndServe(":"+strconv.Itoa(listenPort), nil))
}
