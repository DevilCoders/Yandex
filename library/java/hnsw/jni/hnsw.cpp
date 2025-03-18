//
// Created by Stanislav Lyahnovich on 25.09.18.
//
#include "ru_yandex_hnsw_HNSWIndex.h"

#include <library/cpp/hnsw/index/index_item_storage_base.h>
#include <library/cpp/hnsw/index/dense_vector_distance.h>
#include <library/cpp/hnsw/index_builder/build_options.h>
#include <library/cpp/hnsw/index_builder/dense_vector_index_builder.h>
#include <library/cpp/hnsw/index_builder/index_writer.h>
#include <library/cpp/hnsw/tools/build_dense_vector_index/distance.h>

#include <util/generic/buffer.h>
#include <util/generic/ptr.h>
#include <util/memory/blob.h>
#include <util/stream/buffer.h>

static jint JNI_VERSION = JNI_VERSION_1_8;

// ru.yandex.hnsw.HNSWIndex
static jclass JC_HNSWIndex;
//static jmethodID JMID_HNSWIndex_ctor;
static jfieldID JFID_HNSWIndex_indexAddress;

// ru.yandex.hnsw.HNSWResultFloat
static jclass JC_HNSWResultFloat;
static jmethodID JMID_HNSWResultFloat_ctor;

// ru.yandex.hnsw.DistanceType
static jclass JC_DistanceType;
static jmethodID JMID_DistanceType_ordinal;

// java.lang.IllegalStateException
static jclass JC_IllegalStateException;

using namespace NHnsw;

using TFloatDenseVectorStorage = TDenseVectorStorage<float>;

template<typename T>
class TJLongPtr {
private:
    T* ValuePtr;
    const jobject Obj;
    const jfieldID LongFieldId;
    JNIEnv*const Env;

public:
    TJLongPtr(JNIEnv *env, jobject obj, jfieldID longFieldId)
        : Obj(obj)
        , LongFieldId(longFieldId)
        , Env(env)
    {
        jlong address = env->GetLongField(obj, longFieldId);
        ValuePtr = reinterpret_cast<T*>(address);
    }

    T* operator-> () const {
        return ValuePtr;
    }

    T& operator* () const {
        return  *ValuePtr;
    }

    TJLongPtr& operator= (T* other) {
        if (ValuePtr) {
            delete ValuePtr;
        }
        ValuePtr = other;
        Env->SetLongField(Obj, LongFieldId, (jlong)ValuePtr);
        return *this;
    }

    TJLongPtr& operator= (THolder<T>&& other) {
        if (ValuePtr) {
            delete ValuePtr;
        }
        ValuePtr = other.Release();
        Env->SetLongField(Obj, LongFieldId, (jlong)ValuePtr);
        return *this;
    }

    explicit operator bool() const noexcept {
        return ValuePtr != nullptr;
    }

};

class FloatArrayWrapper {
private:
    jfloatArray Jfloats;
    jfloat* Array;
    JNIEnv*const Env;

public:
    FloatArrayWrapper(JNIEnv *env, jfloatArray jfloats)
        : Jfloats(jfloats)
        , Env(env)

    {
        // 0 here means "no copy"
        Array = env->GetFloatArrayElements(jfloats, 0);
    }

    ~FloatArrayWrapper() {
        Env->ReleaseFloatArrayElements(Jfloats, Array, 0);
    }

    float* getArray() const {
        return Array;
    }
};


jclass getClassReference(JNIEnv *env, const char *classname) {
    // STEP 1/3 : Load the class id
    jclass tempLocalClassRef = env->FindClass(classname);
    // STEP 2/3 : Assign the ClassId as a Global Reference
    jclass globalRef = (jclass) env->NewGlobalRef(tempLocalClassRef);
    // STEP 3/3 : Delete the no longer needed local reference
    env->DeleteLocalRef(tempLocalClassRef);

    return globalRef;
}

/**************************************************************
 * Initialize the static Class and Method Id variables
 **************************************************************/
jint JNI_OnLoad(JavaVM *vm, void *) {
    JNIEnv* env;
    if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION) != JNI_OK) {
        return JNI_ERR;
    }

    // ru.yandex.hnsw.HNSWResultFloat
    JC_HNSWResultFloat = getClassReference(env, "ru/yandex/hnsw/HNSWResultFloat");
    JMID_HNSWResultFloat_ctor = env->GetMethodID(JC_HNSWResultFloat, "<init>", "([I[F)V");
    // ----

    // ru.yandex.hnsw.DistanceType
    JC_DistanceType = getClassReference(env, "ru/yandex/hnsw/DistanceType");
    JMID_DistanceType_ordinal = env->GetMethodID(JC_DistanceType, "ordinal", "()I");

    // ru.yandex.hnsw.HNSWIndex
    JC_HNSWIndex = getClassReference(env, "ru/yandex/hnsw/HNSWIndex");
    //JMID_HNSWIndex_ctor = env->GetMethodID(JC_HNSWIndex, "<init>", "(JJ)V");
    JFID_HNSWIndex_indexAddress = env->GetFieldID(JC_HNSWIndex, "indexAddress", "J");

    JC_IllegalStateException = getClassReference(env, "java/lang/IllegalStateException");

    return JNI_VERSION;
}

