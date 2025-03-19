#include <Python.h>

#include <kernel/ethos/lib/reg_tree/compositions.h>

#include <library/cpp/pybind/typedesc.h>

#include <util/generic/yexception.h>

class TRegTreeException : public yexception {
};

class TRegTreeParamsTraits :
    public NPyBind::TPythonType<NRegTree::TPredictor, NRegTree::TPredictor, TRegTreeParamsTraits>
{
private:
    using TBase = NPyBind::TPythonType<NRegTree::TPredictor, NRegTree::TPredictor, TRegTreeParamsTraits>;
    friend TBase;

    TRegTreeParamsTraits()
        : TBase("RegTreeModel", "RegTree model")
    {
        AddCaller("Prediction", NPyBind::CreateConstMethodCaller<NRegTree::TPredictor>(&NRegTree::TPredictor::Prediction));
    }
public:
    static NRegTree::TPredictor* GetObject(NRegTree::TPredictor& predictor) {
        return &predictor;
    }

    static NRegTree::TPredictor* DoInitObject(PyObject* args, PyObject*) {
        TString modelName;
        if (!NPyBind::ExtractArgs(args, modelName)) {
            ythrow TRegTreeException() << "RegTreeModel must be initialized by model name";
        }
        return new NRegTree::TPredictor(modelName);
    }
};

PyMODINIT_FUNC initlibregtree() {
    NPyBind::TExceptionsHolder::Instance().AddException<TRegTreeException>("RegTreeException", "yexception");

    NPyBind::TPyObjectPtr m = NPyBind::TModuleHolder::Instance().InitModule("libregtree");
    TRegTreeParamsTraits::Instance().Register(m, "RegTreeModel");
}
