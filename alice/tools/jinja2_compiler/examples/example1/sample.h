//
// Autogenerated file
// Don't edit it manually!
// Owner: d-dima, Generation time: 27-04-2022 21-32-55
//

// Forward class declarations
class TSampleProto;

class TSampleProto {
public:
    //
    // Enum declarations
    //
    enum class ESample {
        Esample1 = 0,
        Esample2 = 1,
        Esample3 = 2
    };
    //
    // Internal classes declarations
    //
    class TNestedClass {
        // TODO: this example doesn't support nested class declaration
    };
    //
    // Internal union declarations
    //
    typedef std::variant<
        TString,
        i32
    > OneofValueContainerOneof;
private:
    //
    // Field declarations
    //
    // Comments can be added into generated files from proto file
    bool BoolValue;
    i32 Int32Value;
    TString StringValue;
    float FloatValue;
    double DoubleValue;
    ESample EnumValue;
    TMaybe<i32> IntOptional;
    TVector<TString> StringRepeated;
    TNestedClass SubclassValue;
    OneofValueContainerOneof OneofValueContainer;
public:
    //
    // Field accessors
    //
    void SetBoolValue(const bool& boolValue) {
        BoolValue = boolValue;
    }
    bool& GetBoolValue() {
        return BoolValue;
    }
    const bool& GetBoolValue() const {
        return BoolValue;
    }
    void SetInt32Value(const i32& int32Value) {
        Int32Value = int32Value;
    }
    i32& GetInt32Value() {
        return Int32Value;
    }
    const i32& GetInt32Value() const {
        return Int32Value;
    }
    void SetStringValue(const TString& stringValue) {
        StringValue = stringValue;
    }
    TString& GetStringValue() {
        return StringValue;
    }
    const TString& GetStringValue() const {
        return StringValue;
    }
    void SetFloatValue(const float& floatValue) {
        FloatValue = floatValue;
    }
    float& GetFloatValue() {
        return FloatValue;
    }
    const float& GetFloatValue() const {
        return FloatValue;
    }
    void SetDoubleValue(const double& doubleValue) {
        DoubleValue = doubleValue;
    }
    double& GetDoubleValue() {
        return DoubleValue;
    }
    const double& GetDoubleValue() const {
        return DoubleValue;
    }
    void SetEnumValue(const ESample& enumValue) {
        EnumValue = enumValue;
    }
    ESample& GetEnumValue() {
        return EnumValue;
    }
    const ESample& GetEnumValue() const {
        return EnumValue;
    }
    void SetIntOptional(const TMaybe<i32>& intOptional) {
        IntOptional = intOptional;
    }
    TMaybe<i32>& GetIntOptional() {
        return IntOptional;
    }
    const TMaybe<i32>& GetIntOptional() const {
        return IntOptional;
    }
    void SetStringRepeated(const TVector<TString>& stringRepeated) {
        StringRepeated = stringRepeated;
    }
    TVector<TString>& GetStringRepeated() {
        return StringRepeated;
    }
    const TVector<TString>& GetStringRepeated() const {
        return StringRepeated;
    }
    void AddStringRepeated(const TString& stringRepeated) {
        StringRepeated.push_back(stringRepeated);
    }
    void SetSubclassValue(const TNestedClass& subclassValue) {
        SubclassValue = subclassValue;
    }
    TNestedClass& GetSubclassValue() {
        return SubclassValue;
    }
    const TNestedClass& GetSubclassValue() const {
        return SubclassValue;
    }
    const OneofValueContainerOneof GetOneofValueContainer() const {
        return OneofValueContainer;
    }
    void SetOneofValueContainer(const TString& value) {
        OneofValueContainer = value;
    }
    void SetOneofValueContainer(const i32& value) {
        OneofValueContainer = value;
    }
    > OneofValueContainerOneof;
};