/**************************************************************
 * Destroy the global static Class Id variables
 * According to http://docs.oracle.com/javase/1.5.0/docs/guide/jni/spec/invocation.html#JNI_OnUnload
 * The VM calls JNI_OnUnload when the class loader containing the native library is garbage collected.
 **************************************************************/
void JNI_OnUnload(JavaVM *vm, void *) {
    // Obtain the JNIEnv from the VM
    // NOTE: some re-do the JNI Version check here, but I find that redundant
    JNIEnv* env;
    vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION);

    // Destroy the global references
    env->DeleteGlobalRef(JC_HNSWResultFloat);
    env->DeleteGlobalRef(JC_DistanceType);
    env->DeleteGlobalRef(JC_HNSWIndex);
    env->DeleteGlobalRef(JC_IllegalStateException);
}

class THnswIndex: public THnswIndexBase {
public:
    explicit THnswIndex(const TBlob &indexBlob, const TFloatDenseVectorStorage &storage)
        : THnswIndexBase(indexBlob)
        , Data(indexBlob)
        , Storage(storage)
    {
    }

    void Save(IOutputStream& out) const {
        out.Write(Data.Begin(), Data.Size());
        out.Finish();
    }

    void Save(const char *filename) const {
        TUnbufferedFileOutput out(filename);
        Save(out);
    }

    const TFloatDenseVectorStorage& GetStorage() {
        return Storage;
    }

    size_t GetDataSize() const {
        return Data.Size();
    }

private:
    TBlob Data;
    TFloatDenseVectorStorage Storage;
};

template <class T>
inline TBlob BuildDenseVectorIndex(
    JNIEnv *env,
    THnswBuildOptions options,
    const NHnsw::TDenseVectorStorage<T> &storage,
    EDistance distance)
{
    THnswIndexData indexData;
    switch(distance) {
        case EDistance::DotProduct:
            indexData = BuildDenseVectorIndex<T, NHnsw::TDotProduct<T>>(options, storage, storage.GetDimension());
            break;
        case EDistance::L1Distance:
            indexData = BuildDenseVectorIndex<T, NHnsw::TL1Distance<T>>(options, storage, storage.GetDimension());
            break;
        case EDistance::L2SqrDistance:
            indexData = BuildDenseVectorIndex<T, NHnsw::TL2SqrDistance<T>>(options, storage, storage.GetDimension());
            break;
        default:
            env->ThrowNew(JC_IllegalStateException, "Unknown distance!");
            Y_FAIL("Unknown distance!");
    }
    TBuffer buffer;
    buffer.Reserve(ExpectedSize(indexData));
    TBufferOutput output(buffer);
    WriteIndex(indexData, output);
    output.Finish();
    return TBlob::FromBuffer(buffer);
}

JNIEXPORT void JNICALL Java_ru_yandex_hnsw_HNSWIndex_buildIndex(
    JNIEnv *env,
    jobject hnswIndex,
    jlong vectorsAddr,
    jint vectorsCount,
    jint dimension,
    jobject javaDistanceType,
    jstring javaJsonOptions)
{
    jint distancetypeOrdinal = env->CallIntMethod(javaDistanceType, JMID_DistanceType_ordinal);
    EDistance distanceType = static_cast<EDistance>(distancetypeOrdinal);
    float * vectorsPtr = (float *)vectorsAddr;
    const char *jsonOptions = env->GetStringUTFChars(javaJsonOptions, 0);
    const THnswBuildOptions options = THnswBuildOptions::FromJsonString(TString(jsonOptions));

    TBlob storageBlob = TBlob::NoCopy((const char *) vectorsPtr, (size_t)vectorsCount * dimension * sizeof(float));
    TFloatDenseVectorStorage storage(storageBlob, (size_t)dimension);

    TBlob indexBlob = BuildDenseVectorIndex(env, options, storage, distanceType);

    TJLongPtr<THnswIndex> indexHolder(env, hnswIndex, JFID_HNSWIndex_indexAddress);
    indexHolder = MakeHolder<THnswIndex>(indexBlob, storage);
}

jintArray WrapJArray(JNIEnv *env, jint * ints, size_t size) {
    jintArray jints = env->NewIntArray(size);
    env->SetIntArrayRegion(jints, 0, size, ints);
    return jints;
}

