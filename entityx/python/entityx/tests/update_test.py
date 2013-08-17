import entityx


class UpdateTest(entityx.Entity):
    updated = False

    def update(self, dt):
        self.updated = True
