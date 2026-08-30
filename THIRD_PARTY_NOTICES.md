# Third-Party Notices

This project does not commit fetched dependencies to the repository. The build
scripts download them into `lib/` or the Docker image. The following notices
apply to the versions used by the build configuration.

## TinyUSB

- Project: https://github.com/hathach/tinyusb
- Version: `0.20.0`
- License: MIT License
- Copyright: Copyright (c) 2018, hathach (tinyusb.org)
- License text: https://github.com/hathach/tinyusb/blob/0.20.0/LICENSE

```text
The MIT License (MIT)

Copyright (c) 2018, hathach (tinyusb.org)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
```

## Pico-PIO-USB

- Project: https://github.com/sekigon-gonnoc/Pico-PIO-USB
- Revision: `a5a2f5ae91988449ba576ec9d237e806d5cd4416`
- License: MIT License
- Copyright: Copyright (c) 2021 sekigon-gonnoc
- License text: https://github.com/sekigon-gonnoc/Pico-PIO-USB/blob/a5a2f5ae91988449ba576ec9d237e806d5cd4416/LICENSE

```text
MIT License

Copyright (c) 2021 sekigon-gonnoc

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

## Raspberry Pi Pico SDK

- Project: https://github.com/raspberrypi/pico-sdk
- Version: `2.1.1`
- License: BSD 3-Clause "New" or "Revised" License
- Copyright: Copyright 2020 Raspberry Pi (Trading) Ltd.
- License text: https://github.com/raspberrypi/pico-sdk/blob/2.1.1/LICENSE.TXT

```text
Copyright 2020 (c) 2020 Raspberry Pi (Trading) Ltd.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
```

The Pico SDK may contain additional third-party components. Their license
notices are distributed with the SDK and remain subject to their respective
terms.

## picotool

picotool is used by the Pico SDK to generate UF2 output in the Docker build;
it is not linked into the firmware.

- Project: https://github.com/raspberrypi/picotool
- Version: `2.1.1`
- License: BSD 3-Clause "New" or "Revised" License
- Copyright: Copyright 2020 Raspberry Pi (Trading) Ltd.
- License text: https://github.com/raspberrypi/picotool/blob/2.1.1/LICENSE.TXT

picotool uses the same BSD 3-Clause license text shown above.

## Build Environment

The Docker image installs Debian packages including CMake, GCC Arm
Embedded, Newlib, libstdc++, libusb, Python, Git, and CA certificates. Those
packages are build-time tools and are not distributed by this repository; the
applicable Debian and package-specific licenses apply to each installation.
# Raspberry Pi pico-examples WS2812 PIO program

The WS2812 PIO timing program in `src/ws2812.pio` is derived from the
Raspberry Pi `pico-examples` WS2812 example.

Copyright (c) 2020 Raspberry Pi (Trading) Ltd.

SPDX-License-Identifier: BSD-3-Clause
