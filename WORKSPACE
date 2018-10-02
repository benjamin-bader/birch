# Docs suggest that "http_archive" and friends have deprecated implementations,
# and better ones can be had by manually loading them.
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

git_repository(
    name = "absl",
    commit = "92e07e5590752d6b8e67f7f2f86c6286561e8cea",  # 2018-08-01
    remote = "https://github.com/abseil/abseil-cpp"
)

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
    name = "tclap",
    url = "https://github.com/eile/tclap/archive/tclap-1-2-1-release-final.tar.gz",
    sha256 = "f0ede0721dddbb5eba3a47385a6e8681b14f155e1129dd39d1a959411935098f",
    strip_prefix = "tclap-tclap-1-2-1-release-final",
    build_file = "third_party/tclap.BUILD",
)

new_http_archive(
    name = "zlib",
    url = "https://zlib.net/zlib-1.2.11.tar.gz",
    sha256 = "c3e5e9fdd5004dcb542feda5ee4f0ff0744628baf8ed2dd5d66f8ca1197cb1a1",
    strip_prefix = "zlib-1.2.11",
    build_file = "third_party/zlib.BUILD",
)

new_git_repository(
    name = "spdlog",
    commit = "b6b9d835c588c35227410a9830e7a4586f90777a", # Version 1.1.0
    remote = "https://github.com/gabime/spdlog",
    build_file = "third_party/spdlog.BUILD",
)

new_git_repository(
    name = "fmtlib",
    commit = "3e75ad9822980e41bc591938f26548f24eb88907", # Version 5.2.1
    remote = "https://github.com/fmtlib/fmt",
    build_file = "third_party/fmtlib.BUILD",
)
