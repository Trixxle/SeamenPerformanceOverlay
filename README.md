# Seamen Performance Overlay
<img width="16001" height="5168" alt="HeroSPO" src="https://github.com/user-attachments/assets/7b90c315-2965-48bd-864f-a3aaabc6b962" />


Get in on Steam! [ADD STEAMSTORE LINK]

## What is it?
The Seamen Performance Overlay is a native C++ (no yucky webapp) SteamVR overlay that displays real-time frame time statistics alongside RAM and VRAM usage! It automatically detects your headset's specifications to select the right frame time and frame rate to keep as reference.
The frame time consistency graph it has is a first among VR Overlays and allows users to detect issues that would otherwise go unnoticed. This graph alone has helped brands such as PICO troubleshoot their VR streaming application PICO Connect.

As long as you connect to SteamVR this overlay *should* work with any headset and controllers. Or even without controllers!


## Gallery:
UI                         |  Move it anywhere!
:-------------------------:|:-------------------------:
<img width="500" height="269" alt="Screenshot 2026-04-21 211733" src="https://github.com/user-attachments/assets/e253f596-63bd-469c-8941-3425f801629b" />  |  <img width="401" height="269" alt="MOVEMENT" src="https://github.com/user-attachments/assets/e84cf9d4-89f3-4aa9-bb62-5269260c6cf4" />

Switch Controllers!        |  Fades based on viewing angle!
:-------------------------:|:-------------------------:
<img width="440" height="288" alt="SWITCHCONTROLLER" src="https://github.com/user-attachments/assets/4e9630ba-a02f-493d-bca8-e6b042689aab" /> | <img width="440" height="288" alt="ANGLEFADE" src="https://github.com/user-attachments/assets/6af39476-3368-4f9f-9a57-fa0698ee89a3" />


## Installation Instructions:

Simply download the latest released .zip, extract it anywhere, run SteamVR, and run the .exe within the downloaded folder.
Do not move files outside the folder, all files must remain in one folder.
The overlay will install itself to SteamVR the first time it is ran. The next time you launch SteamVR the overlay will launch automatically. This can be changed inside the SteamVR settings under "Overlay Startup"


## Features:

### Graphs:
The graphs are the main event on this overlay, but to properly read them it is important to understant them.
The graphs display 4 colors: blue, orange, red, and pruple.
  - Blue means it is perfect
  - Orange means the performance was almost optimal
  - Red means the performance is nowehere near optimal
  - Purple means a frame was dropped completely and never shown

Some graphs will also display a dotted line, this line represents the value the bars have to stay under or at to display a frame with perfection.

### Statistics:
  - GPU Frame time and respective graph
  - CPU Frame time and respective graph
  - Frame time consistencty and respective graph
  - Accurate and precise framer rate and respective graph
  - Less accurate but more stable frame rate number for easy viewing
  - System RAM usage and total RAM count
  - System VRAM usage and total VRAM count

### Interactivity:
  - Can be moved anywhere around a controller
  - Can be attached to the left or right controller
  - Opacity controls
  - Size controls
  - Exit button

### Miscelaneous features:
  - Fades based on viewing angle
  - Will fall back to showing in front of view when no controllers are detected. Will attach to controller if it appears mid-session
  - Saves everything about the overlay (opacity, scale, placement etc.) and restores it in future sessions
  - Works across AMD and Nvidia GPUs. (Probably Intel too but not tested)
  - Works across Intel and AMD CPUs
  - A Second dashboard overlay with a "SAVE ME!" button that resets the overlay to default and respawns it above your left or right controller, or HMD when there are no controllers

## Known bugs:
  - Sometimes the icon on the Steam Dashboard will not load (could also be a SteamVR bug)
  - When scaling the overlay to impractical uses the fading based on viewing angle stops working correctly

## Planned features:
  - Linux support (I need a Linux distro to test with)

## Building Instructions

### Prerequisites:
  Qt Framework: You need the community version of Qt installed on your computer. You can download the Qt installer from [qt.io](https://my.qt.io/download). You will have to create a Qt account to access the downloads on that page.
This project is built using MINGW64 so you will need to install the MINGW64 version of Qt.
Qt 6.11 is the version used to build this project but future or older versions can also work. Your mileage may vary.
Make sure you install Qt in C:/Qt/6.11.0/mingw_64/. If you install it somewhere else, change line 41 of CMakeLists.txt to the directory you installed it in.

### Instructions:
Clone the repository. Either download the source code or run:
```
git clone https://github.com/Trixxle/SeamenPerformanceOverlay.git --recursive
```

You can either build using your IDE of choice or by opening a terminal, navigating to the root folder of the code and running:
```
[UNFINISHED]
```
