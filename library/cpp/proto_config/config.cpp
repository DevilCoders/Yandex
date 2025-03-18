#include "config.h"

#include <util/generic/overloaded.h>
#include <library/cpp/resource/resource.h>

#include <util/generic/scope.h>
#include <util/generic/xrange.h>
#include <util/string/split.h>

#include <google/protobuf/message.h>

using namespace google::protobuf;

namespace NProtoConfig {
    void ParseConfigFromJson(IInputStream& input, NProtoBuf::Message& message, const NProtobufJson::TJson2ProtoConfig proto2JsonConfig) {
        NProtobufJson::Json2Proto(input, message, proto2JsonConfig);
    }

    void ParseConfigFromJson(TStringBuf conf, NProtoBuf::Message& message, const NProtobufJson::TJson2ProtoConfig proto2JsonConfig) {
        NProtobufJson::Json2Proto(conf, message, proto2JsonConfig);
    }

    void LoadConfigFromResource(const TString& path, NProtoBuf::Message& message) {
        const TString rawConfig = NResource::Find(path);
        ParseConfigFromJson(rawConfig, message);
    }

    void LoadConfigFromJsonFile(const TString& path, NProtoBuf::Message& message) {
        TFileInput inputStream(path);
        const TString rawConfig = inputStream.ReadAll();
        ParseConfigFromJson(rawConfig, message);
    }

    void OverrideConfig(google::protobuf::Message& config, const TString& with) {
        google::protobuf::Message* msgPtr = &config;

        size_t pos = with.find('=');
        Y_ENSURE(pos < with.length(), "Incorrect override option '" << with << "': '=' char not found");
        TString key = with.substr(0, pos);
        TString val = with.substr(pos + 1);
        Y_ENSURE(key.length(), "Empty key for override '" << with << "'");

        auto descriptorPtr = msgPtr->GetDescriptor();
        auto reflectionPtr = msgPtr->GetReflection();

        // get to leaf proto message
        TVector<TStringBuf> parts = StringSplitter(key).Split('.').ToList<TStringBuf>();
        TStringBuf lastPart(parts.back());
        parts.pop_back();
        for (auto part : parts) {
            auto fDescPtr = descriptorPtr->FindFieldByName(TString(part));
            Y_ENSURE(fDescPtr, "Member '" << part << "' not found for override '" << with << "'");
            msgPtr = reflectionPtr->MutableMessage(msgPtr, fDescPtr);
            descriptorPtr = msgPtr->GetDescriptor();
            reflectionPtr = msgPtr->GetReflection();
        }

        // update leaf proto message
        auto fDescPtr = descriptorPtr->FindFieldByName(TString(lastPart));
        Y_ENSURE(fDescPtr, "member '" << lastPart << "' not found for override '" << with << "'");
        reflectionPtr = msgPtr->GetReflection();
        switch (fDescPtr->type()) {
            case google::protobuf::FieldDescriptor::Type::TYPE_DOUBLE:
                reflectionPtr->SetDouble(msgPtr, fDescPtr, FromString(val));
                break;
            case google::protobuf::FieldDescriptor::Type::TYPE_FLOAT:
                reflectionPtr->SetFloat(msgPtr, fDescPtr, FromString(val));
                break;
            case google::protobuf::FieldDescriptor::Type::TYPE_BOOL:
                reflectionPtr->SetBool(msgPtr, fDescPtr, FromString(val));
                break;
            case google::protobuf::FieldDescriptor::Type::TYPE_INT32:
                reflectionPtr->SetInt32(msgPtr, fDescPtr, FromString(val));
                break;
            case google::protobuf::FieldDescriptor::Type::TYPE_UINT32:
                reflectionPtr->SetUInt32(msgPtr, fDescPtr, FromString(val));
                break;
            case google::protobuf::FieldDescriptor::Type::TYPE_INT64:
                reflectionPtr->SetInt64(msgPtr, fDescPtr, FromString(val));
                break;
            case google::protobuf::FieldDescriptor::Type::TYPE_UINT64:
                reflectionPtr->SetUInt64(msgPtr, fDescPtr, FromString(val));
                break;
            case google::protobuf::FieldDescriptor::Type::TYPE_STRING:
                reflectionPtr->SetString(msgPtr, fDescPtr, val);
                break;
            case google::protobuf::FieldDescriptor::Type::TYPE_ENUM:
                {
                    const NProtoBuf::EnumDescriptor* enumDescPtr = fDescPtr->enum_type();
                    const NProtoBuf::EnumValueDescriptor* enumValueDescPtr = enumDescPtr->FindValueByName(val);
                    Y_ENSURE(enumValueDescPtr, "Enum not found for value '" << val << "'");
                    reflectionPtr->SetEnum(msgPtr, fDescPtr, enumValueDescPtr);
                }
                break;
            default:
                ythrow yexception() << "Override '" << with << "' for type " << fDescPtr->type_name() << " not implemented";
        }
    }

