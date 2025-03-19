package marshal

import "github.com/valyala/fastjson"

var (
	parsers = fastjson.ParserPool{}
	arenas  = fastjson.ArenaPool{}
)
