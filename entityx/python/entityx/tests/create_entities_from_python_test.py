import entityx
from entityx_python_test import Position


class EntityA(entityx.Entity):
    position = entityx.Component(Position, 1, 2)


def create_entities_from_python_test():
    a = EntityA()
    assert a._entity.id == 0
    assert a.position.x == 1.0
    assert a.position.y == 2.0

    b = EntityA()
    assert b._entity.id == 1

    a.destroy()
    c = EntityA()
    # Reuse destroyed entitys ID.
    assert c._entity.id == 0
