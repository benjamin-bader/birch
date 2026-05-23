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
        "@asio",
    ],
)

cc_test(
    name = "message_parser_test",
    size = "small",
    srcs = ["MessageParserTest.cc"],
    deps = [
        "message_parser",
        "//tools:test_main",
        "@googletest//:gtest",
    ],
)

cc_library(
    name = "channel",
    srcs = ["Channel.cc"],
    hdrs = ["Channel.h"],
)

cc_test(
    name = "channel_test",
    size = "small",
    srcs = ["ChannelTest.cc"],
    deps = [
        ":channel",
        "//tools:test_main",
        "@googletest//:gtest",
    ],
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
    visibility = ["//visibility:public"],
    deps = [
        ":channel",
        ":connection",
        ":message_parser",
        ":user",
        "//config",
        "@abseil-cpp//absl/log",
        "@asio",
    ],
)
