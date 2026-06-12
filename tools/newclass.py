#!/usr/bin/env python3
# birch - IRC daemon, built with Bazel
#
# Copyright (C) 2026 Benjamin Bader
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
"""Scaffold a new C++ class (.h / .cc / Test.cc) following birch conventions.

Usage:
    tools/newclass.py <subsystem> <ClassName> [options]

Examples:
    tools/newclass.py irc NickRegistry
    tools/newclass.py config IConfigThing --header-only

Options:
    --header-only   Emit only the .h (interface header); no .cc, no test.
    --no-test       Skip the ClassNameTest.cc file and its cc_test stanza.
    --no-build      Do not touch the subsystem's BUILD file.
    --force         Overwrite existing source files.
"""

import argparse
import re
import sys
from datetime import datetime
from pathlib import Path

YEAR = datetime.now().strftime("%Y")

LICENSE = f"""// birch - IRC daemon, built with Bazel
//
// Copyright (C) {YEAR} Benjamin Bader
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
"""

RULES_CC_LOAD = 'load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")'


def to_snake(name: str) -> str:
    """Convert a CamelCase class name to snake_case.

    NickRegistry         -> nick_registry
    FileConfigDataSource -> file_config_data_source
    ICommand             -> i_command
    """
    s = re.sub(r"(?<=[A-Z])(?=[A-Z][a-z])", "_", name)
    s = re.sub(r"(?<=[a-z0-9])(?=[A-Z])", "_", s)
    return s.lower()


def header_text(subsystem: str, cls: str) -> str:
    guard = f"BIRCH_{to_snake(subsystem).upper()}_{to_snake(cls).upper()}_H"
    return (
        f"{LICENSE}\n"
        f"#ifndef {guard}\n"
        f"#define {guard}\n\n"
        f"namespace birch::{subsystem} {{\n\n\n\n"
        f"}} // namespace birch::{subsystem}\n\n"
        f"#endif\n"
    )


def source_text(subsystem: str, cls: str) -> str:
    return (
        f"{LICENSE}\n"
        f'#include "{cls}.h"\n\n'
        f"namespace birch::{subsystem} {{\n\n\n\n"
        f"}} // namespace birch::{subsystem}\n"
    )


def test_text(subsystem: str, cls: str) -> str:
    return (
        f"{LICENSE}\n"
        f'#include "{cls}.h"\n\n'
        f'#include "gtest/gtest.h"\n\n'
        f"namespace birch::{subsystem} {{\n\n"
        f"TEST({cls}Test, Smoke)\n"
        f"{{\n"
        f"    // TODO: write a real test\n"
        f"}}\n\n"
        f"}} // namespace birch::{subsystem}\n"
    )


def library_stanza(cls: str, target: str, header_only: bool) -> str:
    srcs = "" if header_only else f'    srcs = ["{cls}.cc"],\n'
    return (
        f"\ncc_library(\n"
        f'    name = "{target}",\n'
        f'    hdrs = ["{cls}.h"],\n'
        f"{srcs}"
        f"    deps = [\n"
        f"    ],\n"
        f'    visibility = ["//visibility:public"],\n'
        f")\n"
    )


def test_stanza(cls: str, target: str) -> str:
    return (
        f"\ncc_test(\n"
        f'    name = "{target}_test",\n'
        f'    srcs = ["{cls}Test.cc"],\n'
        f"    deps = [\n"
        f'        ":{target}",\n'
        f'        "//tools:test_main",\n'
        f'        "@googletest//:gtest",\n'
        f"    ],\n"
        f'    size = "small",\n'
        f")\n"
    )


def write_file(path: Path, content: str, force: bool) -> None:
    if path.exists() and not force:
        sys.exit(f"refusing to overwrite existing file: {path} (use --force)")
    path.write_text(content)
    print(f"  wrote {path}")


def update_build(build: Path, stanzas: str) -> None:
    if not build.exists():
        build.write_text(f"{RULES_CC_LOAD}\n{stanzas}")
        print(f"  created {build}")
        return

    text = build.read_text()
    load_match = re.search(r'^load\("@rules_cc//cc:defs\.bzl".*\)$', text, re.MULTILINE)
    if load_match:
        line = load_match.group(0)
        needed = [s for s in ("cc_library", "cc_test") if f'"{s}"' not in line]
        if needed:
            additions = "".join(f', "{s}"' for s in needed)
            new_line = line[:-1] + additions + ")"
            text = text.replace(line, new_line, 1)
    else:
        text = f"{RULES_CC_LOAD}\n" + text
    if not text.endswith("\n"):
        text += "\n"
    text += stanzas
    build.write_text(text)
    print(f"  updated {build}")


def main() -> None:
    p = argparse.ArgumentParser(description="Scaffold a birch C++ class.")
    p.add_argument("subsystem", help="subsystem dir, e.g. irc, server, config")
    p.add_argument("classname", help="CamelCase class name, e.g. NickRegistry")
    p.add_argument(
        "--header-only", action="store_true", help="interface header; no .cc or test"
    )
    p.add_argument("--no-test", action="store_true", help="skip the Test.cc file")
    p.add_argument(
        "--no-build", action="store_true", help="do not modify the BUILD file"
    )
    p.add_argument("--force", action="store_true", help="overwrite existing files")
    args = p.parse_args()

    subsystem = args.subsystem.strip("/")
    cls = args.classname
    if not re.match(r"^[A-Z][A-Za-z0-9]*$", cls):
        sys.exit(f"class name must be CamelCase: {cls!r}")

    root = Path.cwd()
    subdir = root / subsystem
    if not subdir.is_dir():
        sys.exit(f"subsystem directory does not exist: {subdir}")

    target = to_snake(cls)
    emit_cc = not args.header_only
    emit_test = emit_cc and not args.no_test

    print(f"scaffolding birch::{subsystem}::{cls} (target //{subsystem}:{target})")
    write_file(subdir / f"{cls}.h", header_text(subsystem, cls), args.force)
    if emit_cc:
        write_file(subdir / f"{cls}.cc", source_text(subsystem, cls), args.force)
    if emit_test:
        write_file(subdir / f"{cls}Test.cc", test_text(subsystem, cls), args.force)

    if not args.no_build:
        stanzas = library_stanza(cls, target, args.header_only)
        if emit_test:
            stanzas += test_stanza(cls, target)
        update_build(subdir / "BUILD", stanzas)
        print("  NOTE: fill in deps = [...] for the new target(s).")


if __name__ == "__main__":
    main()
