# ReflectionBox

Your sandbox for C++26 static reflection. Clone, run one script, see output — no local LLVM install needed.

`./reflect <file.cc>` pulls a pre-built image from Docker Hub (`sagardesd/reflectionbox:latest`),
compiles your file inside a temporary container, and streams the output back to your terminal.
The image contains [Bloomberg's `clang-p2996` fork](https://github.com/bloomberg/clang-p2996) —
the only experimental compiler that supports P2996 reflection today.

> **Heads up:** This compiler is experimental — Bloomberg's own words are
> *"DO NOT use for production"*. Expect occasional crashes on complex programs.

---

## Prerequisites

Only Docker is required. VS Code is optional.

| Tool | Required for |
|---|---|
| [Docker Desktop](https://www.docker.com/products/docker-desktop/) (or Docker Engine on Linux) | `./reflect` and the devcontainer |
| [VS Code](https://code.visualstudio.com/) + [Dev Containers extension](https://marketplace.visualstudio.com/items?itemName=ms-vscode-remote.remote-containers) | Interactive editor inside the container *(optional)* |

---

## Quick start

```bash
git clone https://github.com/sagardesd/ReflectionBox
cd ReflectionBox
./reflect examples/reflection_demo.cc
```

That's it — one command to compile and run. Here's what happens under the hood:

1. **First run** — pulls `sagardesd/reflectionbox:latest` from Docker Hub (~3 GB, takes 1-2 min depending on your connection).
2. **Every subsequent run** — the image is already cached locally, so it starts instantly.
3. A temporary container spins up, compiles your file with `-std=c++26 -freflection -stdlib=libc++`, runs the binary, streams output, and exits. Nothing is left running.

> **Want to build the image from source instead?** Pass `--build`:
> ```bash
> ./reflect --build examples/reflection_demo.cc
> ```
> This compiles LLVM/Clang from source inside Docker — expect **60-90 minutes** on first build.
> Only needed if you want to modify the compiler or can't use the Docker Hub image.

---

## Hello, reflection

Create any `.cc` file inside the repo:

```cpp
// hello.cc
#include <meta>
#include <print>

struct Point { double x, y, z; };
enum class Color { Red, Green, Blue };

int main() {
    std::println("Fields of Point:");
    template for (constexpr auto m : std::meta::nonstatic_data_members_of(^Point)) {
        std::println("  {}", std::meta::identifier_of(m));
    }

    std::println("Colors:");
    template for (constexpr auto e : std::meta::enumerators_of(^Color)) {
        std::println("  {}", std::meta::identifier_of(e));
    }
}
```

Run it from your host:

```bash
./reflect hello.cc
```

```
[reflect] hello.cc

━━━  cpprun  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
  source : hello.cc
  flags  : -std=c++26 -freflection -stdlib=libc++
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

[1/2] Compiling…
✓ compiled (2s)

[2/2] Running…
━━━  output  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

Fields of Point:
  x
  y
  z
Colors:
  Red
  Green
  Blue

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
✓ exited 0
```

---

## Ways to compile and run

### 1. `./reflect` — from your host (recommended)

```bash
# Compile and run in one command (pulls image from Docker Hub on first use)
./reflect my_demo.cc

# Extra compiler flags
./reflect my_demo.cc -O2
./reflect my_demo.cc -freflection-latest
./reflect my_demo.cc -g -O0 -fsanitize=address,undefined

# Pass arguments to the compiled binary
./reflect my_demo.cc -- --input data.txt

# Build the image from source instead of pulling from Docker Hub
# ⚠️  Takes 60-90 min on first build (compiles LLVM from source)
./reflect --build my_demo.cc
```

How it works:
- **Without `--build` (default):** pulls `sagardesd/reflectionbox:latest` from Docker Hub on first use (~3 GB, cached after that), spins up a temporary container, runs `cpprun` inside it, and exits.
- **With `--build`:** builds the image locally from the `Dockerfile` in `.devcontainer/`. Use this only if you need to modify the compiler setup or can't access Docker Hub.

### 2. `cpprun` inside the container (VS Code terminal)

Open the folder in VS Code and click **Reopen in Container** when prompted.
Then in the integrated terminal:

```bash
cpprun my_demo.cc
cpprun my_demo.cc -O2
cpprun my_demo.cc -freflection-latest
cpprun my_demo.cc -- --input data.txt
```

### Help Menu Also avaialble so try that
```./reflect --help                                            
Usage:  ./reflect <file.cc> [compiler-flags...] [-- binary-args...]

Examples:
  ./reflect examples/reflection_demo.cc
  ./reflect my_demo.cc -O2
  ./reflect my_demo.cc -freflection-latest
  ./reflect my_demo.cc -g -fsanitize=address,undefined
  ./reflect my_demo.cc -- --my-arg value

Image options (mutually exclusive):
  (default)   Pull sagardesd/reflectionbox:latest from Docker Hub (~1-2 min first time)
  --build     Build the image from source locally (~60-90 min, compiles LLVM)

Other:
  --help      Show this message
```

### 3. VS Code task — `Ctrl+Shift+B`

Open any `.cc` file and press **`Ctrl+Shift+B`** to compile and run it.
Compiler errors appear as clickable squiggles in the editor.

More variants via `Ctrl+Shift+P` → **Run Task**:

| Task | Extra flags |
|---|---|
| reflect: compile & run | *(default)* |
| reflect: compile & run -O2 | `-O2` |
| reflect: compile & run (ASan + UBSan) | `-g -O0 -fsanitize=address,undefined` |
| reflect: compile & run (-freflection-latest) | `-freflection-latest` |

---

## Compiler flags

Applied automatically by `cpprun` and `./reflect`:

| Flag | Purpose |
|---|---|
| `-std=c++26` | Required for P2996 syntax |
| `-freflection` | Core reflection — `^T`, `[:e:]`, `std::meta::` |
| `-stdlib=libc++` | Required — system libstdc++ doesn't have `<meta>` |

Optional experimental extensions:

| Flag | Proposal | What it adds |
|---|---|---|
| `-fparameter-reflection` | P3096 | Reflect function parameters |
| `-fexpansion-statements` | P1306 | `template for` over ranges |
| `-fattribute-reflection` | P3385 | Reflect `[[attributes]]` |
| `-freflection-latest` | all of the above | Shorthand for everything |

---

## What's in the repo

```
ReflectionBox/
├── reflect                     # ⭐ Entry-point: pulls Docker image + compiles + runs your file
├── .devcontainer/
│   ├── Dockerfile              # 2-stage build: LLVM from source → slim dev image
│   └── devcontainer.json       # VS Code devcontainer config
├── .github/workflows/
│   └── publish.yml             # CI: builds & pushes sagardesd/reflectionbox:latest to Docker Hub
├── .vscode/
│   └── tasks.json              # Ctrl+Shift+B → cpprun on the open file
├── scripts/
│   ├── cpprun                  # Compile-and-run helper (on $PATH inside the container)
│   └── gen_compile_commands    # Regenerates compile_commands.json for clangd intellisense
├── examples/
│   ├── reflection_demo.cc      # Member names, enum names, generic struct printer
│   ├── part_1_demo.cpp         # Struct introspection with define_static_array + template for
│   ├── json_deserializer.cc    # Reflection-powered JSON → struct deserializer
│   └── json26_example.cc       # Header-only JSON serializer/deserializer round-trip
├── json26.hpp                  # Reflection-powered header-only JSON library
└── tags                        # ctags file for editor navigation
```

---

## Updating clangd intellisense

After adding new `.cc` files, regenerate the clangd index inside the container:

```bash
gen_compile_commands
```

---

## Build time and resources

Most users never need to build — `./reflect` pulls the pre-built image from Docker Hub in 1-2 minutes.

The table below only applies if you pass `--build` (which compiles LLVM from source locally):

| Resource | Requirement |
|---|---|
| Disk during build | ~20 GB |
| Final image size | ~3 GB |
| RAM | 8 GB min, 16 GB recommended |
| Build time | ~30 min on 16 cores, ~90 min on 4 cores |

---

## Resources

- [P2996 paper](https://wg21.link/p2996) — the full reflection proposal
- [Bloomberg clang-p2996](https://github.com/bloomberg/clang-p2996) — the compiler this container builds
- [Implementation status](https://github.com/bloomberg/clang-p2996/blob/p2996/P2996.md) — what's supported, known bugs
- [Compiler Explorer](https://godbolt.org) — online P2996 playground (select `clang (experimental P2996)`)
