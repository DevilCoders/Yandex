from cython.operator import dereference, postincrement

from libcpp cimport bool
from util.generic.hash cimport THashMap
from util.generic.vector cimport TVector
from util.generic.string cimport TString
from util.system.types cimport ui32, ui64


cdef extern from "tools/mstand/squeeze_lib/lib/common.h" namespace "NMstand" nogil:
    ctypedef struct TFilter:
        TString Name
        TString Value

    ctypedef struct TExperimentForSqueeze:
        TVector[TFilter] Filters
        TString FilterHash
        TString Service
        TString TempTablePath
        TString Testid
        ui32 TableIndex

    ctypedef struct TYtParams:
        bool AddAcl
        TVector[TString] SourcePaths
        TVector[TString] YtFiles
        TVector[TString] YuidPaths
        TString LowerKey
        TString Pool
        TString Server
        TString TransactionId
        TString UpperKey
        ui64 DataSizePerJob
        ui64 MemoryLimit


cdef extern from "tools/mstand/squeeze_lib/lib/squeeze_impl.h" namespace "NMstand" nogil:
    THashMap[TString, ui32] GetSqueezeVersions(const TString& service) except +
    TString CreateTempTable(const TString& service)
    TString SqueezeDay(
        const TVector[TExperimentForSqueeze]& experiments,
        const TYtParams& ytParams,
        const THashMap[TString, TVector[TFilter]]& filters
    )


def get_squeeze_versions(service):
    result = {}

    cdef THashMap[TString, ui32] versions = GetSqueezeVersions(service.encode())
    cdef THashMap[TString, ui32].iterator it = versions.begin()
    while it != versions.end():
        name = dereference(it).first
        version = dereference(it).second
        result[name.decode()] = version
        postincrement(it)

    return result

def create_temp_table(TString service):
    return CreateTempTable(service)


def squeeze_day(experiments_for_squeeze, tmp_tables, service_params, transaction_id):
    cdef TYtParams c_yt_params
    c_yt_params.AddAcl = service_params.add_acl

    c_yt_params.SourcePaths = [p.encode() for p in service_params.paths.sources]
    c_yt_params.YuidPaths = [p.encode() for p in service_params.paths.yuids]

    c_yt_params.YtFiles = [p.encode() for p in service_params.local_files.yt_files] + [b"//home/mstand/resources/geodata4.bin"]


    table_bounds = service_params.table_bounds
    if table_bounds.lower_reducer_key:
        c_yt_params.LowerKey = table_bounds.lower_reducer_key.encode()
    if table_bounds.upper_reducer_key:
        c_yt_params.UpperKey = table_bounds.upper_reducer_key.encode()

    yt_job_options = service_params.yt_job_options
    if yt_job_options.data_size_per_job:
        c_yt_params.DataSizePerJob = yt_job_options.data_size_per_job * 1024 * 1024
    else:
        c_yt_params.DataSizePerJob = 300 * 1024 * 1024
    if yt_job_options.memory_limit:
        c_yt_params.MemoryLimit = yt_job_options.memory_limit * 1024 * 1024
    if yt_job_options.pool:
        c_yt_params.Pool = yt_job_options.pool.encode()

    server = service_params.client.config["proxy"].get("url")
    if server:
        c_yt_params.Server = server.encode()
    c_yt_params.TransactionId = transaction_id.encode()

    cdef TVector[TExperimentForSqueeze] c_experiments_for_squeeze
    cdef TExperimentForSqueeze c_efs
    cdef TFilter c_filter
    for i, (efs, tmp_table) in enumerate(zip(experiments_for_squeeze, tmp_tables)):
        if efs.filter_hash:
            c_efs.FilterHash = efs.filter_hash.encode()
        c_efs.Service = efs.service.encode()
        c_efs.Testid = efs.testid.encode()
        c_efs.TempTablePath = tmp_table.encode()
        c_efs.TableIndex = i

        c_efs.Filters.clear()
        for name, value in efs.filters.libra_filters:
            c_filter.Name = name.encode()
            c_filter.Value = value.encode()
            c_efs.Filters.push_back(c_filter)

        c_experiments_for_squeeze.push_back(c_efs)

    cdef THashMap[TString, TVector[TFilter]] c_filters

    operation_id = SqueezeDay(c_experiments_for_squeeze, c_yt_params, c_filters)
    return operation_id.decode()
