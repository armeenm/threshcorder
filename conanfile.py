from conans import ConanFile, tools, Meson
import os

class ConanFileToolsTest(ConanFile):
    name = 'threshcorder'
    version = '0.0.1'
    generators = 'pkg_config'
    requires = ['fmt/7.1.3', 'libalsa/1.2.4', 'magic_enum/0.7.2']
    settings = 'os', 'compiler', 'build_type'
