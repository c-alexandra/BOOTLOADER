# BOOTLOADER

to add libopencm3 submodule to project, in root folder, run
```git submodule add https://github.com/libopencm3/libopencm3.git ```

to initialize submodule
```git submodule update --init```

to add st-link debug functionality to macOS, I use homebrew and
```brew install stlink```

to configure intellisense for library headers, add hardware target to project
configuration; that is, add to c_cpp_properties.json defines,
```STM32F4```

to identify device, run  
```st-info --descr```  
to erase program from device  
```st-flash erase```
to write bin file to memory on device  
```st-flash write app/firmware.bin 0x08000000```

where 0x08000000 is replaced with start of flash memory block

to restart/power cycle chip  
```st-flash --reset```


to begin a debug session from the terminal
start a gdb server  
```st-util```

then, in another terminal,
```arm-none-eabi app/firmware.elf```  
```target extended localhost:4242```  
```load```  
```continue```  

Git commands to delete and rename branches
To delete a local branch,
```git branch -d <branchname>```
The '-d' option is an aliad for '--delete', which only deletes branches which have been fully merged in its upstream branch

```git branch -D <branchname>```
is an alias for '--delete --force' which deletes a branch irrespective of its merge status

To delete a remote branch,
```git push -d <origin> <branchname>```

or ```git push <remote_name> :<branch_name>

to rename branch,
```git branch (-m | -M) [<oldbranch>] <newbranch>```

or if on current branch, 
```git branch -m <newbranch>```

to view all branches
```git branch -a```

To Implement the communication protocol for flashing new firmware, we will use typescript and the serialport dependency

To check typescript version,
```npm list -g```

At time of writing, latest serialport package is 13.0.0

