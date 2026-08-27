# RP2350-Zero carrier 組立

## J1/J3の向きは必ず確認する

J1/J3の穴は左右対称に見えるため、RP2350-Zeroを裏返しても仮置きでは穴が合います。しかしその向きで半田付けすると、5VとGPIOなどが鏡映され、重大な誤配線になります。ピンソケットは取り外せない前提で、次の確認を半田付け前に必ず行ってください。

1. キャリアの **F.Cu（表面）** を上にします。ソケット外側の `USB-C / BOOT`、太い矢印、V字ノッチ、`1=5V` が読める面です。
2. メスソケットをその表面に仮置きします。矢印とV字ノッチが示す **+Y側（横5穴列の反対端）** が、RP2350-ZeroのUSB-C/BOOT/RUN側です。`1=5V` は左上のpin 1も示します。
3. Zeroは、USB-C・BOOT/RUN・LEDがある**部品面をキャリアから外側（上）**、オスピンを下にして仮挿入します。USB-C/BOOT/RUNが矢印側にあり、BOOT/RUNとLEDが上から見えることを確認します。
4. 反対面から見てBOOT/RUNがキャリア側に隠れる場合は中止してください。その姿勢は穴が合っても誤りです。
5. 仮挿入を外し、メスソケットをF.Cu面から片端の1ピンだけ仮止めします。向きと直角を再確認してから残りを半田付けします。

実装後は、J1/J3ともUSB-Cが矢印側に出ていてBOOT/RUNを押せることを確認してください。

## Release PDF paper size

Run `scripts/build_gerbers.sh` to regenerate the release drawings. The
schematic is fitted to ISO A4 landscape. The PCB drawing is exported at
physical scale 1:1 after translating a temporary copy into the page area, so
the complete board is centered without clipping or changing its dimensions.
