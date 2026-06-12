birch
-----

An IRC daemon, written as an exercise in reading RFCs and playing with the Bazel build system.
No guarantees that it will ever be useful.

Licensed under GPLv3.

Scaffolding new classes
-----------------------

New C++ classes follow a fixed convention (GPLv3 header, `BIRCH_<SUBSYS>_<CLASS>_H`
include guard, `namespace birch::<subsystem>`). Rather than copy these by hand, use:

    tools/newclass.py <subsystem> <ClassName>

For example, `tools/newclass.py irc NickRegistry` creates `irc/NickRegistry.h`,
`irc/NickRegistry.cc`, and `irc/NickRegistryTest.cc`, and appends stub `cc_library`
and `cc_test` targets to `irc/BUILD`. Fill in the `deps = [...]` afterwards.

Options:

  - `--header-only`  Emit only the `.h` (interface header); no `.cc` or test.
  - `--no-test`      Skip the `Test.cc` file and its `cc_test` stanza.
  - `--no-build`     Leave the `BUILD` file untouched.
  - `--force`        Overwrite existing source files.

-----------
Copyright 2018 Benjamin Bader