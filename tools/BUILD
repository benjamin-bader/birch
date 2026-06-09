load("@aspect_rules_lint//format:defs.bzl", "format_multirun")
load("@rules_cc//cc:defs.bzl", "cc_library")

format_multirun(name = "format")

cc_library(
    name = "test_main",
    testonly = True,
    srcs = ["TestMain.cc"],
    deps = [
        "@abseil-cpp//absl/debugging:failure_signal_handler",
        "@abseil-cpp//absl/debugging:symbolize",
        "@abseil-cpp//absl/log",
        "@abseil-cpp//absl/log:initialize",
        "@abseil-cpp//absl/status:status_matchers",
        "@googletest//:gtest",
    ],
    visibility = ["//visibility:public"],
)
