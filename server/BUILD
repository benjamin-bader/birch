load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

cc_library(
    name = "message",
    srcs = ["Message.cc"],
    hdrs = ["Message.h"],
)

cc_library(
    name = "message_parser",
    srcs = ["MessageParser.cc"],
    hdrs = ["MessageParser.h"],
    deps = [
        "message",
        "@asio//:asio",
    ],
    copts = [
        # ASIO spuriously triggers this warning on macOS.
        "-Wno-unused-local-typedef",
    ],
)

cc_test(
    name = "message_parser_test",
    srcs = ["MessageParserTest.cc"],
    deps = [
        "message_parser",
        "//tools:test_main",
        "@googletest//:gtest",
    ],
    size = "small",
)

cc_library(
    name = "channel",
    srcs = ["Channel.cc"],
    hdrs = ["Channel.h"],
)

cc_test(
    name = "channel_test",
    srcs = ["ChannelTest.cc"],
    deps = [
        ":channel",
        "//tools:test_main",
        "@googletest//:gtest",
    ],
    size = "small",
)

cc_library(
    name = "connection",
    srcs = ["Connection.cc"],
    hdrs = ["Connection.h"],
)

cc_library(
    name = "connection_list",
    srcs = ["ConnectionList.cc"],
    hdrs = ["ConnectionList.h"],
    deps = [
        ":connection",
        ":dispatcher",
    ],
)

cc_library(
    name = "dispatcher",
    srcs = ["Dispatcher.cc"],
    hdrs = ["Dispatcher.h"],
    deps = [
        ":connection",
        ":message",
        "@abseil-cpp//absl/log",
    ],
)

cc_library(
    name = "user",
    srcs = ["User.cc"],
    hdrs = ["User.h"],
)

cc_library(
    name = "server",
    srcs = ["Server.cc"],
    hdrs = ["Server.h"],
    deps = [
        ":channel",
        ":message_parser",
        ":connection",
        ":user",
        "//config",
        "@abseil-cpp//absl/log",
        "@asio//:asio",
    ],
    copts = [
        # ASIO spuriously triggers this warning on macOS.
        "-Wno-unused-local-typedef",
    ],
    visibility = ["//visibility:public"],
)
