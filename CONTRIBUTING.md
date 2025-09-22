# Contributing

Thanks for your interest in improving the GKrellM NVIDIA GPU plugin!

## Workflow
1. Fork the repository and create a topic branch (`git checkout -b feature/my-change`).
2. Keep commits focused; explain the motivation in the message body.
3. Run `make` to ensure the plugin builds cleanly and `make clean` before committing generated files.
4. Update or add documentation (`README.md`, `docs/`) when behaviour changes.
5. Open a pull request describing the change, testing performed, and any known limitations.

## Coding Style
- Follow GKrellM's existing C conventions (tabs for indentation, braces on new lines, descriptive comments only for non-obvious sections).
- Avoid introducing dependencies beyond GTK+ 2.0 and NVML.
- Keep source files under 1000 lines; split helper functions if necessary.

## Reporting Issues
Use GitHub Issues and include:
- GKrellM version and distribution.
- NVIDIA driver version and GPU model.
- Steps to reproduce the problem.
- Relevant logs or terminal output.
