package protoutil

import (
	"google.golang.org/protobuf/proto"
	"google.golang.org/protobuf/reflect/protoreflect"
	"google.golang.org/protobuf/types/descriptorpb"

	cloud "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv"
)

func Iterate(message protoreflect.Message, callbacks ...interface{}) {
	if !processMessage(message, callbacks...) {
		return
	}

	descriptor := message.Descriptor()
	for i := 0; i < descriptor.Fields().Len(); i++ {
		field := descriptor.Fields().Get(i)
		if !message.Has(field) {
			continue
		}

		if !processField(message, field, callbacks...) {
			continue
		}

		if field.Kind() == protoreflect.MessageKind {
			value := message.Get(field)
			if field.IsList() {
				list := value.List()
				for j := 0; j < list.Len(); j++ {
					Iterate(list.Get(j).Message(), callbacks...)
				}
			} else if field.IsMap() {
				if field.MapValue().Kind() != protoreflect.MessageKind {
					continue
				}
				value.Map().Range(func(key protoreflect.MapKey, value protoreflect.Value) bool {
					Iterate(value.Message(), callbacks...)
					return true
				})
			} else {
				Iterate(value.Message(), callbacks...)
			}
		}
	}
}

func ClearSensitiveData(message protoreflect.Message) {
	var callback FieldCallback = func(m protoreflect.Message, field protoreflect.FieldDescriptor) bool {
		if isSensitive(field) {
			m.Clear(field)
			return false
		}
		return true
	}
	Iterate(message, callback)
}

func isSensitive(field protoreflect.FieldDescriptor) bool {
	options := field.Options()
	if fieldOptions, ok := options.(*descriptorpb.FieldOptions); ok {
		extension := proto.GetExtension(fieldOptions, cloud.E_Sensitive)
		if sensitive, ok := extension.(bool); ok {
			return sensitive
		}
	}
	return false
}

type MessageCallback func(message protoreflect.Message) bool

func processMessage(message protoreflect.Message, callbacks ...interface{}) bool {
	for _, callback := range callbacks {
		messageCallback, ok := callback.(MessageCallback)
		if ok {
			if !messageCallback(message) {
				return false
			}
		}
	}
	return true
}

type FieldCallback func(message protoreflect.Message, field protoreflect.FieldDescriptor) bool

func processField(message protoreflect.Message, field protoreflect.FieldDescriptor, callbacks ...interface{}) bool {
	for _, callback := range callbacks {
		fieldCallback, ok := callback.(FieldCallback)
		if ok {
			if !fieldCallback(message, field) {
				return false
			}
		}
	}
	return true
}
