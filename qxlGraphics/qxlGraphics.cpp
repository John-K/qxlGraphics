//
//  qxlGraphics.c
//  qxlGraphics Framebuffer kext
//
//  Created by John Kelley on 11/10/14.
//  Copyright (c) 2014 John Kelley. All rights reserved.
//

// Ignore "warning: 'register' storage class specifier is deprecated [-Wdeprecated-register]"
// in kern/queue.h
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-register"
#include <IOKit/pci/IOPCIDevice.h>
#pragma clang diagnostic pop

#include <IOKit/IOMemoryDescriptor.h>
#include <spice/qxl_dev.h>
#include "qxlGraphics.h"

bool qxlGraphics::start(IOPCIDevice *provider) {
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
    _vram_bar = provider->getDeviceMemoryWithIndex(QXL_ROM_RANGE_INDEX);
    if (!_vram_bar) {
        return false;
    }
    _rom_bar_map = _vram_bar->createMappingInTask(kernel_task, 0, kIOMapAnywhere, 0, 0);
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

#pragma mark -
#pragma mark Pixel Formats
#pragma mark -

// We only support the 32-bit Pixel Format
static char const pixelFormatStrings[] = IO32BitDirectPixels "\0";

const char *
qxlGraphics::getPixelFormats() {
    return pixelFormatStrings;
}

#pragma mark -
#pragma mark Framebuffer memory
#pragma mark -

IODeviceMemory *
qxlGraphics::getVRAMRange() {
    if (!_vram_bar_map)
        return 0;
    return IODeviceMemory::withSubRange(_vram_bar, 0U, _vram_bar_map->getSize());
}
