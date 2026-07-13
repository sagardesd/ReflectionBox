# Stage 1 — Build Bloomberg's clang-p2996 from source
FROM ubuntu:24.04 AS builder

ARG JOBS=0

ENV DEBIAN_FRONTEND=noninteractive \
    TZ=UTC

RUN apt-get update && apt-get install -y --no-install-recommends \
        git cmake ninja-build python3 pkg-config \
        gcc g++ \
        zlib1g-dev libzstd-dev libxml2-dev \
        ca-certificates curl \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
RUN git clone \
        --depth=1 --branch p2996 --single-branch \
        https://github.com/bloomberg/clang-p2996.git \
        llvm-p2996

RUN cmake -G Ninja \
        -S /src/llvm-p2996/llvm \
        -B /build \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/opt/clang-p2996 \
        -DLLVM_ENABLE_PROJECTS="clang;lld" \
        -DLLVM_ENABLE_RUNTIMES="libcxx;libcxxabi;libunwind" \
        -DLLVM_TARGETS_TO_BUILD=X86 \
        -DLLVM_BUILD_TESTS=OFF    -DLLVM_INCLUDE_TESTS=OFF \
        -DLLVM_BUILD_EXAMPLES=OFF -DLLVM_INCLUDE_EXAMPLES=OFF \
        -DLLVM_BUILD_BENCHMARKS=OFF -DLLVM_INCLUDE_BENCHMARKS=OFF \
        -DLLVM_BUILD_DOCS=OFF     -DLLVM_INCLUDE_DOCS=OFF \
        -DLLVM_ENABLE_DOXYGEN=OFF -DLLVM_ENABLE_SPHINX=OFF \
        -DCLANG_INCLUDE_TESTS=OFF -DCLANG_BUILD_EXAMPLES=OFF \
        -DCLANG_INCLUDE_DOCS=OFF \
        -DLIBCXX_INCLUDE_TESTS=OFF -DLIBCXX_INCLUDE_BENCHMARKS=OFF \
        -DLIBCXXABI_INCLUDE_TESTS=OFF -DLIBUNWIND_INCLUDE_TESTS=OFF \
        -DLLVM_USE_LINKER=gold

RUN NJOBS=$([ "${JOBS}" -gt 0 ] 2>/dev/null && echo "${JOBS}" || nproc) && \
    cmake --build /build --target install --parallel "${NJOBS}"

# cmake install may not create the clang++ symlink on all configurations
RUN [ -e /opt/clang-p2996/bin/clang++ ] || \
    ln -sf clang /opt/clang-p2996/bin/clang++

RUN CLANG_VER=$(ls /opt/clang-p2996/lib/clang/ | sort -V | tail -1) && \
    ln -s "/opt/clang-p2996/lib/clang/${CLANG_VER}" /opt/clang-p2996/lib/clang/current

RUN echo "=== Searching for libc++.so.1 ===" && \
    find /opt/clang-p2996/lib -name 'libc++.so*' -ls && \
    echo "=== Full lib directory structure ===" && \
    ls -la /opt/clang-p2996/lib/ && \
    ls -la /opt/clang-p2996/lib/x86_64-unknown-linux-gnu/ 2>/dev/null || true && \
    LIBCXX_DIR=$(find /opt/clang-p2996/lib -name 'libc++.so.1' -printf '%h' -quit) && \
    echo "=== Using LIBCXX_DIR: ${LIBCXX_DIR} ===" && \
    echo 'int main(){}' | \
    /opt/clang-p2996/bin/clang++ -std=c++26 -freflection -stdlib=libc++ \
    -x c++ - -o /tmp/smoke && \
    LD_LIBRARY_PATH="/opt/clang-p2996/lib:${LIBCXX_DIR}" /tmp/smoke && echo "Smoke test PASSED"


# Stage 2 — Developer image
FROM ubuntu:24.04 AS dev

ENV DEBIAN_FRONTEND=noninteractive \
    TZ=UTC \
    PATH="/opt/clang-p2996/bin:/usr/local/bin:${PATH}" \
    LD_LIBRARY_PATH="/opt/clang-p2996/lib:/opt/clang-p2996/lib/x86_64-unknown-linux-gnu" \
    CC="/opt/clang-p2996/bin/clang" \
    CXX="/opt/clang-p2996/bin/clang++"

RUN apt-get update && apt-get install -y --no-install-recommends \
        binutils gdb git curl ca-certificates python3 vim less \
        libc6-dev libgcc-s1 gcc \
    && rm -rf /var/lib/apt/lists/*

COPY --from=builder /opt/clang-p2996 /opt/clang-p2996

# Register clang-p2996 libraries (including arch-specific subdirectory) with the linker
RUN echo "=== [dev] Searching for libc++.so.1 ===" && \
    find /opt/clang-p2996/lib -name 'libc++.so*' -ls && \
    echo "/opt/clang-p2996/lib" > /etc/ld.so.conf.d/clang-p2996.conf && \
    find /opt/clang-p2996/lib -name 'libc++.so.1' -printf '%h\n' -quit >> /etc/ld.so.conf.d/clang-p2996.conf && \
    echo "=== [dev] ldconfig paths ===" && \
    cat /etc/ld.so.conf.d/clang-p2996.conf && \
    ldconfig && \
    echo "=== [dev] ldconfig cache for libc++ ===" && \
    ldconfig -p | grep libc++

COPY scripts/cpprun               /usr/local/bin/cpprun
COPY scripts/gen_compile_commands /usr/local/bin/gen_compile_commands
RUN chmod +x /usr/local/bin/cpprun /usr/local/bin/gen_compile_commands

RUN userdel -r ubuntu 2>/dev/null || true && \
    groupdel ubuntu 2>/dev/null || true && \
    groupadd --gid 1000 dev && \
    useradd  --uid 1000 --gid dev --shell /bin/bash --create-home dev

USER dev
WORKDIR /workspace

RUN echo 'alias run="cpprun"' >> ~/.bashrc && \
    echo 'export PS1="\[\033[1;36m\][p2996]\[\033[0m\] \w \$ "' >> ~/.bashrc

RUN echo 'int main(){}' | \
    clang++ -std=c++26 -freflection -stdlib=libc++ -x c++ - -o /tmp/smoke2 && \
    /tmp/smoke2 && echo "Smoke test PASSED"

CMD ["/bin/bash"]
