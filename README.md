# QuickNTP (libultrahand build)

---

### Update your Nintendo Switch clock using a list of NTP servers

When using a custom DNS (or a limited connection), it might not be possible to update your clock using Nintendo servers, causing the Switch to get out of sync easily.

With this [Tesla](https://github.com/WerWolv/libtesla) plugin, you can now quickly update your clock using your custom [NTP](https://en.wikipedia.org/wiki/Network_Time_Protocol) servers!

[![QuickNTP badge][version-badge]][changelog] [![GPLv2 License Badge][license-badge]][license] [![Build master branch and upload](https://github.com/nedex/QuickNTP/actions/workflows/build-master.yml/badge.svg)](https://github.com/nedex/QuickNTP/actions/workflows/build-master.yml)

---

![Preview](https://github.com/user-attachments/assets/0218a5e6-92be-44ed-9de1-af400927a493)

## Features

- Update the time by selecting from a list of servers ([customizable!](#customize-servers))
- Show the current offset compared to the selected server
- Set the internal network time to the time set by the user in system settings (time traveling, yay!)

### Customize servers

Since v1.5.0, you can create or modify the provided `quickntp.ini` file, with the following syntax:

```ini
[Servers]
My_Ntp_Server = ntp.example.com
```

Underscores will be replaced by spaces in the UI!

The homebrew will look for the file with the following order priority:
- /config/quickntp.ini
- /config/quickntp/config.ini
- /switch/.overlays/quickntp.ini

**Note**: If the file is missing or invalid, only the `NTP Pool Main` server will be shown.

## Possible future features

- Better error handling / messages
- Show a clock with seconds
- Turn the heart icon into a clock (need to check the font)
- Get the time in a separate thread
- Pick the closest NTP server based on user region
- Update the time with milliseconds (may be a system limitation)

## Contributors

- [@DarkMatterCore](https://github.com/DarkMatterCore) (library updates)
- [@DraconicNEO](https://github.com/DraconicNEO) (new NTP servers)

## Credits

### Special thanks

- [The 4TU Team](https://github.com/fortheusers) for freely hosting this app on their [Homebrew App Store](https://hb-app.store/)

### Code and libraries

- [@3096](https://github.com/3096) for `SwitchTime`, who gave me the initial idea and some code examples
- [@thedax](https://github.com/thedax) for `NX-ntpc`, used by [@3096](https://github.com/3096)
- [@SanketDG](https://github.com/SanketDG) for the [NTP Client](https://github.com/SanketDG/c-projects/blob/master/ntp-client.c) using `getaddrinfo` instead of `gethostbyname`
- [@WerWolv](https://github.com/WerWolv) for `libtesla`
- [@ITotalJustice](https://github.com/ITotalJustice) for `minIni-nx` (forked from [@compuphase](https://github.com/compuphase))

### Suggested NTP servers

- [NTP Pool Project](https://www.ntppool.org)
- [Cloudflare Time Services](https://www.cloudflare.com/time/)
- [Google Public NTP](https://developers.google.com/time)
- [NIST Internet Time Servers](https://tf.nist.gov/tf-cgi/servers.cgi)

## Troubleshooting

- The "Synchronize Clock via Internet" option should be enabled in System Settings [as described here](https://en-americas-support.nintendo.com/app/answers/detail/a_id/22557/p/989/c/188), since this program changes the "Network clock" (which is immutable in settings).
- If the only server shown is `NTP Pool Main`, there is a problem with the `quickntp.ini` file located in the `config` directory on the SD card. More details [here](#customize-servers).

## Disclaimer

- Please don't send a lot of requests to public servers. NTP servers should only be queried at 36-hour intervals.
- This program changes the NetworkSystemClock, which may cause a desync between the console and servers. Use at your own risk!

[version-badge]: https://img.shields.io/github/v/release/nedex/QuickNTP
[changelog]: ./CHANGELOG.md
[license-badge]: https://img.shields.io/github/license/nedex/QuickNTP
[license]: ./LICENSE
