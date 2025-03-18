#include <library/cpp/pybind/typedesc.h>
#include <library/cpp/edit_distance/edit_distance.h>

/*
 * class TContextFreeLevenshtein:
 *   def __init__(self, filename):
 *     """initialize context-free levenshtein model from serialized reprezentaion from file"""
 *   def CalcDistance(self, lhs, rhs):
 *     """calculate distance between 2 unicode strings lhs & rhs"""
 *
 */

struct TContextFreeLevenshteinHolder {
    THolder<NEditDistance::TContextFreeLevenshtein> InternalModel;

    TContextFreeLevenshteinHolder(NEditDistance::TContextFreeLevenshtein* model)
        : InternalModel(model)
    {
    }
};

class TContextFreeLevenshteinTraits: public NPyBind::TPythonType<TContextFreeLevenshteinHolder, NEditDistance::TContextFreeLevenshtein, TContextFreeLevenshteinTraits> {
private:
    typedef class NPyBind::TPythonType<TContextFreeLevenshteinHolder, NEditDistance::TContextFreeLevenshtein, TContextFreeLevenshteinTraits> TParent;
    friend class NPyBind::TPythonType<TContextFreeLevenshteinHolder, NEditDistance::TContextFreeLevenshtein, TContextFreeLevenshteinTraits>;
    TContextFreeLevenshteinTraits();

public:
    static TContextFreeLevenshteinHolder* DoInitObject(PyObject* args, PyObject* kwargs);
    static NEditDistance::TContextFreeLevenshtein* GetObject(const TContextFreeLevenshteinHolder& holder) {
        return holder.InternalModel.Get();
    }
};

TContextFreeLevenshteinTraits::TContextFreeLevenshteinTraits()
    : TParent("_edit_distance.TContextFreeLevenshtein", "context free levenshtein model")
{
    AddCaller("CalcDistance", NPyBind::CreateConstMethodCaller<NEditDistance::TContextFreeLevenshtein>(&NEditDistance::TContextFreeLevenshtein::CalcDistance));
}

TContextFreeLevenshteinHolder* TContextFreeLevenshteinTraits::DoInitObject(PyObject* args, PyObject* /*kwargs*/) {
    TString filename;
    if (!NPyBind::ExtractArgs(args, filename))
        ythrow yexception() << "wrong parameters for TContextFreeLevenshtein()";
    THolder<TContextFreeLevenshteinHolder> holder(new TContextFreeLevenshteinHolder(new NEditDistance::TViterbiLevenshtein()));
    holder->InternalModel->LoadModel(filename);
    return holder.Release();
}

static PyMethodDef _EditDistanceMethods[] = {
    {nullptr, nullptr, 0, nullptr}};

PyMODINIT_FUNC init_edit_distance() {
    PyObject* m = Py_InitModule("_edit_distance", _EditDistanceMethods);
    TContextFreeLevenshteinTraits::Instance().Register(m, "TContextFreeLevenshtein");
}
