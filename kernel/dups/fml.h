#pragma once

#include <util/generic/yexception.h>

#include <contrib/libs/muparser/muParser.h>

namespace NDups {
    template <typename TVariables>
    class TFormula {
    public:
        TFormula(const TString& fml)
        {
            try {
                Formula.SetVarFactory(AddVariable, &Variables);
                Formula.DefineFun("IF", IfFunc, false);
                Formula.DefineFun("MIN", MinFunc, false);
                Formula.DefineFun("MAX", MaxFunc, false);
            } catch (mu::Parser::exception_type &e) {
                ythrow yexception() << "Initialization failure: " << e.GetMsg();
            }

            try {
                Formula.SetExpr(fml);
            } catch (mu::Parser::exception_type &e) {
                ythrow yexception() << "Bad formula '" << fml << "': " << e.GetMsg();
            }

            try {
                // Add variable names to fetch variable values later
                const mu::varmap_type& vars = Formula.GetUsedVar();
                for (mu::varmap_type::const_iterator item = vars.begin(); item != vars.end(); ++item)
                    Variables.Add(item->first);
            } catch (mu::Parser::exception_type &e) {
                ythrow yexception() << "Can't get variables names: " << e.GetMsg();
            }
        }

        inline double Eval() const {
            return Formula.Eval();
        }

        TVariables& GetVariables() {
            return Variables;
        }

    private:
        static inline double IfFunc(double a, double b, double c) {
            return a > 0 ? b : c;
        }

        static inline double MinFunc(double a, double b) {
            return Min(a, b);
        }

        static inline double MaxFunc(double a, double b) {
            return Max(a, b);
        }

        static inline double* AddVariable(const char* name, void* data) {
            return reinterpret_cast<TVariables*>(data)->Get(name);
        }

    private:
        TVariables Variables;
        mu::Parser Formula;
    };
}  // namespace NDups
