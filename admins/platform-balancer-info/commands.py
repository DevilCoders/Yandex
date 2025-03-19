from io import StringIO
from typing import Dict

from .utils.Platform import Platform
from .utils.Yasm import list_alerts

reset = "\x1b[0m"


def green(msg):
    green = "\x1b[32;1m"
    return f"{green}{msg}{reset}"


def red(msg):
    bold_red = "\x1b[31;1m"
    return f"{bold_red}{msg}{reset}"


def project_check(pl: Platform, defaults: Dict, environment=None, application=None):
    apps = pl.project()
    env_apps = []
    for app in apps.applications:
        for env in app.environments:
            if environment and application:
                if not env.name == environment and not app.name == application:
                    continue

            if environment and not env.name == environment:
                continue

            if application and not app.name == application:
                continue

            env_apps.append(env)

    for app in env_apps:
        out = StringIO()
        env = pl.environment(app.objectId)
        out.write(f"{env.applicationName}\n")

        if env.components:
            env_checks(defaults, env, out, pl)

        if env.routes:
            route_checks(defaults, env, out)

        if env.name == "production" and env.routers:
            check_routers(pl, app, env, out)
        preped = out.getvalue()
        out.close()
        print(preped)


def check_routers(pl: Platform, app, env, out):
    out.write(f" routers:\n")
    for r in env.routers:
        out.write(f"  {r.name}\n")
        out.write(f"   monitoring: ")

        if r.name in ["common", "cdn", "common-public", "cdn-router"]:
            out.write(f"{red('error ')} ( migrate from {r.name} balancer)")
            continue

        alerts = list_alerts(f"{env.projectName}.balancer.{r.name}")
        if not alerts:
            out.write(f"{red('missing ')}")
        else:
            out.write('\n')
            for a in alerts:
                out.write(f"    {a['name']}\n")


def env_checks(defaults, env, out, pl):
    out.write(f" components:\n")

    for _, comp in env.components.items():
        out.write(f"  {env.name}.{comp.name}\n")

        inst = pl.instance(comp.objectId, env.version)

        out.write("   healthCheck: ")
        if not inst.useHealthCheck:
            out.write(f"{red('not set')}\n")
        else:
            out.write(f"{green('ok')}\n")

            out.write("   checkFail: ")
            if inst.healthCheckFall > defaults["component"]["checkFail"]:
                tmpl = f"{red('error')} (should be {defaults['component']['checkFail']})\n"
                out.write(tmpl)
            else:
                out.write(f"{green('ok')}\n")

            out.write("   checkRise: ")
            if inst.healthCheckFall > defaults["component"]["checkRise"]:
                tmpl = f"{red('error')} (should be {defaults['component']['checkRise']})\n"
                out.write(tmpl)
            else:
                out.write(f"{green('ok')}\n")

            out.write("   checkTimeout: ")
            if inst.healthCheckTimeout > defaults["component"]["checkTimeout"]:
                tmpl = f"{red('error')} (should be {defaults['component']['checkTimeout']})\n"
                out.write(tmpl)
            else:
                out.write(f"{green('ok')}\n")

        out.write(f"   monitoring: ")
        alerts = list_alerts(f"{env.projectName}.{env.applicationName}.{env.name}.{comp}")
        if not alerts:
            out.write(f"{red('missing ')}\n")
        else:
            out.write('\n')
            for a in alerts:
                out.write(f"    {a['name']}\n")


def route_checks(defaults, env, out):
    out.write(f" routes:\n")
    for route in env.routes:
        out.write(f"  {route.location}\n")
        out.write(f"   proxyNextUpstreamTries: ")
        if route.proxyNextUpstreamTries > defaults["route"]["retries"]:
            tmpl = f"{red('error')} (should be {defaults['route']['retries']})\n"
            out.write(tmpl)
        else:
            out.write(f"{green('ok')}\n")


if __name__ == '__main__':
    from pathlib import Path
    import yaml


    def get_defaults(path):
        f = Path(path)
        with f.open() as fojb:
            data = yaml.safe_load(fojb.read())
        return data


    path = "./best_practice.yaml"
    pl = Platform('ott')
    project_check(pl, get_defaults(path), 'production')
