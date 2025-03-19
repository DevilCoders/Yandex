#include <Python.h>

#include <kernel/cluster/lib/agglomerative/agglomerative_clustering.h>

#include <library/cpp/pybind/typedesc.h>

#include <util/generic/yexception.h>

class TAgglomerativeParamTraits :
    public NPyBind::TPythonType<NAgglomerative::TClusterization, NAgglomerative::TClusterization, TAgglomerativeParamTraits>
{
private:
    using TBase = NPyBind::TPythonType<NAgglomerative::TClusterization, NAgglomerative::TClusterization, TAgglomerativeParamTraits>;
    friend TBase;

    TAgglomerativeParamTraits()
        : TBase("AgglomerativeClustering", "Agglomerative clustering method")
    {
        AddCaller("Add", NPyBind::CreateMethodCaller<NAgglomerative::TClusterization>(&NAgglomerative::TClusterization::Add));

        AddCaller("Build", NPyBind::CreateMethodCaller<NAgglomerative::TClusterization>(&NAgglomerative::TClusterization::Build));
        AddCaller("SetupRelevances", NPyBind::CreateMethodCaller<NAgglomerative::TClusterization>(&NAgglomerative::TClusterization::SetupRelevances));

        AddCaller("Clusters", NPyBind::CreateConstMethodCaller<NAgglomerative::TClusterization>(&NAgglomerative::TClusterization::GetClusters));

        AddCaller("Precision", NPyBind::CreateConstMethodCaller<NAgglomerative::TClusterization>(&NAgglomerative::TClusterization::GetPrecision));
        AddCaller("Recall", NPyBind::CreateConstMethodCaller<NAgglomerative::TClusterization>(&NAgglomerative::TClusterization::GetRecall));
    }
public:
    static NAgglomerative::TClusterization* GetObject(NAgglomerative::TClusterization& clusterization) {
        return &clusterization;
    }

    static NAgglomerative::TClusterization* DoInitObject(PyObject* args, PyObject*) {
        size_t elementsCount = 0;
        if (!NPyBind::ExtractArgs(args, elementsCount)) {
            ythrow yexception() << "Got error while initializing NAgglomerative::TClusterization: " << CurrentExceptionMessage();
        }
        return new NAgglomerative::TClusterization(elementsCount);
    }
};

PyMODINIT_FUNC initlibagglomerative() {
    NPyBind::TPyObjectPtr m = NPyBind::TModuleHolder::Instance().InitModule("libagglomerative");
    TAgglomerativeParamTraits::Instance().Register(m, "AgglomerativeClustering");
}
