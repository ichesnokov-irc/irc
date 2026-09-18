from conan import ConanFile
from conan.tools.files import get, copy
import os

class IrcConan(ConanFile):
    name = "irc"
    description = "A portable header-only C++20 Integer Range Containers library"
    license = "MIT"
    homepage = "https://github.com/ichesnokov-irc/irc"
    url = "https://github.com/ichesnokov-irc/irc"
    topics = ("irc", "portable", "C++20", "header-only")
    
    no_copy_source = True

    def layout(self):
        # Keeps the sources in the root directory after extracting the archive
        self.folders.source = "."

    def source(self):
        # Downloads your source archive from GitHub based on the version
        get(self, **self.conan_data["sources"][self.version], strip_root=True)

    def package(self):
        # Copy everything from your repository's 'include' folder to the package 'include' folder
        copy(self, "*", 
             src=os.path.join(self.source_folder, "include"), 
             dst=os.path.join(self.package_folder, "include"))
        
        # ConanCenter strictly requires copying the license file
        copy(self, "LICENSE*", 
             src=self.source_folder, 
             dst=os.path.join(self.package_folder, "licenses"))

    def package_info(self):
        # Inform Conan that there are no compiled libraries or binaries
        self.cpp_info.bindirs = []
        self.cpp_info.libdirs = []
        # Defines the modern CMake target name for users: irc::irc
        self.cpp_info.set_property("cmake_target_name", "irc::irc")