jfloatArray WrapJArray(JNIEnv *env, jfloat * floats, size_t size) {
    jfloatArray jfloats = env->NewFloatArray(size);
    env->SetFloatArrayRegion(jfloats, 0, size, floats);
    return jfloats;
}

template <class TResult>
inline jobject MakeHNSWResultFloatObject(JNIEnv *env, const TVector<THnswIndexBase::TNeighbor<TResult>>& neighbors) {
    const size_t size = neighbors.size();
    TArrayHolder<jint> tmpIx(new jint[size]);
    TArrayHolder<jfloat> tmpDist(new jfloat[size]);
    for (size_t i = 0; i < size; ++i) {
        tmpIx[i] = neighbors[i].Id;
        tmpDist[i] = neighbors[i].Dist;
    }
    jintArray jIx = WrapJArray(env, tmpIx.Get(), size);
    jfloatArray jDist = WrapJArray(env, tmpDist.Get(), size);

    return env->NewObject(JC_HNSWResultFloat, JMID_HNSWResultFloat_ctor, jIx, jDist);
}

template <class T>
inline jobject GetNearestNeighbors(
    JNIEnv *env,
    TJLongPtr<THnswIndex>& index,
    EDistance distance,
    T *query,
    size_t topSize,
    size_t searchNeighbors,
    size_t dimension)
{
    switch (distance) {
        case EDistance::DotProduct:{
            using TDistance = NHnsw::TDotProduct<T>;
            TDistanceWithDimension<T, TDistance> distanceWithDimension(TDistance(), dimension);
            const TVector<THnswIndexBase::TNeighbor<typename TDistance::TResult>> neighbors = index->GetNearestNeighbors(
                    query,
                    (size_t) topSize,
                    (size_t) searchNeighbors,
                    index->GetStorage(),
                    distanceWithDimension);
            return MakeHNSWResultFloatObject(env, neighbors);
        }
        case EDistance::L1Distance: {
            typedef NHnsw::TL1Distance<T> TDistance;
            TDistanceWithDimension<T, TDistance> distanceWithDimension(TDistance(), dimension);

            const TVector<THnswIndexBase::TNeighbor<typename TDistance::TResult>> neighbors = index->GetNearestNeighbors(
                        query,
                        (size_t) topSize,
                        (size_t) searchNeighbors,
                        Max<size_t>(),
                        index->GetStorage(),
                        distanceWithDimension);
            return MakeHNSWResultFloatObject(env, neighbors);
        }
        case EDistance::L2SqrDistance: {
            typedef NHnsw::TL2SqrDistance<T> TDistance;
            TDistanceWithDimension<T, TDistance> distanceWithDimension(TDistance(), dimension);
            const TVector<THnswIndexBase::TNeighbor<typename TDistance::TResult>> neighbors = index->GetNearestNeighbors(
                        query,
                        (size_t) topSize,
                        (size_t) searchNeighbors,
                        Max<size_t>(),
                        index->GetStorage(),
                        distanceWithDimension);

            return MakeHNSWResultFloatObject(env, neighbors);
        }
        default: {
            env->ThrowNew(JC_IllegalStateException, "Unknown distance!");
            Y_FAIL("Unknown distance!");
        }
    }
}

JNIEXPORT jobject JNICALL Java_ru_yandex_hnsw_HNSWIndex_getNearestNeighbors__JILru_yandex_hnsw_DistanceType_2II(
    JNIEnv *env,
    jobject hnswIndex,
    jlong vectorAddress,
    jint dimension,
    jobject javaDistanceType,
    jint topSize,
    jint searchNeighbors)
{
    // get indexAddress from hnswIndex.indexAddress object
    TJLongPtr<THnswIndex> index(env, hnswIndex, JFID_HNSWIndex_indexAddress);
    if (index) {
        float *query = reinterpret_cast<float *>(vectorAddress);

        jint distancetypeOrdinal = env->CallIntMethod(javaDistanceType, JMID_DistanceType_ordinal);
        EDistance distanceType = static_cast<EDistance>(distancetypeOrdinal);

        return GetNearestNeighbors(env, index, distanceType, query, topSize, searchNeighbors, dimension);
    } else {
        env->ThrowNew(JC_IllegalStateException, "indexAddress is NULL");
        return nullptr;
    }
}

JNIEXPORT jobject JNICALL Java_ru_yandex_hnsw_HNSWIndex_getNearestNeighbors___3FILru_yandex_hnsw_DistanceType_2II(
    JNIEnv *env,
    jobject hnswIndex,
    jfloatArray vector,
    jint dimension,
    jobject javaDistanceType,
    jint topSize,
    jint searchNeighbors)
{
    // get indexAddress from hnswIndex.indexAddress object
    TJLongPtr<THnswIndex> index(env, hnswIndex, JFID_HNSWIndex_indexAddress);
    if (index) {
        FloatArrayWrapper wrapper(env, vector);
        jint distancetypeOrdinal = env->CallIntMethod(javaDistanceType, JMID_DistanceType_ordinal);
        EDistance distanceType = static_cast<EDistance>(distancetypeOrdinal);

        return GetNearestNeighbors(env, index, distanceType, wrapper.getArray(), topSize, searchNeighbors, dimension);
    } else {
        env->ThrowNew(JC_IllegalStateException, "indexAddress is NULL");
        return nullptr;
    }
}

