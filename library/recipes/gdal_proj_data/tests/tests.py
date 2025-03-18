import pyproj
import pytest
from osgeo import osr


def test_pyproj_data_directory():
    geo_proj = pyproj.Proj("+init=epsg:4326")
    mer_proj = pyproj.Proj("+init=epsg:3395")
    # Moscow coordinates
    lat, lon = [55.751244], [37.618423]
    x_res, y_res = pyproj.transform(geo_proj, mer_proj, lon, lat)
    assert x_res[0] == pytest.approx(4187663.6928059706)
    assert y_res[0] == pytest.approx(7473705.970167414)


def test_gdal_data_directory():
    geo_proj = osr.SpatialReference()
    geo_proj.ImportFromEPSG(4326)
    mer_proj = osr.SpatialReference()
    mer_proj.ImportFromEPSG(3395)
    geo_to_mer = osr.CoordinateTransformation(geo_proj, mer_proj)
    # Moscow coordinates
    lat, lon = 55.751244, 37.618423
    x_res, y_res, z_res = geo_to_mer.TransformPoint(lon, lat)
    assert x_res == pytest.approx(4187663.6928059706)
    assert y_res == pytest.approx(7473705.970167414)
    assert z_res == 0
