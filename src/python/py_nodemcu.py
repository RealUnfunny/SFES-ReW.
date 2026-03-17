# This file is for automatically exporting a respective compile_commands.json file to the ./src/* directory.

import os

from SCons.Defaults import DefaultEnvironment

env = DefaultEnvironment()

env.Replace(
    COMPILATIONDB_PATH=os.path.join(
        str(env.subst("$SRC_DIR")), "nodemcu", "compile_commands.json"
    )
)
