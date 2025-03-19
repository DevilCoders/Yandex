package states

import "reflect"

var zeroValue = reflect.Value{}

func getHealthChecker(so StateObject) healthChecker {
	if hc, ok := so.(healthChecker); ok {
		return hc
	}
	return nil
}

func getStatsProvider(so StateObject) statsProvider {
	v := reflect.ValueOf(so)
	statsMth := v.MethodByName("GetStats")
	if statsMth == zeroValue {
		return nil
	}
	t := statsMth.Type()
	if t.Kind() != reflect.Func || t.NumIn() != 0 || t.NumOut() != 1 {
		return nil
	}

	return func() interface{} {
		res := statsMth.Call(nil)
		return res[0].Interface()
	}
}

func getCloseCall(so StateObject) closeCall {
	switch cls := so.(type) {
	case interface{ Close() }:
		return func() error { cls.Close(); return nil }
	case interface{ Close() error }:
		return cls.Close
	}
	return nil
}
