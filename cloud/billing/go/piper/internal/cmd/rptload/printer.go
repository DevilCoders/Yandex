package main

type printer interface {
	Println(i ...interface{})
	Printf(format string, i ...interface{})
}
