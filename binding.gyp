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
        }]
      ]
    }
  ]
}
