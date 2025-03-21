import { terser } from "rollup-plugin-terser";
import license from "rollup-plugin-license";

const LICENSE = `
This file is part of Adblock Plus <https://adblockplus.org/>,
Copyright (C) 2006-present eyeo GmbH

Adblock Plus is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License version 3 as
published by the Free Software Foundation.

Adblock Plus is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Adblock Plus.  If not, see <http://www.gnu.org/licenses/>.
`;

export default [
  {
    input: 'adblock-betafish/alias/isolated.js',
    output: getOutputOption('dist/bundle.isolated.js')
  },
];

function getOutputOption(file) {
  return {
    file,
    format: 'cjs',
    sourcemap: 'hidden',
    plugins: [
      terser({
        module: false,
        toplevel: false,
        compress: false
      }),
      license({
        banner: LICENSE,
      }),
    ]
  }
}
