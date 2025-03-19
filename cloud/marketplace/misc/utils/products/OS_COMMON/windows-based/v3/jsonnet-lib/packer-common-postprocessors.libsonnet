//"post-processors": [
//    {
//      "type": "manifest",
//      "output": "{{ pwd }}/manifest.json",
//      "strip_path": true
//    }
//  ]

local manifest = {
  type: 'manifest',
  output: '{{ pwd }}/manifest.json',
  strip_path: true,
};

{
  fabric_common: [manifest],
}
