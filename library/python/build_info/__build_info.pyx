import future.utils as fu

cdef extern from "library/python/build_info/interface/build_info_static.h":
    cdef const char* GetBuildType() except +;
    cdef const char* GetCompilerVersion() except +;
    cdef const char* GetPyCompilerFlags() except +;
    cdef const char* GetPyBuildInfo() except +;
    cdef const char* GetSandboxTaskId() except +;

def build_type():
    return fu.bytes_to_native_str(GetBuildType())

def compiler_version():
    return fu.bytes_to_native_str(GetCompilerVersion())

def compiler_flags():
    return fu.bytes_to_native_str(GetPyCompilerFlags())

def build_info():
    return fu.bytes_to_native_str(GetPyBuildInfo())

def sandbox_task_id():
    return fu.bytes_to_native_str(GetSandboxTaskId())
