from entityx import Entity, Component
from entityx_python_test import Position


class ConstructorTest(Entity):
    position = Component(Position)

    def __init__(self, x, y):
        self.position.x = x
        self.position.y = y
