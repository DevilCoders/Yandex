PY_ANY_MODULE(alignment_utils PREFIX "")

PYTHON3_MODULE()

OWNER(eranik)

PYTHON2_ADDINCL()

PEERDIR(
    contrib/python/numpy/include # add only headers for dynamic linking
)

NO_COMPILER_WARNINGS()

BUILDWITH_CYTHON_CPP(
  alignment_utils.pyx
)

IF (ARCH_AARCH64 OR OS_WINDOWS)
  ALLOCATOR(J)
ELSE()
  ALLOCATOR(LF)
ENDIF()

IF (OS_DARWIN)
  LDFLAGS(-headerpad_max_install_names)
ENDIF()

NO_LINT()

END()
