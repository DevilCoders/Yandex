#pragma once

#include <kernel/querydata/client/querydata.h>
#include <kernel/querydata/idl/querydata_structs.pb.h>

#include <library/cpp/binsaver/bin_saver.h>

#include <util/generic/strbuf.h>
#include <util/generic/yexception.h>

#include <util/string/escape.h>
#include <library/cpp/string_utils/base64/base64.h>

namespace NQueryData {

    // TODO: kill it with fire

    struct TFactorParser {
        enum EUnescapeType {
            UT_NONE = 0,
            UT_UTF8 = 1,
            UT_BASE64 = 2,
        };

        struct TParseCtx {
            TStringBuf Token;
            TStringBuf Value;
            TStringBuf NameType;
            TStringBuf Name;
            EFactorType Type = FT_NONE;
            EUnescapeType Unescape = UT_NONE;
        };

        void Init(TStringBuf data) {
            while (!!data && data.back() == '\n')
                data.Chop(1);

            Data = data;
            Iter = data;
        }

        void InitToken(TStringBuf nametype, TStringBuf value) {
            Ctx = TParseCtx();
            Ctx.NameType = nametype;
            Ctx.Value = value;
        }

        bool ValidFactor() const {
            return !!Ctx.Name && FT_NONE != Ctx.Type;
        }

        bool IsBinary() const {
            return UT_BASE64 == Ctx.Unescape;
        }

        TStringBuf GetName() const {
            return Ctx.Name;
        }

        TStringBuf GetNameWithType() const {
            return Ctx.NameType;
        }

        EFactorType GetType() const {
            return Ctx.Type;
        }

        TStringBuf GetValue() const {
            return Ctx.Value;
        }

        bool Next() {
            if (!NextToken()) {
                return false;
            }

            SplitToken();
            ProcessNameType();
            ProcessValue();
            return true;
        }

        bool NextToken() {
            if (!Iter) {
                return false;
            }

            while (!!Iter && isspace(Iter[0])) {
                Iter.Skip(1);
            }

            TStringBuf token;
            do {
                token = Iter.NextTok('\t');
            } while (!!Iter && !token);

            if (!token) {
                return false;
            }

            Ctx = TParseCtx();
            Ctx.Token = token;
            return true;
        }

        void SplitToken() {
            Ctx.Token.Split('=', Ctx.NameType, Ctx.Value);
        }

        void Process() {
            ProcessNameType();
            ProcessValue();
        }

        void ProcessNameType() {
            if (!Ctx.NameType) {
                ythrow yexception() << "empty factor name in data line: " << Data;
            }

            TStringBuf type;
            Ctx.NameType.Split(':', Ctx.Name, type);

            NameBuf.clear();
            UnescapeC(Ctx.Name.data(), Ctx.Name.size(), NameBuf);
            Ctx.Name = NameBuf;

            if (!Ctx.Name) {
                ythrow yexception() << "empty factor name in data line: " << Data;
            }

            if (!type || FACTOR_VALUE_TYPE_STRING == type
                    || FACTOR_VALUE_TYPE_STRING_ESCAPED == type
                    || FACTOR_VALUE_TYPE_BINARY == type) {
                Ctx.Type = FT_STRING;
                if (FACTOR_VALUE_TYPE_BINARY == type) {
                    Ctx.Unescape = UT_BASE64;
                } else if (FACTOR_VALUE_TYPE_STRING_ESCAPED == type) {
                    Ctx.Unescape = UT_UTF8;
                }
            } else if (FACTOR_VALUE_TYPE_INT == type) {
                Ctx.Type = FT_INT;
            } else if (FACTOR_VALUE_TYPE_FLOAT == type) {
                Ctx.Type = FT_FLOAT;
            } else {
                ythrow yexception() << "invalid type suffix ':" << type << "'";
            }
        }

        void ProcessValue() {
            TStringBuf& val = Ctx.Value;
            switch (Ctx.Type) {
            case FT_INT:
            case FT_FLOAT:
                while (!!val && isspace(val[0])) {
                    val.Skip(1);
                }

                while (!!val && isspace(val[val.size() - 1])) {
                    val.Chop(1);
                }

                break;
            case FT_STRING:
                if (UT_UTF8 == Ctx.Unescape) {
                    ValueBuf.clear();
                    UnescapeC(val.data(), val.size(), ValueBuf);
                    val = ValueBuf;
                } else if (UT_BASE64 == Ctx.Unescape) {
                    ValueBuf.clear();
                    Base64Decode(val, ValueBuf);
                    val = ValueBuf;
                }
                break;
            default:
                break;
            }

            if (!Ctx.Value && (FT_INT == Ctx.Type || FT_FLOAT == Ctx.Type)) {
                ythrow yexception() << "empty number factor in data line: " << Data;
            }
        }

        void FillBinaryValue(TRawFactor* fac, TStringBuf value) const {
            fac->MutableBinaryValue()->assign(value);
        }

        void FillBinaryValue(TFactor* fac, TStringBuf value) const {
            fac->MutableBinaryValue()->assign(Base64Encode(value));
        }

