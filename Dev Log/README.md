# Pokemon Go Plus Emulator for ESP32

[![Kofi](https://img.shields.io/badge/Social%20Card-TriRealm's_Links-e2b683)](https://trirealm.carrd.co/)
[![Kofi](https://img.shields.io/badge/Kofi-Support_TriRealm-8755D6)](https://ko-fi.com/trirealm)


---

**TriRealm (22/08/2026)**

Working on adding more functionality:

- [x] Making a long button press boot up a web server to change settings on the go easily on mobile which will shut itself down once settings have been saved

- [x] Also made the button Toggle discoverability depending on it's current state (searching or not) with a bright RED if Max (4 Devices is reached) - Will also turn off Advertising mode with every new connection       automatically

- [x] Also re-writing the LED Handler, so I can run WS2812B's to have 1 LED per Paired Device (Capping at 4 Devices)

- [x] LED Brightness Settings on Web Server (WOW They're Bright LMAO)

- [x] Per-Device Settings (Auto-Spin & Auto-Catch) so all 4 devices can have their own combination of settings they desire

All these changes are theoretical right now (I only have ESP32-C5 Devkits, so I'm awaiting my ESP32-WROOM to arrive in the mail to flash and confirm the changes I'm making work)

All this code is implemented and the code seems sound and SHOULD work! but theoretical till have the hardware to test/trial run

---

**TriRealm (28/08/2026)**

Working Prototype On Scrap PCB's I had laying around, all features and changes function perfectly!

![Very First Working Prototype with changes to code above.](https://i.postimg.cc/yx9S4gPK/image.png)

Now I'm just waiting on my Custom PCB's designed to house these components without any visible wires and make it a nice clean package!


---

**TriRealm (30/08/2026)**

Altered the boot sequence to go through all 4 LED's to verify they're working while giving the impression its "loading" the softare!

Changed the settings configs and the Web-Server options to allow users to change the SSID and Password for their PGP-Emu Device

Made a [web flasher and UART Console](https://trirealm.github.io/pgpemu) linked on the [MAIN Page](https://github.com/TriRealm/pgpemu) and at the top of this Dev Log to allow users to easily flash to the latest version and upload their **secrets/keys & blobs** 

- this will allow users to change/monitor the console of the device which or change settings for those who are unable to configure a dev environment locally on a PC & be more END user friendly!

Currently Waiting on my Custom PCB's to arrive to get this all neatly onto one small form factor board that can be thrown into a bag with a battery pack and do its work!

The Current Version is designed to be ran from the ESP32's USB-C port for power, there is traces for all the components so there will be no wires that can be damaged from solder joins etc.

Next up in my task is admin for this project, so clean up the Main ReadMe file, and then start the process of filming and editing a video about all the changes/tutorials on how to use the flasher and web-server etc to make it as user-friendly as possible

---



---

Maybe for Future revisions?

[MP2759 Battery Management/Charger Chip](https://www.monolithicpower.com/en/products/battery-management/chargers/mp2759.html) - Would allow for battery to be connected to the unit directly and if the battery died, could plug in and charge WITHOUT having to disconnect the battery and STILL actively use the PGPEmu Safely (Chip has `Power Path Management`)

[FCC No. of ESP32 Devboard](https://fccid.io/2A54N-ESP32)
