//
//  qxlGraphics.c
//  qxlGraphics Framebuffer kext
//
//  Created by John Kelley on 11/10/14.
//  Copyright (c) 2014 John Kelley. All rights reserved.
//

#include <IOKit/pci/IOPCIDevice.h>
#include <IOKit/IOMemoryDescriptor.h>
#include <spice/qxl_dev.h>
#include "qxlGraphics.h"

bool qxlGraphics::Start(IOPCIDevice *provider) {
    IODeviceMemory *bar;
    
    if (!provider) {
        return false;
    }
    
    _provider = provider;
    
    provider->setMemoryEnable(true);
    provider->setIOEnable(true);
    
    // get and map IO address
    bar = provider->getDeviceMemoryWithIndex(QXL_IO_RANGE_INDEX);
    if (!bar) {
        return false;
    }
    _io_bar_map = bar->createMappingInTask(kernel_task, 0, kIOMapAnywhere, 0, 0); //getPhysicalAddress();
    if (!_io_bar_map) {
        return false;
    }
    _io_base = _io_bar_map->getVirtualAddress();
    
    // get and map ROM address
    bar = provider->getDeviceMemoryWithIndex(QXL_ROM_RANGE_INDEX);
    if (!bar) {
        return false;
    }
    _rom_bar_map = bar->createMappingInTask(kernel_task, 0, kIOMapAnywhere, 0, 0);
    _rom_base = _rom_bar_map->getVirtualAddress();
    _rom = (QXLRom *)_rom_base;
    if (_rom->magic != QXL_ROM_MAGIC) {
        return false;
    }
    
    // get and map RAM address
    bar = provider->getDeviceMemoryWithIndex(QXL_RAM_RANGE_INDEX);
    if (!bar) {
        return false;
    }
    _ram_bar_map = bar->createMappingInTask(kernel_task, 0, kIOMapAnywhere, 0, 0);
    _ram_base = _ram_bar_map->getVirtualAddress();
    _ram_header = (QXLRam *)_ram_base + _rom->ram_header_offset;
    if (_ram_header->magic != QXL_RAM_MAGIC) {
        return false;
    }
    // set physical addr?
    // set size?

    // get and map VRAM address
    bar = provider->getDeviceMemoryWithIndex(QXL_VRAM_RANGE_INDEX);
    if (!bar) {
        return false;
    }
    _vram_bar_map = bar->createMappingInTask(kernel_task, 0, kIOMapAnywhere, 0, 0);
    _vram_base = _vram_bar_map->getVirtualAddress();
    
    //TODO: setup memory regions
    //TODO: setup ring buffers
    return true;
}