JNIEXPORT jlong JNICALL Java_ru_yandex_hnsw_HNSWIndex_getIndexSize(
    JNIEnv *env,
    jobject hnswIndex)
{
    TJLongPtr<THnswIndex> index(env, hnswIndex, JFID_HNSWIndex_indexAddress);
    return index->GetDataSize();
}

JNIEXPORT void JNICALL Java_ru_yandex_hnsw_HNSWIndex_saveToMemory(
    JNIEnv *env,
    jobject hnswIndex,
    jlong blobAddress,
    jlong blobSize)
{
    TJLongPtr<THnswIndex> index(env, hnswIndex, JFID_HNSWIndex_indexAddress);
    jlong indexSize = index->GetDataSize();
    if (blobSize >= indexSize) {
        TMemoryOutput blobStream((void *) blobAddress, (size_t) blobSize);
        index->Save(blobStream);
    } else {
        env->ThrowNew(JC_IllegalStateException, "blobSize is not enough to save index");
    }
}

JNIEXPORT void JNICALL Java_ru_yandex_hnsw_HNSWIndex_save(
    JNIEnv *env,
    jobject hnswIndex,
    jstring javaFilename)
{
    const char *filename = env->GetStringUTFChars(javaFilename, 0);
    TJLongPtr<THnswIndex> index(env, hnswIndex, JFID_HNSWIndex_indexAddress);
    index->Save(filename);
}

JNIEXPORT void JNICALL Java_ru_yandex_hnsw_HNSWIndex_loadFromFile(
    JNIEnv *env,
    jobject hnswIndex,
    jstring javaFilename,
    jlong storageAddress,
    jint vectorsCount,
    jint dimension)
{
    const char *filename = env->GetStringUTFChars(javaFilename, 0);

    TBlob storageBlob = TBlob::NoCopy((const char *) storageAddress, (size_t)vectorsCount * (size_t)dimension * sizeof(float));
    TFloatDenseVectorStorage storage(storageBlob, (size_t)dimension);

    const TBlob &indexBlob = TBlob::FromFileContent(filename);
    TJLongPtr<THnswIndex> index(env, hnswIndex, JFID_HNSWIndex_indexAddress);
    index = MakeHolder<THnswIndex>(indexBlob, storage);
}

JNIEXPORT void JNICALL Java_ru_yandex_hnsw_HNSWIndex_loadFromFiles(
    JNIEnv *env,
    jobject hnswIndex,
    jstring javaIndexFilename,
    jstring javaVectorsFilename,
    jint dimension)
{
    const char *indexFilename = env->GetStringUTFChars(javaIndexFilename, 0);
    const char *vectorsFilename = env->GetStringUTFChars(javaVectorsFilename, 0);

    TFloatDenseVectorStorage storage(vectorsFilename, (size_t)dimension);

    const TBlob &indexBlob = TBlob::FromFileContent(indexFilename);
    TJLongPtr<THnswIndex> index(env, hnswIndex, JFID_HNSWIndex_indexAddress);
    index = MakeHolder<THnswIndex>(indexBlob, storage);
}

JNIEXPORT void JNICALL Java_ru_yandex_hnsw_HNSWIndex_loadFromAddr(
    JNIEnv *env,
    jobject hnswIndex,
    jlong indexBlobAddr,
    jlong indexBlobSize,
    jlong storageAddress,
    jint vectorsCount,
    jint dimension,
    jboolean copyIndexMemory)
{
    TBlob storageBlob = TBlob::NoCopy((const char *) storageAddress, (size_t)vectorsCount * (size_t)dimension * sizeof(float));
    TFloatDenseVectorStorage storage(storageBlob, (size_t)dimension);

    TBlob indexBlob = copyIndexMemory ?
        TBlob::Copy((const char *) indexBlobAddr, indexBlobSize)
        : TBlob::NoCopy((const char *) indexBlobAddr, indexBlobSize);

    TJLongPtr<THnswIndex> index(env, hnswIndex, JFID_HNSWIndex_indexAddress);
    index = MakeHolder<THnswIndex>(indexBlob, storage);
}

JNIEXPORT void JNICALL Java_ru_yandex_hnsw_HNSWIndex_dispose(
    JNIEnv *env,
    jobject hnswIndex)
{
    TJLongPtr<THnswIndex> index(env, hnswIndex, JFID_HNSWIndex_indexAddress);
    index = nullptr;
}
