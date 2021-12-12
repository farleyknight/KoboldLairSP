from conans import tools, ConanFile, CMake

class KoboldLairSPConan(ConanFile):
    name = "KoboldLairSP"
    version = "0.1"
    license = "MIT"
    author = "Farley Knight farleyknight@gmail.com"
    url = "http://github.com/farleyknight/KoboldLairSP"
    description = "SP = Slotted Page -- Store variable-length records in a data buffer."
    topics = ("c++", "slotted-page", "byte-buffer")
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False], "fPIC": [True, False]}
    default_options = {"shared": False, "fPIC": True}
    generators = "cmake"
    exports_sources = "src/*"
    requires = "MagicScrollBB/0.2.4"

    settings = {
        "os": None,
        "compiler": {"gcc": {"cppstd": ['17', '20']},
                     "apple-clang": {"cppstd": ['17', '20']},
                     "Visual Studio": {"cppstd": ['17', '20']}},
        "arch": None,
        "build_type": None
    }

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def build(self):
        cmake = CMake(self)
        cmake.configure(source_folder="src")
        cmake.build()

        # Explicit way:
        # self.run('cmake %s/hello %s'
        #          % (self.source_folder, cmake.command_line))
        # self.run("cmake --build . %s" % cmake.build_config)

    def package(self):
        self.copy("*.hpp", dst="include", src="src")
        self.copy("*.lib", dst="lib", keep_path=False)
        self.copy("*.dll", dst="bin", keep_path=False)
        self.copy("*.dylib*", dst="lib", keep_path=False)
        self.copy("*.so", dst="lib", keep_path=False)
        self.copy("*.a", dst="lib", keep_path=False)

    def validate(self):
        tools.check_min_cppstd(self, "17")

    def package_info(self):
        self.cpp_info.cxxflags = ["-std=c++17"]
