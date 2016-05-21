{
  "targets": [
    {
      "target_name": "line-in",
      "include_dirs" : [
        "<!(node -e \"require('nan')\")"
      ],
      "cflags": [
        "-std=c++11"
      ],
      "conditions": [
        ["OS == 'mac'", {
          "sources": [
            "src/darwin.cc"
          ],
          "link_settings": {
            "libraries": [
              "-framework", "AudioToolBox"
            ]
          }
        }],
        ["OS == 'linux'", {
          "sources": [
            "src/pulse.cc"
          ],
          "include_dirs": [
            "vendor"
          ],
          "libraries": [
            "-l:libpulse.so.0",
            "-l:libpulse-simple.so.0"
          ]
        }]
      ]
    }
  ]
}
