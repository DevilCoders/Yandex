_scenario() {
    HOSTS = !hostlist:HOSTS !hostlist:HOSTS:clusters

    HOSTS a:
    HOSTS b: a [0->0,1->1]
}
