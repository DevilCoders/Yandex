from __future__ import print_function

import inspect
import json
from testenv.jobs.docs import DeployDocs
from testenv.jobs.docs.DeployDocs import *  # noqa: F403


def main():
    result = []
    for attr in dir(DeployDocs):
        clazz = getattr(DeployDocs, attr)
        if inspect.isclass(clazz) and issubclass(clazz, CloudBaseDeployDocs):  # noqa: F405
            result.append({
                'name': clazz.__name__,
                'owners': clazz.Owners,
                'observedPaths': clazz.ObservedPaths,
                'targetsToBuild': clazz.TargetsToBuild
            })

    print(json.dumps(result))


if __name__ == "__main__":
    main()
