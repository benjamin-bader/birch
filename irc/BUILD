load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

cc_library(
    name = "message",
    srcs = ["Message.cc"],
    hdrs = ["Message.h"],
    deps = [
        "@abseil-cpp//absl/container:flat_hash_map",
        "@abseil-cpp//absl/container:linked_hash_map",
    ],
    visibility = ["//visibility:public"],
)

cc_test(
    name = "message_test",
    size = "small",
    srcs = ["MessageTest.cc"],
    deps = [
        ":message",
        "//tools:test_main",
        "@abseil-cpp//absl/strings",
        "@abseil-cpp//absl/strings:str_format",
    ],
)

cc_library(
    name = "user",
    hdrs = ["User.h"],
    srcs = ["User.cc"],
    deps = [
        "@abseil-cpp//absl/container:flat_hash_map",
        "@abseil-cpp//absl/log",
        "@abseil-cpp//absl/strings",
        "@abseil-cpp//absl/time",
    ],
)

cc_library(
    name = "channel",
    hdrs = ["Channel.h"],
    srcs = ["Channel.cc"],
    deps = [
        ":user",
        "@abseil-cpp//absl/container:flat_hash_map",
        "@abseil-cpp//absl/log",
        "@abseil-cpp//absl/log:check",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/status:statusor",
    ],
)

cc_library(
    name = "irc_state",
    hdrs = ["IrcState.h"],
    srcs = ["IrcState.cc"],
    deps = [
        ":channel",
        ":user",
        "//server:asio_ssl",
        "@abseil-cpp//absl/container:flat_hash_map",
        "@abseil-cpp//absl/log",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/status:statusor",
        "@abseil-cpp//absl/strings",
    ],
)

cc_library(
    name = "i_command",
    hdrs = ["ICommand.h"],
    deps = [
        ":irc_state",
        ":user",
        "@abseil-cpp//absl/status",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "nick_registry",
    hdrs = ["NickRegistry.h"],
    srcs = ["NickRegistry.cc"],
    deps = [
        "@abseil-cpp//absl/container:flat_hash_map",
        "@abseil-cpp//absl/log",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/strings",
    ],
)

cc_library(
    name = "session",
    hdrs = ["Session.h"],
    srcs = ["Session.cc"],
    deps = [
        ":capability_set",
        ":message",
        "//util:observable",
        "@abseil-cpp//absl/container:flat_hash_map",
        "@abseil-cpp//absl/log",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/strings",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "capability_set",
    hdrs = ["CapabilitySet.h"],
    srcs = ["CapabilitySet.cc"],
    deps = [
        ":capabilities",
        "@abseil-cpp//absl/container:flat_hash_map",
        "@abseil-cpp//absl/container:flat_hash_set",
        "@abseil-cpp//absl/log",
        "@abseil-cpp//absl/log:check",
    ],
    visibility = ["//visibility:public"],
)

cc_test(
    name = "capability_set_test",
    srcs = ["CapabilitySetTest.cc"],
    deps = [
        ":capability_set",
        "//tools:test_main",
    ],
    size = "small",
)

cc_library(
    name = "capabilities",
    hdrs = ["Capabilities.h"],
    srcs = ["Capabilities.cc"],
    deps = [
        "//proto:cc_caps_proto",
        "@abseil-cpp//absl/log",
        "@abseil-cpp//absl/log:check",
    ],
    visibility = ["//visibility:public"],
)

cc_test(
    name = "capabilities_test",
    srcs = ["CapabilitiesTest.cc"],
    deps = [
        ":capabilities",
        "//tools:test_main",
        "@googletest//:gtest",
    ],
    size = "small",
)
