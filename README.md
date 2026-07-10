# ReflectionBox

A ready-to-use devcontainer for experimenting with **C++26 static reflection (P2996)**.

Clone it, open in VS Code, and you get a full compiler environment with a
one-command workflow to write, compile, and run any reflection-heavy C++ file —
no local LLVM installation needed.

It builds [Bloomberg's `clang-p2996` fork](https://github.com/bloomberg/clang-p2996)
from source inside Docker and wires it to a Bazel toolchain and a `cpprun`
helper script.

> **Heads up:** This compiler is experimental — Bloomberg's own words are
> *"DO NOT use for production"*. Expect occasional crashes on complex programs.

---

## Prerequisites

| Tool | Purpose |
|---|---|
| [Docker Desktop](https://www.docker.com/products/docker-desktop/) (or Docker Engine) | Runs the container |
| [VS Code](https://code.visualstudio.com/) | Editor |
| [Dev Containers extension](https://marketplace.visualstudio.com/items?itemName=ms-vscode-remote.remote-containers) | Opens the folder inside the container |

---

## Quick start

```bash
git clone https://github.com/<you>/ReflectionBox
cd ReflectionBox
code .
```

VS Code will show a popup — click **Reopen in Container**.

> The **first build takes 60–90 minutes** (it compiles LLVM from source).
> Every start after that is instant — Docker caches the compiler layer.

---

## Hello, reflection

Once inside the container, create a file anywhere in the workspace:

```cpp
// hello.cc
#include <meta>
#include <print>

struct Point { double x, y, z; };

enum class Color { Red, Green, Blue };

int main() {
    // Iterate over struct members at compile time
    std::println("Fields of Point:");
    template for (constexpr auto m : std::meta::nonstatic_data_members_of(^Point)) {
        std::println("  {}", std::meta::identifier_of(m));
    }

    // Enumerate an enum's values at compile time
    std::println("Colors:");
    template for (constexpr auto e : std::meta::enumerators_of(^Color)) {
        std::println("  {}", std::meta::identifier_of(e));
    }
}
```

Compile and run it in one command:

```bash
cpprun hello.cc
```

```
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

## Three ways to compile and run

### 1. `cpprun` in the terminal

```bash
# Basic
cpprun my_demo.cc

# Optimised
cpprun my_demo.cc -O2

# Debug + AddressSanitizer + UBSan
cpprun my_demo.cc -g -O0 -fsanitize=address,undefined

# All experimental reflection extensions enabled
cpprun my_demo.cc -freflection-latest

# Pass arguments to the compiled binary
cpprun my_demo.cc -- --input data.txt
```

`cpprun` compiles with `-std=c++26 -freflection -stdlib=libc++` by default.
The binary is placed in `/tmp` so the workspace stays clean.

### 2. VS Code task — `Ctrl+Shift+B`

Open any `.cc` file and press **`Ctrl+Shift+B`** to compile and run it.
Compiler errors appear as clickable squiggles in the editor.

More variants in the task picker (`Ctrl+Shift+P` → **Run Task**):

| Task | Extra flags |
|---|---|
| compile & run | *(default)* |
| compile & run optimised | `-O2` |
| compile & run debug + sanitisers | `-g -O0 -fsanitize=address,undefined` |
| compile & run reflection-latest | `-freflection-latest` |

### 3. Bazel

```bash
# Run the included examples
bazel run //:reflection_demo
bazel run //:json26_example
bazel test //...
```

Add your own file to `BUILD.bazel`:

```python
cc_binary(
    name = "my_demo",
    srcs = ["my_demo.cc"],
)
```

Then `bazel run //:my_demo`.

---

## Compiler flags

These are applied automatically by `cpprun` and the Bazel toolchain:

| Flag | Needed for |
|---|---|
| `-std=c++26` | P2996 syntax |
| `-freflection` | Core reflection (`^T`, `[:e:]`, `std::meta::`) |
| `-stdlib=libc++` | `<meta>` header (system libstdc++ doesn't have it) |

Optional experimental extensions (pass via `cpprun` or `copts` in Bazel):

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
├── .devcontainer/
│   ├── Dockerfile              # 2-stage build: LLVM from source → slim dev image
│   └── devcontainer.json       # VS Code devcontainer config
├── .vscode/
│   └── tasks.json              # Ctrl+Shift+B → cpprun on the open file
├── scripts/
│   ├── cpprun                  # Compile-and-run helper (on $PATH inside the container)
│   └── gen_compile_commands    # Regenerate compile_commands.json for clangd
├── toolchain/
│   ├── BUILD.bazel             # Bazel cc_toolchain registration
│   └── cc_toolchain_config.bzl # Starlark toolchain config
├── examples/
│   ├── reflection_demo.cc      # P2996 basics: member names, enum names, generic struct print
│   ├── json26_example.cc       # Header-only JSON serializer/deserializer using reflection
│   └── json26_test.cc          # Test suite for the JSON library
├── json26.hpp                  # Reflection-powered header-only JSON library
├── MODULE.bazel                # Bzlmod workspace
├── BUILD.bazel                 # Bazel targets
└── .bazelrc                    # Bazel defaults
```

---

## Updating clangd intellisense

After adding new `.cc` files, regenerate the clangd index:

```bash
gen_compile_commands
```

This writes `compile_commands.json` at the workspace root so clangd
autocompletes `std::meta::` correctly and understands `^T` / `[:e:]` syntax.

---

## Using without VS Code

```bash
# Build the image
docker build \
  --target dev \
  --build-arg JOBS=$(nproc) \
  -t reflectionbox \
  -f .devcontainer/Dockerfile \
  .

# Mount your code and get a shell
docker run --rm -it \
  -v "$(pwd)":/workspace \
  reflectionbox

# Then inside the container:
cpprun examples/reflection_demo.cc
```

---

## Build time and resource requirements

| Resource | Requirement |
|---|---|
| Disk during build | ~20 GB |
| Final image size | ~3 GB |
| RAM | 8 GB min, 16 GB recommended |
| Build time | ~30 min on 16 cores, ~90 min on 4 cores |

The LLVM builder stage is cached after the first build. Rebuilding only the
dev stage (e.g. changing `cpprun`) reuses the cached LLVM layer and takes
seconds.

---

## Resources

- [P2996 paper](https://wg21.link/p2996) — the full reflection proposal
- [Bloomberg clang-p2996](https://github.com/bloomberg/clang-p2996) — the compiler this container builds
- [Implementation status](https://github.com/bloomberg/clang-p2996/blob/p2996/P2996.md) — what's implemented, known bugs
- [Compiler Explorer](https://godbolt.org) — online P2996 playground (no install needed for quick tests, select `clang (experimental P2996)`)
