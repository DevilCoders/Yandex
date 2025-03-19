#include "../../mn_file.h"
#include "../util/yexception.h"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <functional>
#include <iostream>
#include <random>
#include <sstream>

using NMatrixnet::yexception;


//-----------------------------------------------------------------------------
// TCommandLine
//-----------------------------------------------------------------------------

class TCommandLine {
public:
    TCommandLine(int argc, char* argv[]) {
        ExecutablePath_ = argv[0];
        size_t slash_pos = ExecutablePath_.find_last_of("/\\");
        if (slash_pos == std::string::npos)
            ythrow yexception() << "Couldn't extract executable directory from the command line";
        ExecutableDir_ = ExecutablePath_.substr(0, slash_pos);
        std::vector<std::string> params(&argv[1], &argv[argc]);
        IsSaveMode_ = !params.empty() && (_stricmp(params[0].c_str(), "SAVE") == 0);
    }

    std::string GetExecutablePath() const { return ExecutablePath_; }
    std::string GetExecutableDir() const { return ExecutableDir_; }
    bool IsSaveMode() const { return IsSaveMode_; }

private:
    std::string ExecutablePath_;
    std::string ExecutableDir_;
    bool IsSaveMode_;
};


//-----------------------------------------------------------------------------
// TParamFile
//-----------------------------------------------------------------------------

template<typename T>
class TParamFileBase {
protected:
    static void ExtractValue(std::istream& istrm, T& out) {
        istrm >> out;
    }
};

template<>
class TParamFileBase<std::string> {
protected:
    static void ExtractValue(std::istream& istrm, std::string& out) {
        std::getline(istrm, out);
    }
};

template<typename T>
class TParamFile: public TParamFileBase<T> {
public:
    static T Read(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            ythrow yexception() << "Couldn't open file " << filename;
        }
        T result;
        ExtractValue(file, result);
        if (file.fail()) {
            ythrow yexception() << "Couldn't read a value from file " << filename;
        }
        std::cout << "Loaded file " + filename << "." << std::endl;
        return result;
    }
};


//-----------------------------------------------------------------------------
// TTimePoint
//-----------------------------------------------------------------------------

class TTimePoint {
public:
    static TTimePoint Now() {
        TTimePoint pt;
        pt.Clock_ = std::chrono::system_clock::now();
        return pt;
    }

    int GetDurationMs(const TTimePoint& before) const {
        return static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(Clock_ - before.Clock_).count());
    }

private:
    TTimePoint() {}
    std::chrono::time_point<std::chrono::system_clock> Clock_;
};


//-----------------------------------------------------------------------------
// TDocuments
//-----------------------------------------------------------------------------

class TDocuments;
using TDocumentsPtr = std::shared_ptr<TDocuments>;

class TDocuments {
public:
    static TDocumentsPtr GenerateRandom(size_t numDocuments, size_t numFactors, int seed) {
        TDocumentsPtr doc(new TDocuments);
        doc->Resize(numDocuments, numFactors);
        std::uniform_real_distribution<float> distribution(0, 1);
        std::mt19937 engine(static_cast<std::mt19937::result_type>(seed));
        auto generator = std::bind(distribution, engine);
        std::generate_n(doc->AllFactors_.begin(), doc->AllFactors_.size(), generator);
        std::cout << "Generated " << numDocuments << " documents with " << numFactors << " random factors each." << std::endl;
        return doc;
    }

    size_t GetNumDocuments() const {
        return NumDocuments_;
    }

    size_t GetNumFactors() const {
        return NumFactors_;
    }

    const float* const* GetDocsFactors() const {
        return DocsFactors_.data();
    }

private:
    TDocuments() {}

    void Resize(size_t numDocuments, size_t numFactors) {
        NumDocuments_ = numDocuments;
        NumFactors_ = numFactors;
        AllFactors_.resize(NumDocuments_ * NumFactors_);
        DocsFactors_.resize(NumDocuments_);
        for (size_t i = 0; i != NumDocuments_; ++i)
            DocsFactors_[i] = &AllFactors_[i * NumFactors_];
    }

    size_t NumDocuments_;
    size_t NumFactors_;
    std::vector<float> AllFactors_;
    std::vector<const float*> DocsFactors_;
};


//-----------------------------------------------------------------------------
// TRelevances
//-----------------------------------------------------------------------------

class TRelevances;
using TRelevancesPtr = std::shared_ptr<TRelevances>;