    namespace {
        TString Print(const TKeyStack& keyStack) {
            TString res;
            for (auto&& key : keyStack) {
                res.append(std::visit(TOverloaded{
                    [](const TField& f) {
                        return TString::Join(".", f.Name);
                    },
                    [](const TMapIdx& k) {
                        return TString::Join("[", k.Idx.Quote(), "]");
                    },
                    [](const TIdx& k) {
                        return TString::Join("[", ToString(k.Idx), "]");
                    },
                }, key));
            }
            return res;
        }

        struct TParseContext {
            TUnknownFieldCbImpl& UnknownFieldCb_;
            TKeyStack& ParseStack_;
        };

        struct TFunc: public TParseContext, public NConfig::IConfig::IFunc {
        public:
            explicit TFunc(google::protobuf::Message& message, TParseContext pc)
                : TParseContext(pc)
                , Message_(message)
            {
            }

            void DoConsume(const TString& key, NConfig::IConfig::IValue* value) override;
        private:
            google::protobuf::Message& Message_;
        };


        struct TFuncRepeated: public TParseContext, public NConfig::IConfig::IFunc {
        public:
            TFuncRepeated(Message& message, const FieldDescriptor& fd, TParseContext pc)
                : TParseContext(pc)
                , Message_(message)
                , Fd_(fd)
            {}

            void DoConsume(const TString& key, NConfig::IConfig::IValue* value) override;
        private:
            Message& Message_;
            const FieldDescriptor& Fd_;
        };


        void CheckRequiredFields(const google::protobuf::Message& m, const TKeyStack& parseStack) {
            for (auto i : xrange(m.GetDescriptor()->field_count())) {
                auto* f = m.GetDescriptor()->field(i);
                if (f->is_required()) {
                    Y_ENSURE(m.GetReflection()->HasField(m, f),
                        "Required field " << f->name() << " not set at " << parseStack);
                }
            }
        }


        void SetField(
            TParseContext pc,
            google::protobuf::Message& m,
            const google::protobuf::FieldDescriptor& fd,
            NConfig::IConfig::IValue& value
        ) {
            const auto* reflection = m.GetReflection();
            switch (fd.cpp_type()) {
            case FieldDescriptor::CPPTYPE_FLOAT:
                reflection->SetFloat(&m, &fd, FromString<float>(value.AsString()));
                break;
            case FieldDescriptor::CPPTYPE_DOUBLE:
                reflection->SetFloat(&m, &fd, FromString<double>(value.AsString()));
                break;
            case FieldDescriptor::CPPTYPE_INT32:
                reflection->SetInt32(&m, &fd, FromString<int32>(value.AsString()));
                break;
            case FieldDescriptor::CPPTYPE_INT64:
                reflection->SetInt64(&m, &fd, FromString<int64>(value.AsString()));
                break;
            case FieldDescriptor::CPPTYPE_UINT32:
                reflection->SetUInt32(&m, &fd, FromString<uint32>(value.AsString()));
                break;
            case FieldDescriptor::CPPTYPE_UINT64:
                reflection->SetUInt64(&m, &fd, FromString<uint64>(value.AsString()));
                break;
            case FieldDescriptor::CPPTYPE_BOOL:
                reflection->SetBool(&m, &fd, value.AsBool());
                break;
            case FieldDescriptor::CPPTYPE_STRING:
                reflection->SetString(&m, &fd, value.AsString());
                break;
            case FieldDescriptor::CPPTYPE_ENUM:
                reflection->SetEnum(&m, &fd, fd.enum_type()->FindValueByName(value.AsString()));
                break;
            case FieldDescriptor::CPPTYPE_MESSAGE: {
                auto* inner = reflection->MutableMessage(&m, &fd);
                TFunc f(*inner, pc);
                value.AsSubConfig()->ForEach(&f);
                CheckRequiredFields(*inner, pc.ParseStack_);
                break;
            }
            default:
                ythrow yexception() << "Unsupported value type at " << pc.ParseStack_;
            }
        }

