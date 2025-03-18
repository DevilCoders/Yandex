"""Directory with various utils. Utils are distributed in the following way
    - common: common utils, used by everybody everywhere
    - pregen: utils, which usually are executed on first stage of recluster (in free hosts phase)
    - postgen: utils, which usually are execute on last stage of recluster (when everything is done, and we only need only like add ints and so on)
    - sky: utils, which are usually used to run something by skynet and gather remote data
    - check: utils, which are used to control internal integrity of db. They are executed on every build in check phase
    - admin: ???
    - mongo: utils to manipulate with mongo db statistics
    - clickhouse: utils to work with clickhouse statistics
    - rare: common utils, which are executed not quite often (thus they can be easily broken)
    - standalone: utils, which can be executed without anything from gencfg (only standard python packages imported)
    - other: unsorted stuff, which can not be moved to one of previous group
    - api: api-specific utils, used to contruct reply to specific requests
    - binary: auxiliarily binary utils
"""
