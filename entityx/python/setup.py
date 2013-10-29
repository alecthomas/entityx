try:
    from distribute import setup
except ImportError:
    from setuptools import setup

setup(
    name='entityx',
    version='0.0.1',
    packages=['entityx'],
    license='MIT',
    description='EntityX Entity-Component Framework Python bindings',
    )
