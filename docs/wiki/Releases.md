# Releases and Automation

## One prerelease per push

Every branch push runs the `Build and release every push` workflow on Windows Server 2022. It configures MSVC and Qt, restores vcpkg caches, builds the application, packages NSIS, performs the installed-app smoke test, and creates a uniquely tagged prerelease.

Tags follow this form:

```text
build-<workflow-run-number>-<8-character-commit>
```

The installer is uploaded directly as a GitHub Release asset. The workflow intentionally retains no ordinary Actions artifact.

## Pages publishing

GitHub Pages publishes directly from `master/docs`. That keeps the documentation deployment static and artifact-free. The generated `content.generated.js` file is committed so the browser needs no build tool or API at runtime.

## Release confidence gates

1. Configure with the pinned Qt and vcpkg toolchain.
2. Compile the Release target.
3. Build exactly one expected NSIS installer.
4. Hash the package with SHA-256.
5. Install silently into an isolated directory.
6. Verify the executable and Qt platform plugin.
7. Launch offscreen and require it to stay alive.
8. Uninstall silently.
9. Publish the exact tested installer.
