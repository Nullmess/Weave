def generate(env):
    py_name = "python3"
    if env["PLATFORM"] == "win32":
        # Windows : Python 3 nommé python
        py_name = "python"

    env.SetDefault(
        PYTHON3=py_name,
    )


def exists(env):
    return True