        template <typename TFac>
        bool FillCurrentFactor(TFac* fac) const {
            if (!ValidFactor()) {
                return false;
            }

            switch (Ctx.Type) {
            default:
                ythrow yexception() << "invalid factor type " << (int)Ctx.Type;
            case FT_STRING:
                if (IsBinary()) {
                    FillBinaryValue(fac, Ctx.Value);
                } else {
                    fac->MutableStringValue()->assign(Ctx.Value);
                }
                break;
            case FT_INT:
                fac->SetIntValue(FromString<i64>(Ctx.Value));
                break;
            case FT_FLOAT:
                fac->SetFloatValue(FromString<float>(Ctx.Value));
                break;
            }
            return true;
        }

        void Clear() {
            Data.Clear();
            Iter.Clear();
            Ctx = TParseCtx();
            NameBuf.clear();
            ValueBuf.clear();
        }

    private:
        TStringBuf Data;
        TStringBuf Iter;

        TParseCtx Ctx;

        TString NameBuf;
        TString ValueBuf;
    };


    struct TFactorIdMapping {
        typedef THashMap<TString, ui32> TFactor2Id;

        TFactor2Id Factor2Id;
        TVector<TString> Id2Factor;

        int operator& (IBinSaver& s) {
            int i = 0;
            s.Add(++i, &Factor2Id);
            s.Add(++i, &Id2Factor);
            return 0;
        }

        ui32 MapName(TStringBuf f) {
            ui32 fid = -1;

            TFactor2Id::const_iterator fit = Factor2Id.find(f);
            if (fit == Factor2Id.end()) {
                TString s = TString{f};
                fid = Factor2Id[s] = Id2Factor.size();
                Id2Factor.push_back(s);
            } else {
                fid = fit->second;
            }

            return fid;
        }

        ui32 MapName(TStringBuf f) const {
            return Factor2Id.at(f);
        }

        TStringBuf MapId(ui32 id) const {
            return Id2Factor.at(id);
        }

        void CopyTo(TFileDescription& fd) {
            for (ui32 i = 0; i < Id2Factor.size(); ++i) {
                TFactorMeta* fm = fd.AddFactorsMeta();
                fm->SetName(Id2Factor[i]);
            }
        }
    };


    struct TValueBuilder {
        TFactorIdMapping Mapping;

        mutable TRawQueryData QueryDataBuff;
        mutable TString Buffer;

        int operator& (IBinSaver& s) {
            int i = 0;
            s.Add(++i, &Mapping);
            return i;
        }

        std::pair<TStringBuf, size_t> Build(TStringBuf f, ui64 tstamp) {
            return DoBuild(Mapping, f, tstamp);
        }

        std::pair<TStringBuf, size_t> BuildJson(TStringBuf f, ui64 tstamp) {
            QueryDataBuff.Clear();

            if (tstamp) {
                QueryDataBuff.SetVersion(tstamp);
            }

            QueryDataBuff.SetJson(f.data(), f.size());

            return Serialize(0);
        }

        std::pair<TStringBuf, size_t> BuildKeyRef(TStringBuf f) {
            QueryDataBuff.Clear();
            QueryDataBuff.SetKeyRef(f.data(), f.size());

            return Serialize(0);
        }

    private:
        std::pair<TStringBuf, size_t> Serialize(size_t sz0) {
            Buffer.clear();

            if (!QueryDataBuff.SerializeToString(&Buffer))
                ythrow yexception() << "could not serialize";

            return std::make_pair<TStringBuf, size_t>(TStringBuf(Buffer), Buffer.size() + sz0);
        }

        template <typename TMapping>
        std::pair<TStringBuf, size_t> DoBuild(TMapping& m, TStringBuf f, ui64 tstamp) {
            TFactorParser p;
            QueryDataBuff.Clear();

            if (tstamp) {
                QueryDataBuff.SetVersion(tstamp);
            }

            size_t sz = 0;
            p.Init(f);
            while (p.Next()) {
                sz += p.GetName().size();

                TRawFactor* fac = QueryDataBuff.AddFactors();
                fac->SetId(m.MapName(p.GetName()));
                p.FillCurrentFactor(fac);
            }

            return Serialize(sz);
        }
    };


    struct TCommonFactorsBuilder {
        bool Empty = true;
        TVector<TFactor> Factors;
        TMaybe<TString> Json;

        void BuildFactors(TStringBuf f) {
            Empty = false;
            TFactorParser p;
            p.Init(f);
            while (p.Next()) {
                Factors.emplace_back();
                TFactor* fac = &Factors.back();
                fac->MutableName()->assign(p.GetName());
                p.FillCurrentFactor(fac);
            }
        }

        void SetJson(TStringBuf json) {
            Empty = false;
            Json = TString{json};
        }

        size_t Size() const {
            size_t fsz = 0;
            for (ui32 i = 0, sz = Factors.size(); i < sz; ++i) {
                fsz += Factors[i].SerializeAsString().size();
            }
            return fsz + (Json ? Json->size() : 0);
        }

        void CopyTo(TFileDescription& fd) {
            if (!Empty) {
                for (ui32 i = 0; i < Factors.size(); ++i) {
                    TFactor* fac = fd.AddCommonFactors();
                    fac->CopyFrom(Factors[i]);
                }

                if (Json) {
                    fd.SetCommonJson(*Json);
                }

                fd.SetAllowedCommon(true);
            }
        }
    };

    bool DoCheckSize(TString& s, size_t data, size_t limit, TStringBuf name);

}
