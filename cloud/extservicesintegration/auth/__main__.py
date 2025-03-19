from functools import wraps

import cachetools.func
import tornado
import tornado.ioloop
import tornado.web

from gauthling_daemon import GauthlingClient
from yc_auth.authentication import TokenAuth
from yc_auth.authorization import authorize


def singleton_provider(func):
    # nonlocal
    instance_holder = [None]

    @wraps(func)
    def wrapped(*args, **kwargs):
        instance = instance_holder[0]
        if not instance:
            instance = instance_holder[0] = func(*args, **kwargs)
        return instance

    return wrapped


@singleton_provider
def gauthling_client():
    return GauthlingClient("gauthling.man1.dogfood.cloud.yandex.net:4284")


@cachetools.func.ttl_cache(maxsize=1024, ttl=10)
def auth_authz(token, projectId):
    auth = TokenAuth(gauthling_client)
    auth_ctx = auth.authenticate(token)
    authorize(
        gauthling_client=gauthling_client(),
        service="compute",
        action="list_instances",
        token=auth_ctx.token,
        project_id=projectId,
    )


class AuthHandler(tornado.web.RequestHandler):
    def get(self):
        token = self.request.headers.get("X-YaCloud-SubjectToken")
        projectId = self.request.headers.get("X-YaCloud-ProjectId")
        auth_authz(token, projectId)
        print "OK"


def make_app():
    return tornado.web.Application([(r"/auth", AuthHandler)])

if __name__ == "__main__":
    app = make_app()
    app.listen(8090)
    tornado.ioloop.IOLoop.current().start()
