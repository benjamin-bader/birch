# Docs suggest that "http_archive" and friends have deprecated implementations,
# and better ones can be had by manually loading them.
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

git_repository(
    name = "boringssl",
    commit = "1c91287e05463520c75877af46b665880d11ab63",
    remote = "https://boringssl.googlesource.com/boringssl",
)

http_archive(
    name = "gtest",
    url = "https://github.com/google/googletest/archive/release-1.8.1.zip",
    sha256 = "927827c183d01734cc5cfef85e0ff3f5a92ffe6188e0d18e909c5efebf28a0c7",
    strip_prefix = "googletest-release-1.8.1",
)

new_http_archive(
    name = "asio",
    url = "https://github.com/chriskohlhoff/asio/archive/asio-1-12-0.zip",
    sha256 = "3b3f2e38ac18f9a20a405c0852c90e8b637b7733c520829ddc80937d2ee7a5ec",
    strip_prefix = "asio-asio-1-12-0/asio/include",
    build_file = "third_party/asio.BUILD",
)

new_http_archive(
    name = "sqlite",
    url = "https://www.sqlite.org/2018/sqlite-amalgamation-3240000.zip",
    sha256 = "ad68c1216c3a474cf360c7581a4001e952515b3649342100f2d7ca7c8e313da6",
    strip_prefix = "sqlite-amalgamation-3240000",
    build_file = "third_party/sqlite.BUILD",
)

new_http_archive(
    name = "zlib",
    url = "https://zlib.net/zlib-1.2.11.tar.gz",
    sha256 = "c3e5e9fdd5004dcb542feda5ee4f0ff0744628baf8ed2dd5d66f8ca1197cb1a1",
    strip_prefix = "zlib-1.2.11",
    build_file = "third_party/zlib.BUILD",
)