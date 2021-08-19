### GRecovery

* Data Recovery Utility
* Base in [PhotoRec](https://www.cgsecurity.org)


#### Dependencies

* gcc, meson
* libz ( & dev )
* ext2fs ( & dev )
* libjpeg ( & dev )
* libntfs-3g ( & dev )
* libgtk 3.0 ( & dev )


#### Build

1. Configure: meson build --prefix /usr --strip

2. Build: ninja -C build

3. Install: sudo ninja -C build install

4. Uninstall: sudo ninja -C build uninstall
