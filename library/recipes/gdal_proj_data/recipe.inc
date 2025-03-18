USE_RECIPE(library/recipes/gdal_proj_data/gdal_proj_data)

DEPENDS(
    library/recipes/gdal_proj_data
)

DATA(
    arcadia/contrib/libs/proj/data
    arcadia/contrib/libs/gdal/data
)
