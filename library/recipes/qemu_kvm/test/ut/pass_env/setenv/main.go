package main

import (
	"a.yandex-team.ru/library/go/test/recipe"
)

type setEnv struct{}

func (r *setEnv) Start() error {
	recipe.SetEnv("CUSTOM_ENV", `"lol", 'kek' | cheburek`)
	return nil
}
func (r *setEnv) Stop() error {
	return nil
}
func main() {
	recipe.Run(&setEnv{})
}
