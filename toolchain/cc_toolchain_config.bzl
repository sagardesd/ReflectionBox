"""Bazel C++ toolchain config for Bloomberg clang-p2996 (P2996 reflection)."""

load(
    "@bazel_tools//tools/cpp:cc_toolchain_config_lib.bzl",
    "action_config",
    "feature",
    "feature_set",
    "flag_group",
    "flag_set",
    "tool",
    "tool_path",
    "with_feature_set",
)
load("@bazel_tools//tools/build_defs/cc:action_names.bzl", "ACTION_NAMES")

def _impl(ctx):
    # ── Tool paths ────────────────────────────────────────────
    tool_paths_list = [
        tool_path(name = "gcc",      path = ctx.attr.compiler_path),
        tool_path(name = "g++",      path = ctx.attr.compiler_path),
        tool_path(name = "ar",       path = ctx.attr.ar_path),
        tool_path(name = "strip",    path = ctx.attr.strip_path),
        tool_path(name = "nm",       path = ctx.attr.nm_path),
        tool_path(name = "objcopy",  path = ctx.attr.objcopy_path),
        tool_path(name = "objdump",  path = ctx.attr.objdump_path),
        tool_path(name = "ld",       path = ctx.attr.ld_path),
        tool_path(name = "gcov",     path = "/bin/false"),
        tool_path(name = "llvm-cov", path = "/bin/false"),
    ]

    # ── Compile actions ───────────────────────────────────────
    compile_actions = [
        ACTION_NAMES.c_compile,
        ACTION_NAMES.cpp_compile,
        ACTION_NAMES.cpp_header_parsing,
        ACTION_NAMES.assemble,
        ACTION_NAMES.preprocess_assemble,
        ACTION_NAMES.cpp_module_compile,
        ACTION_NAMES.cpp_module_codegen,
    ]

    link_actions = [
        ACTION_NAMES.cpp_link_executable,
        ACTION_NAMES.cpp_link_dynamic_library,
        ACTION_NAMES.cpp_link_nodeps_dynamic_library,
    ]

    # ── Feature: default compile flags ───────────────────────
    default_compile_feature = feature(
        name = "default_compile_flags",
        enabled = True,
        flag_sets = [
            flag_set(
                actions = compile_actions,
                flag_groups = [
                    flag_group(
                        flags = ctx.attr.extra_compile_flags + [
                            ctx.attr.resource_dir_flag,
                        ] + [
                            "-isystem" + inc
                            for inc in ctx.attr.stdlib_includes
                        ],
                    ),
                ],
            ),
        ],
    )

    # ── Feature: default link flags ───────────────────────────
    default_link_feature = feature(
        name = "default_link_flags",
        enabled = True,
        flag_sets = [
            flag_set(
                actions = link_actions,
                flag_groups = [
                    flag_group(
                        flags = ctx.attr.extra_link_flags + [
                            "-L" + d
                            for d in ctx.attr.stdlib_lib_dirs
                        ],
                    ),
                ],
            ),
        ],
    )

    # ── Feature: opt ─────────────────────────────────────────
    opt_feature = feature(
        name = "opt",
        flag_sets = [
            flag_set(
                actions = compile_actions,
                flag_groups = [flag_group(flags = ["-O2", "-DNDEBUG"])],
            ),
        ],
    )

    # ── Feature: dbg ─────────────────────────────────────────
    dbg_feature = feature(
        name = "dbg",
        flag_sets = [
            flag_set(
                actions = compile_actions,
                flag_groups = [flag_group(flags = ["-g", "-O0"])],
            ),
        ],
    )

    # ── Feature: fastbuild ────────────────────────────────────
    fastbuild_feature = feature(
        name = "fastbuild",
        flag_sets = [
            flag_set(
                actions = compile_actions,
                flag_groups = [flag_group(flags = ["-O1"])],
            ),
        ],
    )

    # ── Feature: warnings ────────────────────────────────────
    warnings_feature = feature(
        name = "warnings",
        enabled = True,
        flag_sets = [
            flag_set(
                actions = compile_actions,
                flag_groups = [
                    flag_group(flags = ["-Wall", "-Wextra", "-Wpedantic"]),
                ],
            ),
        ],
    )

    # ── Feature: strip_debug_symbols ─────────────────────────
    strip_debug_feature = feature(
        name = "strip_debug_symbols",
        flag_sets = [
            flag_set(
                actions = link_actions,
                flag_groups = [flag_group(flags = ["-Wl,-S"])],
            ),
        ],
    )

    # ── Feature: supports_start_end_lib (lld) ─────────────────
    supports_start_end_lib = feature(
        name = "supports_start_end_lib",
        enabled = True,
    )

    # ── Feature: supports_pic ─────────────────────────────────
    supports_pic = feature(
        name = "supports_pic",
        enabled = True,
        flag_sets = [
            flag_set(
                actions = compile_actions,
                flag_groups = [
                    flag_group(
                        flags = ["-fPIC"],
                        expand_if_available = "pic",
                    ),
                ],
            ),
        ],
    )

    # ── Feature: user_compile_flags ───────────────────────────
    user_compile_flags_feature = feature(
        name = "user_compile_flags",
        enabled = True,
        flag_sets = [
            flag_set(
                actions = compile_actions,
                flag_groups = [
                    flag_group(
                        flags = ["%{user_compile_flags}"],
                        iterate_over = "user_compile_flags",
                        expand_if_available = "user_compile_flags",
                    ),
                ],
            ),
        ],
    )

    # ── Feature: include_paths (bazel-controlled -I flags) ────
    include_paths_feature = feature(
        name = "include_paths",
        enabled = True,
        flag_sets = [
            flag_set(
                actions = compile_actions,
                flag_groups = [
                    flag_group(
                        flags = ["-iquote%{quote_include_paths}"],
                        iterate_over = "quote_include_paths",
                        expand_if_available = "quote_include_paths",
                    ),
                    flag_group(
                        flags = ["-I%{include_paths}"],
                        iterate_over = "include_paths",
                        expand_if_available = "include_paths",
                    ),
                    flag_group(
                        flags = ["-isystem%{system_include_paths}"],
                        iterate_over = "system_include_paths",
                        expand_if_available = "system_include_paths",
                    ),
                ],
            ),
        ],
    )

    # ── Feature: dependency_file (for Bazel's include scanning) ─
    dependency_file_feature = feature(
        name = "dependency_file",
        enabled = True,
        flag_sets = [
            flag_set(
                actions = compile_actions,
                flag_groups = [
                    flag_group(
                        flags = ["-MD", "-MF", "%{dependency_file}"],
                        expand_if_available = "dependency_file",
                    ),
                ],
            ),
        ],
    )

    # ── Feature: random_seed (for reproducible builds) ────────
    random_seed_feature = feature(
        name = "random_seed",
        enabled = True,
        flag_sets = [
            flag_set(
                actions = compile_actions,
                flag_groups = [
                    flag_group(
                        flags = ["-frandom-seed=%{output_file}"],
                        expand_if_available = "output_file",
                    ),
                ],
            ),
        ],
    )

    # ── Feature: output_execpath_flags ────────────────────────
    output_execpath_flags_feature = feature(
        name = "output_execpath_flags",
        enabled = True,
        flag_sets = [
            flag_set(
                actions = link_actions,
                flag_groups = [
                    flag_group(
                        flags = ["-o", "%{output_execpath}"],
                        expand_if_available = "output_execpath",
                    ),
                ],
            ),
        ],
    )

    # ── Feature: runtime_library_search_directories ───────────
    runtime_library_search_dirs_feature = feature(
        name = "runtime_library_search_directories",
        flag_sets = [
            flag_set(
                actions = link_actions,
                flag_groups = [
                    flag_group(
                        iterate_over = "runtime_library_search_directories",
                        flag_groups = [
                            flag_group(
                                flags = ["-Wl,-rpath,%{runtime_library_search_directories}"],
                                expand_if_available = "runtime_library_search_directories",
                            ),
                        ],
                        expand_if_available = "runtime_library_search_directories",
                    ),
                ],
            ),
        ],
    )

    # ── Feature: library_search_directories ───────────────────
    library_search_dirs_feature = feature(
        name = "library_search_directories",
        flag_sets = [
            flag_set(
                actions = link_actions,
                flag_groups = [
                    flag_group(
                        iterate_over = "library_search_directories",
                        flag_groups = [
                            flag_group(
                                flags = ["-L%{library_search_directories}"],
                                expand_if_available = "library_search_directories",
                            ),
                        ],
                        expand_if_available = "library_search_directories",
                    ),
                ],
            ),
        ],
    )

    # ── Feature: libraries_to_link ────────────────────────────
    libraries_to_link_feature = feature(
        name = "libraries_to_link",
        flag_sets = [
            flag_set(
                actions = link_actions,
                flag_groups = [
                    flag_group(
                        iterate_over = "libraries_to_link",
                        flag_groups = [
                            flag_group(
                                flags = ["-Wl,--start-lib"],
                                expand_if_equal = {
                                    "variable": "libraries_to_link.type",
                                    "value": "object_file_group",
                                },
                            ),
                            flag_group(
                                flags = ["%{libraries_to_link.object_files}"],
                                iterate_over = "libraries_to_link.object_files",
                                expand_if_equal = {
                                    "variable": "libraries_to_link.type",
                                    "value": "object_file_group",
                                },
                            ),
                            flag_group(
                                flags = ["-Wl,--end-lib"],
                                expand_if_equal = {
                                    "variable": "libraries_to_link.type",
                                    "value": "object_file_group",
                                },
                            ),
                            flag_group(
                                flags = ["%{libraries_to_link.name}"],
                                expand_if_equal = {
                                    "variable": "libraries_to_link.type",
                                    "value": "object_file",
                                },
                            ),
                            flag_group(
                                flags = ["%{libraries_to_link.name}"],
                                expand_if_equal = {
                                    "variable": "libraries_to_link.type",
                                    "value": "interface_library",
                                },
                            ),
                            flag_group(
                                flags = ["-Wl,-whole-archive", "%{libraries_to_link.name}", "-Wl,-no-whole-archive"],
                                expand_if_equal = {
                                    "variable": "libraries_to_link.type",
                                    "value": "static_library",
                                },
                                expand_if_true = "libraries_to_link.is_whole_archive",
                            ),
                            flag_group(
                                flags = ["%{libraries_to_link.name}"],
                                expand_if_equal = {
                                    "variable": "libraries_to_link.type",
                                    "value": "static_library",
                                },
                                expand_if_false = "libraries_to_link.is_whole_archive",
                            ),
                            flag_group(
                                flags = ["-l%{libraries_to_link.name}"],
                                expand_if_equal = {
                                    "variable": "libraries_to_link.type",
                                    "value": "dynamic_library",
                                },
                            ),
                            flag_group(
                                flags = ["-l:%{libraries_to_link.name}"],
                                expand_if_equal = {
                                    "variable": "libraries_to_link.type",
                                    "value": "versioned_dynamic_library",
                                },
                            ),
                        ],
                        expand_if_available = "libraries_to_link",
                    ),
                ],
            ),
        ],
    )

    features = [
        default_compile_feature,
        default_link_feature,
        opt_feature,
        dbg_feature,
        fastbuild_feature,
        warnings_feature,
        strip_debug_feature,
        supports_start_end_lib,
        supports_pic,
        user_compile_flags_feature,
        include_paths_feature,
        dependency_file_feature,
        random_seed_feature,
        output_execpath_flags_feature,
        runtime_library_search_dirs_feature,
        library_search_dirs_feature,
        libraries_to_link_feature,
    ]

    return cc_common.create_cc_toolchain_config_info(
        ctx              = ctx,
        features         = features,
        action_configs   = [],
        tool_paths       = tool_paths_list,
        cxx_builtin_include_directories = [
            "/opt/clang-p2996/lib/clang/current/include",
            "/opt/clang-p2996/include/c++/v1",
            "/usr/include",
            "/usr/local/include",
        ],
        toolchain_identifier  = "p2996-clang",
        host_system_name      = "local",
        target_system_name    = "local",
        target_cpu            = "k8",
        target_libc           = "glibc",
        compiler              = "clang-p2996",
        abi_version           = "clang",
        abi_libc_version      = "glibc",
    )

cc_toolchain_config = rule(
    implementation = _impl,
    attrs = {
        "compiler_path":       attr.string(mandatory = True),
        "ar_path":             attr.string(mandatory = True),
        "strip_path":          attr.string(mandatory = True),
        "nm_path":             attr.string(mandatory = True),
        "objcopy_path":        attr.string(mandatory = True),
        "objdump_path":        attr.string(mandatory = True),
        "ld_path":             attr.string(mandatory = True),
        "sysroot":             attr.string(default = ""),
        "resource_dir_flag":   attr.string(mandatory = True),
        "stdlib_includes":     attr.string_list(default = []),
        "stdlib_lib_dirs":     attr.string_list(default = []),
        "extra_compile_flags": attr.string_list(default = []),
        "extra_link_flags":    attr.string_list(default = []),
    },
    provides = [CcToolchainConfigInfo],
)
