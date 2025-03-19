#pragma once
struct TZonedString;
namespace NProto {
    class TZonedString;
};

void Pack(const TZonedString& zs, NProto::TZonedString& proto);
void Unpack(const NProto::TZonedString& proto, TZonedString& zs);
