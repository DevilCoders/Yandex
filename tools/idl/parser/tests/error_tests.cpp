#include "test_helpers.h"

#include <yandex/maps/idl/idl.h>
#include <yandex/maps/idl/utils/errors.h>

#include <string>

namespace yandex {
namespace maps {
namespace idl {
namespace parser {
namespace error_tests {

BOOST_AUTO_TEST_CASE(flex_bison_errors)
{
    auto env = simpleEnv("parser/tests/idl_frameworks", "parser/tests/idl_root", false);
    try {
        env.idl("with_errors/with_flex_bison_errors.idl");
        BOOST_FAIL("Failed to catch Flex / Bison errors");
    } catch (const utils::GroupedError& e) {
        checkStringsEqualByLine(e.what(),
            "Idl file " + utils::asConsoleBold("'parser/tests/idl_root/with_errors/with_flex_bison_errors.idl'") + "\n"
            "  contains following syntax errors:\n"
            "    line 9, enum: unexpected STRUCT\n"
            "    line 10: unexpected '}'\n"
            "    line 12: unexpected IDENTIFIER\n"
            "    line 51, interface: Please do not use 'listener' as argument name - it is a keyword\n"
            "    line 54: unexpected IDENTIFIER\n");
    }
}

BOOST_AUTO_TEST_CASE(validator_errors)
{
    auto env = simpleEnv("parser/tests/idl_frameworks", "parser/tests/idl_root", false);
    try {
        env.idl("with_errors/with_validator_errors.idl");
        BOOST_FAIL("Failed to catch validator errors");
    } catch (const utils::GroupedError& e) {
        checkStringsEqualByLine(e.what(),
            "Idl file " + utils::asConsoleBold("'with_errors/with_validator_errors.idl'") + "\n"
            "  contains following errors:\n"
            "    Duplicates: Duplicate item found 'F'\n"
            "    Duplicates.F: Enum cannot have @internal tag\n"
            "    Inconsistent: Non-bitfield enum constant 'B' has value\n"
            "    TooShort: Enum has only one constant\n"
            "    WithoutValues: Bitfield enum constant 'Field1' has no value\n"
            "    WithoutValues: Bitfield enum constant 'Field3' has no value\n"
            "    WithDuplicates: Duplicate field type found 'int'\n"
            "    EmptyStruct: Struct has no fields\n"
            "    TooSmall: Variant has only one item\n"
            "    S.o: Type not allowed 'any'. Any-Collection cannot be a struct / variant field\n"
            "    S.m: Only dictionaries with string keys are supported\n"
            "    S.InnerS.k: Duplicate field\n"
            "    S.InnerS.k: Duplicate field\n"
            "    S.InnerS.k: Type not allowed 'void'. Void can only be used as a type of function return value\n"
            "    S.InnerLiteS.k: Duplicate field\n"
            "    S.InnerLiteS.f: Lite struct cannot have bridged fields\n"
            "    S.InnerLiteS.k: Duplicate field\n"
            "    S.InnerLiteS.k: Type not allowed 'void'. Void can only be used as a type of function return value\n"
            "    L: Derives from itself\n"
            "    LambdaListener.onLambdaSomething: Duplicate lambda method\n"
            "    LambdaListener.onInterfaceReturn: Lambda-listener methods must return void\n"
            "    LambdaListener.onInterfaceReturn, return value: Type not allowed 'I'. Cannot return interface from listener method\n"
            "    LambdaListener.onListenerParameter, parameter 'invalidParameter': Type not allowed 'L'. Cannot pass weakly held listener to platform code - use strong_ref platform interface instead\n"
            "    LambdaListener.onConstParameter: Parameter \"s\" must be const because in C++ it is passed by reference\n"
            "    SomeStruct.internalField: Field with internal type has not @internal tag\n"
            "    I: Doc refers to a method of lambda listener 'LambdaListener#onLambdaSomething(...)'\n"
            "    I: Base type is of wrong kind\n"
            "    I.method: Non internal function has internal arguments or internal return type\n"
            "    I.notAllowed, parameter 'x': Constant qualifier is not allowed\n"
            "    I.notAllowed, parameter 't': Constant qualifier is not allowed\n"
            "    I.constNotAllowed, return value: Constant qualifier is not allowed\n"
            "    I.useConstParameter: Parameter \"s\" must be const because in C++ it is passed by reference\n"
            "    I.boolProperty: Constant qualifier is not allowed on non-interface types - use readonly on the right side\n"
            "    I.readonlyInterfaceProperty: Properties with \"strong\" interface types are not allowed - use weak_ref or shared_ref\n"
            "    I.generatedInterfaceProperty: Gen qualifier is not allowed on properties with interface types\n"
            "    I.generatedInterfaceProperty: Properties with \"strong\" interface types are not allowed - use weak_ref or shared_ref\n"
            "    I.generatedInterfaceProperty: Readonly qualifier required for properties with interface types\n"
            "    I.correctInterfaceProperty: Properties with \"strong\" interface types are not allowed - use weak_ref or shared_ref\n"
            "    I.correctInterfaceProperty: Readonly qualifier required for properties with interface types\n"
            "    I.methodWithInvalidObjcName: Invalid Objective-C-specific function name\n"
            "    I.methodWithIncorrectListener, return value: Type not allowed 'L'. Cannot pass weakly held listener to platform code - use strong_ref platform interface instead\n"
            "    I.interfacesProperty: Collecton not allowed \"strong\" interface types\n"
            "    I.interfacesMapProperty: Collecton not allowed \"strong\" interface types\n"
            "    I.interfaces, return value: Vector cannot contains interface type\n"
            "    I.interfacesMap, return value: Dictionary cannot contains interface type\n"
            "    LS.value: Field with @internal tag must: be optional or be of type vector/dict or have default value\n"
            "    ContainsListener: Doc refers to listener's nonexistent method 'L#onSomeUnusualResponse(...)'\n"
            "    ContainsListener.l: Type not allowed 'L'. Listener cannot be a struct / variant field\n"
            "    InternalStatics: Static interface has no functions and no properties\n"
            "    UndocumentedStatics.setFailedAssertionListener: Non internal function has internal arguments or internal return type\n"
            "    SomeStatics.constMethod: Const qualifier is not allowed on static interface function\n");
    }
}

BOOST_AUTO_TEST_CASE(not_found_error)
{
    auto env = simpleEnv("parser/tests/idl_frameworks", "parser/tests/idl_root", false);
    try {
        env.idl("with_errors/with_internal_not_found_error.idl");
        BOOST_FAIL("Failed to catch internal error");
    } catch (const utils::Exception& e) {
        checkStringsEqualByLine(e.what(),
            "\n  " + ::yandex::maps::idl::utils::asConsoleBold("tools-idl-app internal error:") + "\n"
            "    Could not find 'NotFound' from 'with_errors.I'");
    }
}

BOOST_AUTO_TEST_CASE(disable_internal_checks_error)
{
    auto env = simpleEnv("parser/tests/idl_frameworks", "parser/tests/idl_root", true);
    env.idl("with_errors/with_internal_errors.idl");
    BOOST_CHECK(true);
}

} // namespace error_tests
} // namespace parser
} // namespace idl
} // namespace maps
} // namespace yandex
