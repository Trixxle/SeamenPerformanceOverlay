<img width="1920" height="620" alt="HEROP_SPO" src="https://github.com/user-attachments/assets/0b8810ce-4f83-4a09-b29b-a6eb50a9bd4d" />

# Seamen Performance Overlay

Get in on [Steam!](https://store.steampowered.com/app/4666560/Seamen_Performance_Overlay/)

Has my tool been useful to you? Consider donating to my [ko-fi!](https://ko-fi.com/jornjorn)

## What is it?
The Seamen Performance Overlay is a native C++ SteamVR overlay that displays real-time frame time statistics, battery levels, and system resource usages! It automatically detects your headset's specifications to select the right frame time and frame rate to keep as reference.
The frame time consistency graph is a first among VR Overlays and allows users to detect issues that would otherwise go unnoticed. This graph alone has helped PICO XR troubleshoot their VR streaming application PICO Connect.

## Compatability
As long as you connect to SteamVR, this overlay *should* work with any headset and controllers. Or even without controllers!
For the time being the overlay only supports Windows. However, Linux support is planned.


## Gallery
UI                         |  Move it anywhere!
:-------------------------:|:-------------------------:
<img width="400" height="246" alt="Screenshot 2026-07-11 164456" src="https://github.com/user-attachments/assets/c2d22748-db18-4dfb-972c-a101b836b589" />  | <img width="384" height="216" alt="MOVEMENT" src="https://github.com/user-attachments/assets/dfdd2157-b8cc-46a7-b33e-32f963233d74" />

Switch Controllers!        |  Fades based on viewing angle!
:-------------------------:|:-------------------------:
<img width="384" height="216" alt="SWITCHCONTROLLER" src="https://github.com/user-attachments/assets/a1704bda-4af8-4024-afb9-6f95fb34bfa3" /> | <img width="400" height="262" alt="angleFade" src="https://github.com/user-attachments/assets/6af39476-3368-4f9f-9a57-fa0698ee89a3" />


## Installation Instructions
> [!NOTE]
> SteamVR must be installed for the overlay to work.

Go to the [releases](https://github.com/Trixxle/SeamenPerformanceOverlay/releases) and simply download the latest released .zip. Extract it anywhere, run SteamVR, and run the .exe within the downloaded folder.
Do not move files outside the folder, all files must remain in one folder.
The overlay will install itself to SteamVR the first time it is ran. The next time you launch SteamVR, the overlay will launch automatically. This can be changed inside the SteamVR settings under "Startup Overlay Apps". 
> [!IMPORTANT]
> If you have the Steam version of this overlay installed, it can interfere with the GitHub version and vice versa. It is recommended to keep one at a time.


## Features

### Graphs
The graphs are the main event on this overlay, but to properly read them it is important to understand them.
The graphs display 4 colors: blue, orange, red, and purple.
  - ${\color{lightblue}Blue}$ means optimal
  - ${\color{orange}Orange}$ means almost optimal
  - ${\color{red}Red}$ means nowhere near optimal
  - ${\color{purple}Purple}$ means a frame was dropped completely and never shown

<sub>* *Colorblindness options are available*</sub>

Some graphs will also display a dotted line, this line represents the value the bars have to stay under or at to display a frame with perfection.

### Statistics
  - GPU Frame time and respective graph
  - CPU Frame time and respective graph
  - Frame time consistency and respective graph
  - Precise frame rate and respective graph
  - Smoothened frame rate number for easy viewing
  - System RAM usage and total RAM count
  - System VRAM usage and total VRAM count
  - Headset battery level (if supported by headset)
  - Controllers battery level (if supported by controllers)
  - Trackers battery level (if supported by trackers)

### Accessibility
  - Can be moved anywhere around a controller
  - Can be attached to the left or right controller
  - Opacity controls
  - Size controls
  - Distance fade controls
  - Color blindness options

### Miscelaneous
  - Fades based on viewing angle
  - Will fall back to showing in front of view when no controllers are detected. Will attach to a controller if it appears mid-session with priority to the manually attached controller.
  - Saves everything about the overlay (opacity, scale, placement etc.) and restores it in future sessions
  - Works across AMD and Nvidia GPUs. (Intel Arc support is not 100% yet but should work)
  - Works across Intel and AMD CPUs
  - A second dashboard overlay with additional settings such as a reset button and the colorblindness options.
  - Dynamic high power and low power rendering modes. - When the SteamVR Dashboard is open the overlay will render at a higher quality to allow for smooth interactivity. When the SteamVR Dashbaord is closed the overlay switches back to its default low power rendering mode to use as little performance as possible

## Known bugs
All current known bugs can be found on the [Issues page.](https://github.com/Trixxle/SeamenPerformanceOverlay/issues)

## Building Instructions

### Prerequisites
  Qt Framework: You need the community version of Qt installed on your computer. You can download the Qt installer from [qt.io](https://my.qt.io/download). You will have to create a Qt account to access the downloads on that page.
This project is built using MINGW64 so you will need to install the MINGW64 version of Qt. Qt 6.11 is the version used to build this project but future or older versions can also work. Your mileage may vary. Make sure you install Qt in C:/Qt/6.11.0/mingw_64/. If you install it somewhere else, or a different version, change line 41 of CMakeLists.txt to the directory/version you installed it in.

### Instructions
Clone the repository. Either download the source code or run:
```bash
git clone https://github.com/Trixxle/SeamenPerformanceOverlay.git --recursive
```

> [!NOTE]
> This project requires steamworks API integration to build, but not to run.
As this overlay uses steam integration it requires the Steam SDK. This SDK is only accessible through Steam's partner program to which you can sign up for [here.](https://partner.steamgames.com/?ref=stebet.net).

he SDK should be included in the directory of this project like this: 

```bash
├── thirdparty
│   ├── steamworks
│   │   ├── public
│   │   │   ├── steam
│   │   ├── redistributable_bin
```

Build using your IDE of choice. This version is made to work with MinGW. For other compilers you will need to replace the respective DLL's for OpenVR.

It is planned to allow this project to be built without the steam API, for now this is not yet possible.

## Contact
For **feedback, bug reports**, and/or **feature requests**, join the Discord: https://discord.gg/j2skervRuZ 
You can also report issues on the [Issues page.](https://github.com/Trixxle/SeamenPerformanceOverlay/issues)

Website: https://seamen.gay

Corporate website: https://corporate.seamen.gay

Direct email: jorn@seamen.gay

Learn our language! Why would you want to? No idea - https://seamenese.com

