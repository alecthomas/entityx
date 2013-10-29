from entityx import Entity


class EventTest(Entity):
    collided = False

    def on_collision(self, event):
        assert event.a
        assert event.b
        self.collided = True
