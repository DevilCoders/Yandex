LIBRARY()

OWNER(kostik)

SRCS(
    action_points.h
    adapter.h
    copy_points.h
    counter_module.h
    file_module.h
    file_reader.h
    file_writer.h
    files_writer.h
    input_point.h
    mem_2darray.h
    mem_file.h
    mirror_module.h
    misc_points.h
    module_factory.h
    output_point.h
    safe_caller.h
    sequence_caller.h
    simple_factory.h
    stream_points.h
    value_binders.h
    vector_buffer.h
    vector_sort_module.h
    yhash_module.h
    2darray_reader.cpp
    2darray_writer.cpp
    access_point_info.cpp
    access_points.cpp
    calc_module.cpp
    map_helpers.cpp
    simple_calc_net.cpp
    simple_module.cpp
)

PEERDIR(
    library/cpp/deprecated/atomic
)

END()
