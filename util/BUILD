load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

cc_library(
    name = "file_watcher_interface",
    hdrs = ["IFileWatcher.h"],
    deps = [
        "@abseil-cpp//absl/status:statusor",
    ],
)

cc_library(
    name = "noop_file_watcher",
    hdrs = ["NoopFileWatcher.h"],
    srcs = ["NoopFileWatcher.cc"],
    deps = [
        ":file_watcher_interface",
        "@abseil-cpp//absl/status:statusor",
    ],
)

cc_library(
    name = "inotify_file_watcher",
    srcs = ["InotifyFileWatcher.cc"],
    hdrs = ["InotifyFileWatcher.h"],
    target_compatible_with = select({
        "@platforms//os:linux": [],
        "//conditions:default": ["@platforms//:incompatible"],
    }),
    deps = [
        ":file_watcher_interface",
        "@abseil-cpp//absl/container:flat_hash_map",
        "@abseil-cpp//absl/log",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/status:statusor",
    ],
)

cc_library(
    name = "kqueue_file_watcher",
    srcs = ["KqueueFileWatcher.cc"],
    hdrs = ["KqueueFileWatcher.h"],
    target_compatible_with = select({
        "@platforms//os:macos": [],
        "//conditions:default": ["@platforms//:incompatible"],
    }),
    deps = [
        ":file_watcher_interface",
        "@abseil-cpp//absl/container:flat_hash_map",
        "@abseil-cpp//absl/container:flat_hash_set",
        "@abseil-cpp//absl/log",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/status:statusor",
    ],
)

cc_test(
    name = "kqueue_file_watcher_test",
    size = "small",
    srcs = ["KqueueFileWatcherTest.cc"],
    target_compatible_with = select({
        "@platforms//os:macos": [],
        "//conditions:default": ["@platforms//:incompatible"],
    }),
    deps = [
        ":kqueue_file_watcher",
        "//tools:test_main",
    ],
)

cc_test(
    name = "win_file_watcher_test",
    size = "small",
    srcs = ["WinFileWatcherTest.cc"],
    target_compatible_with = select({
        "@platforms//os:windows": [],
        "//conditions:default": ["@platforms//:incompatible"],
    }),
    deps = [
        ":win_file_watcher",
        "//tools:test_main",
    ],
)

cc_library(
    name = "win_file_watcher",
    srcs = ["WinFileWatcher.cc"],
    hdrs = ["WinFileWatcher.h"],
    linkopts = ["/DEFAULTLIB:user32.lib"],
    target_compatible_with = select({
        "@platforms//os:windows": [],
        "//conditions:default": ["@platforms//:incompatible"],
    }),
    deps = [
        ":file_watcher_interface",
        "@abseil-cpp//absl/container:flat_hash_map",
        "@abseil-cpp//absl/container:flat_hash_set",
        "@abseil-cpp//absl/log",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/status:statusor",
        "@abseil-cpp//absl/strings",
    ],
)

cc_library(
    name = "file_watcher",
    visibility = ["//visibility:public"],
    srcs = ["FileWatcher.cc"],
    deps = [
        ":file_watcher_interface",
        ":noop_file_watcher",
        "@abseil-cpp//absl/log",
    ] + select({
        "@platforms//os:linux": [":inotify_file_watcher"],
        "@platforms//os:macos": [":kqueue_file_watcher"],
        "@platforms//os:windows": [":win_file_watcher"],
    }),
)
