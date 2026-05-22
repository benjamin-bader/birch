load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

cc_library(
    name = "file_watcher_interface",
    hdrs = ["IFileWatcher.h"],
    deps = [
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
    srcs = ["KqueueFileWatcherTest.cc"],
    size = "small",
    target_compatible_with = select({
        "@platforms//os:macos": [],
        "//conditions:default": ["@platforms//:incompatible"],
    }),
    deps = [
        ":kqueue_file_watcher",
        "//tools:test_main",
    ],
)

cc_library(
    name = "file_watcher",
    srcs = ["FileWatcher.cc"],
    deps = [
        ":file_watcher_interface",
        "@abseil-cpp//absl/status:statusor",
    ] + select({
        "@platforms//os:macos": [":kqueue_file_watcher"],
        "//conditions:default": [],
    }),
    visibility = ["//visibility:public"],
)
