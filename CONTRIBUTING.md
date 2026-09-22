# Contributing

Anyone can contribute to this project without requirements or reservation.

You may contribute code, documentation, tests, bug reports, design ideas, or
any other improvement. Contributions may be large or small, complete or
unfinished, and you do not need prior approval, an invitation, or a specific
background to participate.

There is no required format for contributions. When practical, include a brief
description of what changed and why, along with any useful steps to reproduce
or verify it. If you are unsure how to proceed, open an issue or submit a draft
change and explain what you are trying to do.

GitHub Actions runs the portable checks automatically on pull requests. If you
want to run them locally, use `sh tests/run_checks.sh` with C/C++ compilers and
Python 3 installed. These checks do not require Unreal Engine and do not verify
the Unreal module or runtime behavior. An optional `CC`/`CXX` override selects
the compilers, for example `CC=clang CXX=clang++ sh tests/run_checks.sh`.

For user-visible changes, an entry under Unreleased in [CHANGELOG.md](CHANGELOG.md)
is helpful. Maintainers can help with checks and release notes; neither is a
prerequisite for submitting a contribution.

By contributing, you agree that your contribution is dedicated to the public
domain under the terms of the [Unlicense](LICENSE), consistent with this
project's license.
