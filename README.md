# SOCD resolver
A simple and not-so-simple SOCD resolver for fast-paced FPS games.

## What is SOCD
SOCD stands for Simultaneous Opposite Cardinal Directions. It refers to the situation where 2 opposing movement keys are pressed at the same time, for instance A and D. This typically results in no (neutral) movement.

### What is SOCD resolution
This is the act of “resolving the conflict” between 2 opposing movement keys; which key should take priority?

There are a few resolution methods, but **in this specific repository, the SOCD resolution is based on “last input wins”.**

For example, that means that if you press A and then D, A will be de-activated immediately when D is pressed, and until D is released.

### Advantages of SOCD resolution
This specific example allows immediate strafing in games such as Counter-Strike 2 and Overwatch 2. This software is especially powerful in Overwatch due to immediate character acceleration and model shifting animations.

# Warning
The use of automation software is generally forbidden in competitive FPS titles. Use this at your own risk.

## Detections for AHK vs C
The AHK version is less risky as it is more recognised and uses less “suspicious” methods. The C version may have a slightly higher risk of detection.

## Warden (Overwatch)
No bans have been personally observed

## VALVe Anti-Cheat (Counter Strike 2)
No bans have been personally observed, but you may get kicked if you use this immediately in the match.

## Kernel anti-cheats
Some games like Valorant use a kernel anti-cheat, which may outright block the SOCD resolution from even working. **There is minimal risk of ban here.** In this case, your best bet is a keyboard with this feature.

# How to use
There are 2 SOCD resolvers included, along with their source code. All of them have been compiled for convenience.

## AHK version
AHK stands for AutoHotKey, an interpreted scripting engine to automate actions. This version’s source code is very minimal. 

### How to run
Simply run the “SOCD_AHK.exe” file and it will run in the background. It can be closed  by opening the system tray (^ in your taskbar) and right-clicking the program.

### Method
AHKv2 uses GetKeyState to read keyboard events and SendInput to inject synthetic input events

### Further optimisation
Since AHK is interpreted, the parsing complexity and amount affects the performance. 
If you wish to only have A-D strafing, remove the lines associated with W and S.

## C version
This variant has been made with the heavy use of A.I. It should in theory be much faster than the AHK script. Note that this version only supports A and D keys.

### How to run
Simply run the “SOCD_AD_C.exe” file. It will spawn a window that you can minimise. Closing this window will close the program.

### Method
Creates a message-only window (an invisible window) to receive keyboard messages and SendInput to inject synthetic input events. 

## SOCD tester
This is a complementary program that displays what WASD keys are pressed, and how fast the SOCD pairs were released. This can be used to ensure SOCD is working. 

### How to run
Simply run the “SOCD_tester.exe” file. 