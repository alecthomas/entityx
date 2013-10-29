import entityx


class UpdateTest(entityx.Entity):
    updated = False

    def update(self, dt, frame):
        self.updated = True
