//
//  qxlGraphics.h
//  qxlGraphics Framebuffer kext
//
//  Created by John Kelley on 11/10/14.
//  Copyright (c) 2014 John Kelley. All rights reserved.
//

#pragma once

#include <stdint.h>
#include <sys/types.h>

class IOPCIDevice;
class IODeviceMemory;
class IOMemoryMap;

class qxlGraphics {
    IOPCIDevice * _provider;
    
    IOPhysicalAddress _io_base;
    IOPhysicalAddress _rom_base;
    IOPhysicalAddress _ram_base;
    IOPhysicalAddress _vram_base;
    
    IOMemoryMap *_io_bar_map;
    IOMemoryMap *_rom_bar_map;
    IOMemoryMap *_ram_bar_map;
    IOMemoryMap *_vram_bar_map;
    
    QXLRom *_rom;
    QXLRam *_ram_header;

public:
    bool Init();
    bool Start(IOPCIDevice *provider);
    
};