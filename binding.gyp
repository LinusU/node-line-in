{
  "targets": [
    {
      "target_name": "line-in",
      "include_dirs" : [
        "<!(node -e \"require('nan')\")"
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
            "-lpulse",
            "-lpulse-simple"
          ]
        }]
      ]
    }
  ]
}
