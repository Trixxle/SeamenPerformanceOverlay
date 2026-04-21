<img width="16001" height="5168" alt="HeroSPO" src="https://github.com/user-attachments/assets/7b90c315-2965-48bd-864f-a3aaabc6b962" />

# Seamen Performance Overlay

## Features:

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


## Installation Instructions:

Simply download the latest released .zip, extract it anywhere, run SteamVR, and run the .exe within the downloaded folder.
Do not move files outside the folder, all files must remain in one folder.
The overlay will install itself to SteamVR the first time it is ran. The next time you launch SteamVR the overlay will launch automatically. This can be changed inside the SteamVR settings under "Overlay Startup"


## Building Instructions

### Prerequisites:
  Qt Framework:
You need the community version of Qt installed on your computer. You can download it from [qt.io](https://my.qt.io/download). You will have to create a Qt account to access the downloads on that page.
This project is built using MINGW64 so you will need to download the MINGW64 version of Qt.
Qt 6.11 is the version used to build this project but future or older versions can also work. Your mileage may vary.

### Instructions:
Make sure you install Qt in C:/Qt/6.11.0/mingw_64/. If you install it somewhere else, change line 41 of CMakeLists.txt to the directory you installed it in.
