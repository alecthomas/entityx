import _entityx


"""These classes provide a convenience layer on top of the raw entityx::python
primitives.

They allow you to declare your entities and components in an intuitive way:

    class Player(Entity):
        position = Component(Position)
        direction = Component(Direction)
        sprite = Component(Sprite, 'player.png')  # Sprite component with constructor argument

        def update(self, dt, frame):
            self.position.x += self.direction.x * dt
            self.position.x += self.direction.y * dt

Note that components assigned from C++ must be assigned prior to assigning
PythonComponent, otherwise they will be created by the Entity constructor.
"""


__all__ = ['Entity', 'Component']


class Component(object):
    """A field that manages Component creation/retrieval.

    Use like so:

    class Player(Entity):
        position = Component(Position)

        def move_to(self, x, y):
            self.position.x = x
            self.position.y = y
    """
    def __init__(self, cls, *args, **kwargs):
        self._cls = cls
        self._args = args
        self._kwargs = kwargs

    def _build(self, entity_id):
        component = self._cls.get_component(_entityx._entity_manager, entity_id)
        if not component:
            component = self._cls(*self._args, **self._kwargs)
            component.assign_to(_entityx._entity_manager, entity_id)
        return component


class EntityMetaClass(_entityx.Entity.__class__):
    """Collect registered components from class attributes.

    This is done at class creation time to reduce entity creation overhead.
    """

    def __new__(cls, name, bases, dct):
        dct['_components'] = components = {}
        # Collect components from base classes
        for base in bases:
            if '_components' in base.__dict__:
                components.update(base.__dict__['_components'])
        # Collect components
        for key, value in dct.items():
            if isinstance(value, Component):
                components[key] = value
        return type.__new__(cls, name, bases, dct)


class Entity(_entityx.Entity):
    """Base Entity class.

    Python Enitities differ in semantics from C++ components, in that they
    contain logic, receive events, and so on.
    """

    __metaclass__ = EntityMetaClass

    def __new__(cls, *args, **kwargs):
        entity_id = kwargs.pop('entity_id', None)
        self = _entityx.Entity.__new__(cls)
        if entity_id is None:
            entity_id = _entityx._entity_manager.configure(self)
        _entityx.Entity.__init__(self, _entityx._entity_manager, entity_id)
        for k, v in self._components.items():
            setattr(self, k, v._build(self._entity_id))
        return self

    def __init__(self):
        """Default constructor."""

    def __repr__(self):
        return '<%s.%s %d.%d>' % (self.__class__.__module__, self.__class__.__name__, self._entity_id.index, self._entity_id.version)

    @classmethod
    def _from_raw_entity(cls, entity_id, *args, **kwargs):
        """Create a new Entity from a raw entity.

        This is called from C++.
        """
        self = Entity.__new__(cls, entity_id=entity_id)
        cls.__init__(self, *args, **kwargs)
        return self


def emit(event):
    """Emit an event.

    :param event: A Python-exposed C++ subclass of entityx::BaseEvent.
    """
    return _entityx._event_manager.emit(event)
