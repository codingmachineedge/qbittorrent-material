; Register the app as the handler for .torrent files and magnet: links so
; double-clicking a .torrent or clicking a magnet link launches it with the
; source as argv[1] (Application::run() adds it on cold start).
;
; This lives in its own .nsh file (included verbatim via CPACK_NSIS_EXTRA_
; INSTALL_COMMANDS with a single !include line) instead of being embedded as
; a CMake string, because round-tripping quotes/backslashes through
; CMakeLists.txt -> CPackConfig.cmake -> project.nsi corrupts them.
WriteRegStr HKCR ".torrent" "" "qBittorrentMaterial.torrent"
WriteRegStr HKCR ".torrent" "Content Type" "application/x-bittorrent"
WriteRegStr HKCR "qBittorrentMaterial.torrent" "" "BitTorrent Torrent"
WriteRegStr HKCR "qBittorrentMaterial.torrent\DefaultIcon" "" "$INSTDIR\bin\qbittorrent.exe,0"
WriteRegStr HKCR "qBittorrentMaterial.torrent\shell\open\command" "" '"$INSTDIR\bin\qbittorrent.exe" "%1"'
WriteRegStr HKCR "magnet" "" "URL:Magnet Link"
WriteRegStr HKCR "magnet" "URL Protocol" ""
WriteRegStr HKCR "magnet\DefaultIcon" "" "$INSTDIR\bin\qbittorrent.exe,0"
WriteRegStr HKCR "magnet\shell\open\command" "" '"$INSTDIR\bin\qbittorrent.exe" "%1"'
System::Call 'shell32::SHChangeNotify(i 0x08000000, i 0, i 0, i 0)'
