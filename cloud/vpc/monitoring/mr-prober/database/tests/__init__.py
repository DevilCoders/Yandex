import pytest
from sqlalchemy.exc import IntegrityError

import database
from database.models import (
    Prober,
    ProberConfig,
    ProberVariable,
)


def test_prober_variable(test_database):
    db = database.session_maker()
    prober = Prober(
        id=1,
        arcadia_path="path",
        name="prober name",
        slug="prober_name",
        description="prober description",
    )
    prober_config = ProberConfig(
        id=1,
        prober_id=prober.id,
    )
    db.add(prober)
    db.add(prober_config)
    db.commit()

    # try to add variable without name
    prober_var_1 = ProberVariable(
        prober_config_id=prober_config.id,
        value="value",
    )
    db.add(prober_var_1)
    with pytest.raises(IntegrityError):
        db.commit()

    db.rollback()

    # add variable with name
    prober_var_1.name = "var_name"
    db.add(prober_var_1)
    db.commit()

    db.close()
