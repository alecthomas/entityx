import entityx
from entityx_python_test import Position


class EntityA(entityx.Entity):
    position = entityx.Component(Position, 1, 2)

    def __init__(self, x=None, y=None):
        if x is not None:
            self.position.x = x
        if y is not None:
            self.position.y = y


def create_entities_from_python_test():
    a = EntityA()
    assert a._entity.id & 0xffffffff == 0
    assert a.position.x == 1.0
    assert a.position.y == 2.0

    b = EntityA()
    assert b._entity.id & 0xffffffff == 1

    a.destroy()
    c = EntityA()
    # Reuse destroyed index of "a".
    assert c._entity.id & 0xffffffff == 0
    # However, version is different
    assert a._entity.id != c._entity.id and c._entity.id > a._entity.id

    d = EntityA(2.0, 3.0)
    assert d.position.x == 2.0
    assert d.position.y == 3.0
