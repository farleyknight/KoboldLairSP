import os

from conans import ConanFile, CMake, tools

VERSION = "0.2"

class KoboldLairSPTestConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "cmake", "cmake_find_package", "cmake_paths"
    requires = f"KoboldLairSP/{VERSION}", "fmt/8.0.1", "gtest/cci.20210126"

    def build(self):
        cmake = CMake(self)
        # Current dir is "test_package/build/<build_id>" and CMakeLists.txt is
        # in "test_package"
        cmake.configure()
        cmake.build()

    def imports(self):
        self.copy("*.dll", dst="bin", src="bin")
        self.copy("*.dylib*", dst="bin", src="lib")
        self.copy('*.so*', dst='bin', src='lib')

    def test(self):
        if not tools.cross_building(self):
            os.chdir("bin")
            self.run(".%sexample" % os.sep)

    def package_info(self):
        self.cpp_info.libs = [f"KoboldLairSP/{VERSION}"]

    def validate(self):
        tools.check_min_cppstd(self, "17")

    def package_info(self):
        self.cpp_info.cxxflags = ["-std=c++17"]
