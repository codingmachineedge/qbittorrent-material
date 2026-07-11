# Troubleshooting

## The Windows helper cannot find a compiler

Install Visual Studio 2022 Build Tools with the Desktop development with C++ workload, then reopen the terminal so discovery can see the installation.

## The first configure step is slow

CMake invokes vcpkg during configure. A cold run may compile native dependencies for tens of minutes; later runs use the binary cache.

## The installed application reports a Qt platform error

Confirm that `plugins/platforms/qwindows.dll` exists beside the installed `bin` directory. Always test the packaged install tree rather than only the development executable.

## The documentation search reports an invalid pattern

Open the regex builder and inspect the validation message. Unbalanced groups, invalid character ranges, and unsupported flags are the most common causes. Disable Regex to search for the literal text.

## Imported pages disappeared

Imports are local to the current browser profile. Export a wiki JSON bundle before clearing site data or moving to another browser.

## GitHub Pages shows an old page

Wait for the Pages deployment to finish, force-refresh once, and check that `content.generated.js` was regenerated after documentation changes.
