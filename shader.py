"""
Compiles a vertex and fragment shader
and returns them as C strings.
"""
import re

from pyshaders import from_files_names, ShaderCompilationError
from colorama import init, Fore


def remove_comments(text: str) -> str:
    pattern = r"(\".*?\"|\'.*?\')|(/\*.*?\*/|//[^\r\n]*$)"
    regex = re.compile(pattern, re.DOTALL | re.MULTILINE)
    def replacer(match: re.Match):
        return match.group(1) if not match.group(2) else ""
    return regex.sub(replacer, text)

def convert(text: str):
    text = remove_comments(text)
    output = '"'
    for line in text.split("\n"):
        output += f"{line.strip()}\\n"
    output += '"'
    return output


def make_header():
    with open("src/shader.h", "w") as h:
        with open("shaders/main.vert", "r") as v:
            h.write(f"static const char *vertex_shader_src = {convert(v.read())};\n")
        with open("shaders/main.frag", "r") as f:
            h.write(f"static const char *fragment_shader_src= {convert(f.read())};")


def main():
    init(autoreset=True)
    print("Compiling shaders...")
    print("-" * 20)
    try:
        from_files_names("shaders/main.vert", "shaders/main.frag")
        print(Fore.LIGHTGREEN_EX + "SHADERS COMPILED SUCCESSFULLY")
        make_header()
        print("Generated shaders.h successfully!")
        print("-" * 20)
    except ShaderCompilationError as e:
        print(Fore.LIGHTRED_EX + "ERROR COMPILING SHADERS")
        print(e.logs)
        print("-" * 20)
        input("Press Enter to continue...")
        exit(2)


if __name__ == "__main__":
    main()
