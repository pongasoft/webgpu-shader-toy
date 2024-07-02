To add new icons:

* Generate kit on FontAwesome and download the web zip file
* Open zip and copy `metadata/icons.yml` and `webfonts/fa-solid-900.ttf` to `external/fontawesome/src/icons`
* From `external/fontawesome`, run `gen-font.sh`

Note: requires `binary_to_compressed_c` to be in the PATH (this can be compiled from ImGui distribution)