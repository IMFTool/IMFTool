#!/usr/bin/env python
# -*- coding: utf-8 -*-

import json, os
from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain
from conan.errors import ConanInvalidConfiguration
from conan.tools.env import VirtualBuildEnv

required_conan_version = ">=2.0"

class ImfToolConan(ConanFile):
    # ---Package reference---
    name = "imftool"
    version = "1.9.0"
    # ---Metadata---
    description = "A tool for editing IMF CPLs and creating new versions of an existing IMF package"
    license = "GPLv3"
    author = "HSRM"
    topics = ["IMF"]
    homepage = "https://github.com/IMFTool"
    url = "https://github.com/IMFTool/IMFTool"
    # ---Requirements---
    requires = ("qt/[>=6.5.0]@de.privatehive/stable", "asdcplib/[>=2.12.1]", "regxmllib/[>=1.1.5]@de.privatehive/stable", "xerces-c/[>=3.2.5]@de.privatehive/stable")
    tool_requires = ["cmake/3.21.7", "ninja/[>=1.11.1]"]
    # ---Sources---
    exports = ["LICENSE"]
    exports_sources = ["LICENSE", "*"]
    # ---Binary model---
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False], "fPIC": [True, False] }
    default_options = {"shared": True,
                       "fPIC": True,
                       "qt/*:GUI": True,
                       "qt/*:opengl": "desktop",
                       "qt/*:qtbase": True,
                       "qt/*:widgets": True,
                       "qt/*:qtsvg": True}
    # ---Build---
    generators = []
    # ---Folders---
    no_copy_source = False

    def validate(self):
        valid_os = ["Windows", "Linux", "Macos"]
        if str(self.settings.os) not in valid_os:
            raise ConanInvalidConfiguration(f"{self.name} {self.version} is only supported for the following operating systems: {valid_os}")
        valid_arch = ["x86_64", "armv8"]
        if str(self.settings.arch) not in valid_arch:
            raise ConanInvalidConfiguration(f"{self.name} {self.version} is only supported for the following architectures on {self.settings.os}: {valid_arch}") 

    def generate(self):
        VirtualBuildEnv(self).generate()
        tc = CMakeToolchain(self, generator="Ninja")
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()
