#include "polygon_parser.h"

#include <util/string/cast.h>

using namespace NReverseGeocoder;
using namespace NYandexMap;

void NReverseGeocoder::NYandexMap::TPolygonParser::Parse(const TStringBuf& str) {
    Begin_ = str.data();
    End_ = str.data() + str.size();
    State_ = EState::FirstClosed;

    const char* ptr = Begin_;
    ParseImpl(ptr);
}

void NReverseGeocoder::NYandexMap::TPolygonParser::Parse(const char*& ptr, const char* begin, const char* end) {
    Begin_ = begin;
    End_ = end;
    State_ = EState::FirstClosed;
    ParseImpl(ptr);
}

static double StringToDouble(const char* ptr, const char** se0) {
    char** se = const_cast<char**>(se0);
    double ret = StrToD(ptr, se);
    se0 = const_cast<const char**>(se);
    return ret;
}

void NReverseGeocoder::NYandexMap::TPolygonParser::ParseImpl(const char*& ptr) {
    NProto::TPolygon* polygon = nullptr;
    while (ptr != End_ && State_ != EState::Parsed) {
        if (isspace(*ptr)) {
            ++ptr;
            continue;
        }
        switch (State_) {
            case EState::Parsed: {
                break;
            }
            case EState::FirstClosed: {
                if (*ptr != '(')
                    ythrow yexception() << "Expected '(', '" << *ptr << "' given";
                State_ = EState::SecondClosed;
                ++ptr;
                break;
            }
            case EState::SecondClosed: {
                if (*ptr == '(') {
                    State_ = EState::SecondOpened;
                    polygon = Region_->AddPolygons();

                    PolyNum++;
                    const auto kind = (1 == PolyNum) ? NProto::TPolygon::TYPE_OUTER : NProto::TPolygon::TYPE_INNER;

                    polygon->SetPolygonId(PolyNum);
                    polygon->SetType(kind);
                } else {
                    ythrow yexception() << "Expected '(', '" << *ptr << "' given";
                }
                ++ptr;
                break;
            }
            case EState::PolygonDelimeter: {
                if (*ptr == ',') {
                    State_ = EState::SecondClosed;
                } else if (*ptr == ')') {
                    State_ = EState::Parsed;
                } else {
                    ythrow yexception() << "Expected ',' or ')', '" << *ptr << "' given";
                }
                ++ptr;
                break;
            }
            case EState::SecondOpened: {
                if (!isdigit(*ptr) && *ptr != '-')
                    ythrow yexception() << "Expected digit, '" << *ptr << "' given";
                NProto::TLocation* location = polygon->AddLocations();
                location->SetLon(StringToDouble(ptr, &ptr));
                while (ptr != End_ && isspace(*ptr))
                    ++ptr;
                if (ptr == End_)
                    yexception() << "Expected latitude, found end of line";
                location->SetLat(StringToDouble(ptr, &ptr));
                State_ = EState::Delimeter;
                break;
            }
            case EState::Delimeter: {
                if (*ptr == ',')
                    State_ = EState::SecondOpened;
                else if (*ptr == ')')
                    State_ = EState::PolygonDelimeter;
                else
                    ythrow yexception() << "Expected ',' or ')', '" << *ptr << "' given";
                ++ptr;
                break;
            }
        }
    }

    if (State_ == EState::SecondClosed)
        ythrow yexception() << "Expected ')', found end of line";
    if (State_ == EState::SecondOpened)
        ythrow yexception() << "Expected '))', found end of line";
    if (State_ == EState::Delimeter)
        ythrow yexception() << "Expected ',' or '))', found end of line";
    if (State_ != EState::Parsed)
        ythrow yexception() << "Stopped in state: " << State_ << "(expected: " << EState::Parsed << ")";
}

void NReverseGeocoder::NYandexMap::TMultiPolygonParser::Parse(const TStringBuf& str) {
    Begin_ = str.data();
    End_ = str.data() + str.size();
    State_ = EState::Closed;

    const char* ptr = Begin_;
    ParseImpl(ptr);
}

void NReverseGeocoder::NYandexMap::TMultiPolygonParser::ParseImpl(const char*& ptr) {
    while (ptr != End_ && State_ != EState::Parsed) {
        if (isspace(*ptr)) {
            ++ptr;
            continue;
        }
        switch (State_) {
            case EState::Parsed: {
                break;
            }
            case EState::Closed: {
                if (*ptr != '(')
                    ythrow yexception() << "Expected '(', '" << *ptr << "' given";
                State_ = EState::Opened;
                ++ptr;
                break;
            }
            case EState::Opened: {
                TPolygonParser polygonParser(*Region_);
                polygonParser.Parse(ptr, Begin_, End_);
                State_ = EState::Delimeter;
                break;
            }
            case EState::Delimeter: {
                if (*ptr == ',') {
                    State_ = EState::Opened;
                } else if (*ptr == ')') {
                    State_ = EState::Parsed;
                } else {
                    ythrow yexception() << "Expected '(', '" << *ptr << "' given";
                }
                ++ptr;
                break;
            }
        }
    }

    if (State_ != EState::Parsed)
        ythrow yexception() << "Expected '(' in end of line";
}
