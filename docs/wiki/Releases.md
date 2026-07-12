# Releases and Automation

## One release per push

Every branch push runs the `Build and release every push` workflow on Windows Server 2022. It configures MSVC and Qt, restores vcpkg caches, builds the application, packages NSIS, performs the installed-app smoke test, and creates a uniquely tagged, full GitHub release marked "latest."

The dependency restore includes libgit2, which powers the app's private
workspace repository without requiring `git.exe`. The installed-app launch gate
therefore also verifies that the packaged workspace runtime dependencies can be
loaded before an installer is published.

Tags follow this form:

```text
build-<workflow-run-number>-<8-character-commit>
```

The installer is uploaded directly as a GitHub Release asset. The workflow intentionally retains no ordinary Actions artifact.

GitHub Pages can create a transient internal deployment artifact when `master/docs` changes. The installer workflow's final cleanup step removes completed Actions artifacts after every push, while leaving the tested installer safely attached to its GitHub Release.

## Pages publishing

GitHub Pages publishes directly from `master/docs`. That keeps the documentation deployment static and artifact-free. The generated `content.generated.js` file is committed so the browser needs no build tool or API at runtime.

## Release confidence gates

1. Configure with the pinned Qt, libgit2, and vcpkg toolchain.
2. Compile the Release target.
3. Build exactly one expected NSIS installer.
4. Hash the package with SHA-256.
5. Install silently into an isolated directory.
6. Verify the executable and Qt platform plugin.
7. Launch offscreen and require it to stay alive.
8. Uninstall silently.
9. Publish the exact tested installer.
