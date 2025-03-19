#include "service.h"

static struct yfs_service_factory *g_service_factory;

////////////////////////////////////////////////////////////////////////////////

void yfs_service_factory_init(struct yfs_service_factory *sf)
{
    g_service_factory = sf;
}

struct yfs_service_factory *yfs_service_factory_get()
{
    return g_service_factory;
}
