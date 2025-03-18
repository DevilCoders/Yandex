#include <iostream>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>
#include <boost/test/tree/visitor.hpp>

using namespace boost::unit_test;

class TestVisitor: public test_tree_visitor {
public:
    std::vector<std::string> CurrentSuite;

    using test_tree_visitor::visit;

    void visit(test_case const& test) override {
        for (const auto& part : CurrentSuite) {
            std::cout << part << "::";
        }
        std::cout << test.p_name.get() << std::endl;
    }

    bool test_suite_start(test_suite const& suite) override {
        CurrentSuite.push_back(suite.p_name.get());
        return true;
    }

    void test_suite_finish(const test_suite& /* suite */) override {
        CurrentSuite.pop_back();
    }
};

struct YaTestGlobalFixture {
    YaTestGlobalFixture() {
        size_t argc = framework::master_test_suite().argc;
        for (size_t i = 0; i < argc; i++) {
            std::string argument(framework::master_test_suite().argv[i]);
            if (argument == "list") {
                TestVisitor visitor;
                traverse_test_tree(framework::master_test_suite(), visitor, true);
                exit(EXIT_SUCCESS);
            }
        }
    }
};

BOOST_GLOBAL_FIXTURE(YaTestGlobalFixture);