        void AddField(
            TParseContext pc,
            google::protobuf::Message& m,
            const google::protobuf::FieldDescriptor& fd,
            NConfig::IConfig::IValue& value
        ) {
            const auto* reflection = m.GetReflection();
            switch (fd.cpp_type()) {
            case FieldDescriptor::CPPTYPE_FLOAT:
                reflection->AddFloat(&m, &fd, FromString<float>(value.AsString()));
                break;
            case FieldDescriptor::CPPTYPE_DOUBLE:
                reflection->AddFloat(&m, &fd, FromString<double>(value.AsString()));
                break;
            case FieldDescriptor::CPPTYPE_INT32:
                reflection->AddInt32(&m, &fd, FromString<int32>(value.AsString()));
                break;
            case FieldDescriptor::CPPTYPE_INT64:
                reflection->AddInt64(&m, &fd, FromString<int64>(value.AsString()));
                break;
            case FieldDescriptor::CPPTYPE_UINT32:
                reflection->AddUInt32(&m, &fd, FromString<uint32>(value.AsString()));
                break;
            case FieldDescriptor::CPPTYPE_UINT64:
                reflection->AddUInt64(&m, &fd, FromString<uint64>(value.AsString()));
                break;
            case FieldDescriptor::CPPTYPE_BOOL:
                reflection->AddBool(&m, &fd, value.AsBool());
                break;
            case FieldDescriptor::CPPTYPE_STRING:
                reflection->AddString(&m, &fd, value.AsString());
                break;
            case FieldDescriptor::CPPTYPE_ENUM:
                reflection->AddEnum(&m, &fd, fd.enum_type()->FindValueByName(value.AsString()));
                break;
            case FieldDescriptor::CPPTYPE_MESSAGE: {
                auto* inner = reflection->AddMessage(&m, &fd);
                TFunc f(*inner, pc);
                value.AsSubConfig()->ForEach(&f);
                CheckRequiredFields(*inner, pc.ParseStack_);
                break;
            }
            default:
                ythrow yexception() << "Unsupported value type at " << pc.ParseStack_;
            }
        }

        void TFunc::DoConsume(const TString& key, NConfig::IConfig::IValue* value) {
            ParseStack_.emplace_back(TField(key));
            Y_DEFER {
                ParseStack_.pop_back();
            };

            const google::protobuf::Descriptor* md = Message_.GetDescriptor();
            if (const google::protobuf::FieldDescriptor* fd = md->FindFieldByName(key)) {
                if (fd->is_repeated()) {
                    Y_ENSURE(value->IsContainer(), "Container expected at " << ParseStack_);

                    TFuncRepeated f(Message_, *fd, *this);
                    value->AsSubConfig()->ForEach(&f);
                } else {
                    SetField(*this, Message_, *fd, *value);
                }
            } else {
                std::visit(TOverloaded{
                    [](std::monostate) {},
                    [&](const TUnknownFieldCb& cb) {
                        cb(key, value);
                    },
                    [&](const TStackUnknownFieldCb& cb) {
                        Y_VERIFY(!ParseStack_.empty());
                        Y_VERIFY(std::holds_alternative<TField>(ParseStack_.back()));
                        auto st = ParseStack_;
                        auto last = std::get<TField>(st.back());
                        st.pop_back();
                        cb(st, last.Name, value);
                    }
                }, UnknownFieldCb_);
            }
        }

        void TFuncRepeated::DoConsume(const TString& key, NConfig::IConfig::IValue* value) {
            if (Fd_.is_map()) {
                ParseStack_.emplace_back(TMapIdx(key));
            } else {
                ParseStack_.emplace_back(TIdx(FromString<size_t>(key)));
            }

            Y_DEFER {
                ParseStack_.pop_back();
            };

            if (Fd_.is_map()) {
                // A workaround for map reflection suggested in
                // https://groups.google.com/d/msg/protobuf/nNZ_ItflbLE/x7hLZ1GtAAAJ
                auto* mapEntry = Message_.GetReflection()->AddMessage(&Message_, &Fd_);
                auto* keyFd = mapEntry->GetDescriptor()->FindFieldByNumber(1);
                auto* valFd = mapEntry->GetDescriptor()->FindFieldByNumber(2);

                Y_ENSURE(keyFd->cpp_type() == FieldDescriptor::CPPTYPE_STRING,
                         "Unsupported map key type " << keyFd->type_name()
                                                     << " at " << ParseStack_);

                auto* innerR = mapEntry->GetReflection();
                innerR->SetString(mapEntry, keyFd, key);
                SetField(*this, *mapEntry, *valFd, *value);
            } else {
                AddField(*this, Message_, Fd_, *value);
            }
        }
    }

    void ParseConfigImpl(NConfig::IConfig& config, google::protobuf::Message& message, TUnknownFieldCbImpl uf) {
        TKeyStack parseStack;
        TFunc func(message, {uf, parseStack});
        config.ForEach(&func);
        CheckRequiredFields(message, parseStack);
    }
}

template <>
void Out<NProtoConfig::TKeyStack>(IOutputStream& out, const NProtoConfig::TKeyStack& st) {
    out << NProtoConfig::Print(st);
}
