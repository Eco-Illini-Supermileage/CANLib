CANLib FDCAN Wrapper
Version 1.0

How to Install into your STM32Cube project
1. Copy this directory into your project
2. In STM32CubeIDE,
   * Open the CANLib folder, right-click the Inc folder,
     and click 'Add/remove include path'
   * Right-click the examples folder and click 
     'Resource Configurations -> Exclude from build'
   * In the project properties under 'C/C++ General ->
     Paths and Symbols' in the 'Source Location' tab,
     add the CANLib/Src directory.
3. Configure an FDCAN peripheral in your IOC file.
4. Read the canlib.h file and the example code for how to
   use the library.
