from fastapi import FastAPI

import api.routes.agents
import api.routes.cluster_variables
import api.routes.clusters
import api.routes.health
import api.routes.prober_configs
import api.routes.prober_files
import api.routes.probers
import api.routes.recipe_files
import api.routes.recipes


def register_all_routes(app: FastAPI):
    app.include_router(api.routes.agents.router)
    app.include_router(api.routes.health.router)
    app.include_router(api.routes.clusters.router)
    app.include_router(api.routes.recipes.router)
    app.include_router(api.routes.probers.router)
    app.include_router(api.routes.recipe_files.router)
    app.include_router(api.routes.prober_files.router)
    app.include_router(api.routes.prober_configs.router)
    app.include_router(api.routes.cluster_variables.router)
