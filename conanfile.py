from conans import ConanFile, tools, Meson
import os

class ConanFileToolsTest(ConanFile):
    name = 'threshcorder'
    version = '0.0.1'
    generators = 'pkg_config'
    requires = ['fmt/7.1.3', 'libalsa/1.2.4']
    settings = 'os', 'compiler', 'build_type'
