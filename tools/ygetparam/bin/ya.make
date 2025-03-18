OWNER(
  knoxja
  g:antiinfra
)

PY2_PROGRAM(ygetparam)

PY_MAIN(
    tools.ygetparam.ygetparam:run
)

PEERDIR(
    tools/ygetparam
    tools/ygetparam/ygetparam_modules
    tools/ygetparam/ygetparam_modules/etcd
)

END()
