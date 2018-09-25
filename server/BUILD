load("//:vars.bzl", "COPTS")

cc_library(
    name = "message",
    srcs = ["Message.cc"],
    hdrs = ["Message.h"],
    copts = COPTS,
)

cc_library(
    name = "message_parser",
    srcs = ["MessageParser.cc"],
    hdrs = ["MessageParser.h"],
    copts = COPTS,
    deps = [
        "message",
    ],
)

cc_test(
    name = "message_parser_test",
    srcs = ["MessageParserTest.cc"],
    copts = COPTS,
    deps = ["message_parser", "@gtest//:gtest_main"],
)

cc_library(
    name = "channel",
    srcs = ["Channel.cc"],
    hdrs = ["Channel.h"],
    copts = COPTS,
)

cc_test(
    name = "channel_test",
    srcs = ["ChannelTest.cc"],
    copts = COPTS,
    deps = ["channel", "@gtest//:gtest_main"]
)

cc_library(
    name = "connection",
    srcs = ["Connection.cc"],
    hdrs = ["Connection.h"],
    copts = COPTS,
)

cc_library(
    name = "user",
    srcs = ["User.cc"],
    hdrs = ["User.h"],
    copts = COPTS,
)

cc_library(
    name = "server",
    srcs = ["Server.cc"],
    hdrs = ["Server.h"],
    deps = [
        "channel",
        "message_parser",
        "connection",
        "user",
        "@asio//:asio",
        "@boringssl//:ssl",
    ],
    copts = COPTS,
    visibility = ["//visibility:public"],
)
