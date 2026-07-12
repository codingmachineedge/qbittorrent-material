; Remove the .torrent/magnet: file associations written by
; file_associations_install.nsh. See that file for why this lives in its own
; .nsh instead of a CMake string.
DeleteRegKey HKCR "qBittorrentMaterial.torrent"
DeleteRegValue HKCR ".torrent" ""
DeleteRegKey HKCR "magnet\shell\open\command"
DeleteRegKey HKCR "magnet\shell\open"
DeleteRegKey HKCR "magnet\shell"
DeleteRegKey HKCR "magnet\DefaultIcon"
DeleteRegValue HKCR "magnet" "URL Protocol"
System::Call 'shell32::SHChangeNotify(i 0x08000000, i 0, i 0, i 0)'
