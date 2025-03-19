PY3_PROGRAM()
OWNER(
    d-kruchinin
    hotckiss
)

PEERDIR(
    contrib/python/boto3
    contrib/python/docker
    contrib/python/Flask
    contrib/python/retry
    contrib/python/requests-unixsocket
    yt/python/client
    cloud/ai/nirvana/nv_launcher_agent/lib
)

PY_SRCS(
    MAIN app.py
    agent.py
    controller.py
    docker_cache.py
    docker_client.py
    docker_creator.py
    deploy_nirvana.py
    job.py
    job_executor.py
    job_manager.py
    job_status.py
    layer_manager.py
    proxy.py
    secrets.py
    service.py
    test.py
    deployer.py
)

END()