class TRelevances {
public:
    static TRelevancesPtr Calculate(const NMatrixnet::IRelevCalcer& relevCalcer, const TDocuments& documents) {
        TRelevancesPtr relevs(new TRelevances);
        size_t numRelevances = documents.GetNumDocuments();
        relevs->Resize(numRelevances);
        TTimePoint startTime = TTimePoint::Now();
        relevCalcer.DoCalcRelevs(documents.GetDocsFactors(), relevs->Relevances_.data(), numRelevances);
        TTimePoint endTime = TTimePoint::Now();
        std::cout << "Calculated " << numRelevances << " relevances, "
                  << "elapsed " << endTime.GetDurationMs(startTime) << " milliseconds." << std::endl;
        return relevs;
    }

    static TRelevancesPtr Load(const std::string& filename) {
        TRelevancesPtr relevs(new TRelevances);
        std::ifstream file(filename);
        if (!file.is_open()) {
            ythrow yexception() << "Couldn't open file " << filename;
        }
        while (!file.eof()) {
            double relevance;
            file >> relevance;
            if (file.fail()) {
                ythrow yexception() << "Couldn't read values from file " << filename;
            }
            while (isspace(file.peek()))
                file.get();
            relevs->Relevances_.push_back(relevance);
        }
        std::cout << "Loaded file " << filename << "." << std::endl;
        return relevs;
    }

    void Save(const std::string& filename) const {
        std::ofstream file(filename);
        if (!file.is_open()) {
            ythrow yexception() << "Couldn't open file " << filename << " for writing";
            return;
        }
        for (size_t i = 0; i != Relevances_.size(); ++i) {
            double relevance = Relevances_[i];
            if (i != 0)
                file << " ";
            file << relevance;
            if (file.fail())
                ythrow yexception() << "Couldn't write values to file " << filename;
        }
        std::cout << "Saved file " << filename << "." << std::endl;
    }

    void Expect(const TRelevances& expectedRelevs) const {
        size_t sz1 = Relevances_.size();
        size_t sz2 = expectedRelevs.Relevances_.size();
        if (sz1 != sz2) {
            ythrow yexception() << "Numbers of relevances are not equal: " << sz1 << " != " << sz2;
        }
        for (size_t i = 0; i != sz1; ++i) {
            double r1 = Relevances_[i];
            double r2 = expectedRelevs.Relevances_[i];
            if (fabs(r1 - r2) > 0.00001 * std::max(r1, r2)) {
                ythrow yexception() << "Relevances #" << i << " are not equal: " << r1 << " != " << r2;
            }
        }
        std::cout << "Relevances are the same as expected." << std::endl;
    }

private:
    TRelevances() {}

    void Resize(size_t NumRelevances) {
        Relevances_.resize(NumRelevances);
    }

    std::vector<double> Relevances_;
};


//-----------------------------------------------------------------------------
// Main
//-----------------------------------------------------------------------------

void DoMain(int argc, char *argv[]) {
    TCommandLine commandLine(argc, argv);
    std::string path_to_data_dir = TParamFile<std::string>::Read(commandLine.GetExecutableDir() + "/path_to_data_dir.txt");

    NMatrixnet::TMnSseFilePtr model(new NMatrixnet::TMnSseFile((path_to_data_dir + "/model.info").c_str()));

    size_t numFactors = model->MaxFactorIndex() + 1;
    size_t numDocuments = TParamFile<size_t>::Read(path_to_data_dir + "/num_documents.txt");
    int seed = TParamFile<int>::Read(path_to_data_dir + "/seed.txt");
    TDocumentsPtr documents = TDocuments::GenerateRandom(numDocuments, numFactors, seed);

    TRelevancesPtr calculatedRelevs = TRelevances::Calculate(*model, *documents);

    if (commandLine.IsSaveMode()) {
        calculatedRelevs->Save(path_to_data_dir + "/relevances.txt");
    } else {
        TRelevancesPtr expectedRelevs = TRelevances::Load(path_to_data_dir + "/relevances.txt");
        calculatedRelevs->Expect(*expectedRelevs);
    }
}

int main(int argc, char *argv[]) {
#if !defined(MATRIXNET_WITHOUT_ARCADIA_NO_EXCEPTIONS)
    try {
#endif // !defined(MATRIXNET_WITHOUT_ARCADIA_NO_EXCEPTIONS)

        DoMain(argc, argv);
        std::cout << "Everything's OK." << std::endl;
        return EXIT_SUCCESS;

#if !defined(MATRIXNET_WITHOUT_ARCADIA_NO_EXCEPTIONS)
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
#endif // !defined(MATRIXNET_WITHOUT_ARCADIA_NO_EXCEPTIONS)
}
