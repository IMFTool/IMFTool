#!/usr/bin/env python
# -*- coding: utf-8 -*-

import json
from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain, CMakeDeps
from conan.errors import ConanInvalidConfiguration
from conan.tools.env import VirtualBuildEnv
from conan.tools.files import copy
import os

required_conan_version = ">=2.0"

class ImfToolConan(ConanFile):
    jsonInfo = json.load(open("info.json", 'r'))
    # ---Package reference---
    name = jsonInfo["projectName"].lower()
    version = "%u.%u.%u" % (jsonInfo["version"]["major"], jsonInfo["version"]["minor"], jsonInfo["version"]["patch"])
    user = jsonInfo["domain"]
    channel = "%s" % ("snapshot" if jsonInfo["version"]["snapshot"] else "stable")
    # ---Metadata---
    description = jsonInfo["projectDescription"]
    license = jsonInfo["license"]
    author = jsonInfo["vendor"]
    topics = jsonInfo["topics"]
    homepage = jsonInfo["homepage"]
    url = jsonInfo["repository"]
    # ---Requirements---
    requires = ("qt/6.8.2@de.privatehive/stable", "qtappbase/1.9.0@de.privatehive/stable", "regxmllib/1.1.5@imftool/stable", "asdcplib/2.13.1@imftool/stable", "xerces-c/3.2.5", "openjpeg/2.5.2", "zlib/1.3.1")
    # cmake 3.23 is needed if we use XCode generator
    tool_requires = ["cmake/3.23.5", "ninja/1.11.1"]
    # ---Sources---
    exports = ["info.json", "LICENSE"]
    exports_sources = ["info.json", "LICENSE", "regxmllib/*", "photon/*", "files/*", "src/*", "resources/*", "CMakeLists.txt"]
    # ---Binary model---
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False], "fPIC": [True, False], "app5Support": [True, False], "xcode": [True, False], "bundleJVM": [True, False]}
    default_options = {"shared": True,
                       "fPIC": True,
                       "app5Support": True,
                       "xcode": False,
                       "bundleJVM": True,
                       "qt/*:GUI": True,
                       "qt/*:opengl": "desktop",
                       "qt/*:widgetsstyle": "stylesheet",
                       "qt/*:qtbase": True,
                       "qt/*:widgets": True,
                       "qt/*:qtsvg": True,
                       "regxmllib/*:shared": False,
                       "asdcplib/*:shared": False,
                       "asdcplib/*:encryption_support": True,
                       "xerces-c/*:shared": False,
                       "xerces-c/*:network": False,
                       "openjpeg/*:shared": False,
                       "zlib/*:shared": False}
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

    def requirements(self):
        if self.options.app5Support:
            self.requires("imath/3.1.9", options={"shared": True})
            self.requires("openexr/3.3.1", options={"shared": True})
        if self.options.bundleJVM:
            self.requires("openjdk/8.0.442@de.privatehive/stable")

    def config_options(self):
        if self.settings.os != "Macos":
            self.options.rm_safe("Xcode")

    def generate(self):
        VirtualBuildEnv(self).generate()
        CMakeDeps(self).generate()
        tc = CMakeToolchain(self, generator="Xcode" if self.options.xcode and self.settings.os == 'Macos' else "Ninja")
        tc.variables["BUILD_APP5_SUPPORT"] = self.options.app5Support
        if self.options.bundleJVM:
            openjdk = self.dependencies["openjdk"]
            tc.variables["JVM_PATH"] = os.path.join(openjdk.package_folder, "jre").replace("\\", "/")
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        if self.settings.build_type == 'Release':
            cmake.install(cli_args=["--strip"])
        else:
            cmake.install()

    def deploy(self):
        copy(self, "%s-%s-*-installer*" % (self.name, self.version), src=self.package_folder, dst=self.deploy_folder)
        copy(self, "%s-%s-*.AppImage*" % (self.name, self.version), src=self.package_folder, dst=self.deploy_folder)
        copy(self, "%s-%s-*.dmg*" % (self.name, self.version), src=self.package_folder, dst=self.deploy_folder)
