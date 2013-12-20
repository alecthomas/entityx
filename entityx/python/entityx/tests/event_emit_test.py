from entityx import Entity, emit
from entityx_python_test import Collision


class AnEntity(Entity):
    pass


def emit_collision_from_python():
    a = AnEntity()
    b = AnEntity()
    collision = Collision(a, b)
    emit(collision)
