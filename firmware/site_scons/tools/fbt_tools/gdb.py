def generate(env):
    env.SetDefault(
        GDB="gdb",
        GDBPY="gdb-py3",
        GDBCOM="$GDB $GDBOPTS $SOURCES",  # Sans variable $TARGET
        GDBPYCOM="$GDBPY $GDBOPTS $GDBPYOPTS $SOURCES",  # Sans variable $TARGET
    )


def exists(env):
    return True
