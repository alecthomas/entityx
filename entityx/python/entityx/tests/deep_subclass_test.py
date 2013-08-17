from entityx import Entity, Component
from entityx_python_test import Position, Direction


class BaseEntity(Entity):
    direction = Component(Direction)


class DeepSubclassTest(BaseEntity):
    position = Component(Position)

    def test_deep_subclass(self):
        assert self.direction
        assert self.position


class DeepSubclassTest2(DeepSubclassTest):
    position2 = Component(Position)

    def test_deeper_subclass(self):
        assert self.direction
        assert self.position
        assert self.position2
        assert self.position is self.position
