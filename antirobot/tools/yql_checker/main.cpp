#include <antirobot/daemon_lib/yql_rule_set.h>

#include <util/stream/output.h>


int main(int argc, char* argv[]) {
    if (argc != 2) {
        Cerr << "Usage: yql_checker <expression>\n";
        return 1;
    }

    try {
        NAntiRobot::TYqlRuleSet ruleSet({argv[1]});
    } catch (const yexception&) {
        Cerr << "Bad YQL: " << CurrentExceptionMessage() << '\n';
        return 1;
    }

    Cerr << "Correct YQL\n";
}
