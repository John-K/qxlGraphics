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

#pragma mark -
#pragma mark IOService Initialization and setup
#pragma mark -

bool
qxlGraphics::start(IOPCIDevice *provider) {
    IODeviceMemory *bar;
    
    if (!provider) {
        return false;
    }
    
    _provider = provider;
    
    provider->setMemoryEnable(true);
    provider->setIOEnable(true);
    
    // XXX: Apple says that we should do init in enableController instead of here
    
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

IOReturn
qxlGraphics::enableController( void ) {
    // XXX: should we be doing init here?
    
    return kIOReturnSuccess;
}

#pragma mark -
#pragma mark Attributes
#pragma mark -

#pragma mark -
#pragma mark Display Modes
#pragma mark -

DisplayMode const
qxlGraphics::_supported_modes[] = {
    1024,  768, DISP_FLAGS,	// 4:3
    1280,  720, DISP_FLAGS_DEFAULT,	// 16:9
    1920, 1080, DISP_FLAGS,	// 16:9
    1920, 1200, DISP_FLAGS,	// 16:10
};

IOItemCount
qxlGraphics::getDisplayModeCount() {
    return sizeof(_supported_modes)/sizeof(DisplayMode);
}

IOReturn
qxlGraphics::getDisplayModes(IODisplayModeID *allDisplayModes) {
    for (uint32_t i = 0; i < getDisplayModeCount(); ++i) {
        *allDisplayModes++ = i;
    }
    
    return kIOReturnSuccess;
}

IOReturn
qxlGraphics::getCurrentDisplayMode(IODisplayModeID *displayMode, IOIndex *depth) {
    *displayMode = _current_mode_id;
    *depth = _supported_depth;
    
    return kIOReturnSuccess;
}

IOReturn
qxlGraphics::setDisplayMode(IODisplayModeID displayMode, IOIndex depth) {
    if (depth != _supported_depth)
        return kIOReturnBadArgument;
    
    _current_mode_id = displayMode;
    
    //TODO: inform qxl hardware of the mode change
    
    return kIOReturnSuccess;
}

IOReturn
qxlGraphics::getInformationForDisplayMode(IODisplayModeID displayMode, IODisplayModeInformation* info) {
    if (displayMode > getDisplayModeCount() - 1) {
        return kIOReturnBadArgument;
    }
 
    const DisplayMode *curMode = &_supported_modes[_current_mode_id];
    
    info->maxDepthIndex = 0;
    info->nominalWidth = curMode->width;
    info->nominalHeight = curMode->height;
    info->flags = curMode->flags;
    info->refreshRate = _refresh_60Hz;
    
    return kIOReturnSuccess;
}


UInt64
qxlGraphics::getPixelFormatsForDisplayMode(IODisplayModeID displayMode, IOIndex depth) {
    // as per developer documentation:
    //     "IOFramebuffer subclasses must implement this method to return zero."
    
    return 0;
}

IOReturn
qxlGraphics::getPixelInformation(IODisplayModeID displayMode, IOIndex depth, IOPixelAperture aperture, IOPixelInformation* pixelInfo) {
    if (displayMode > getDisplayModeCount() - 1)
        return kIOReturnBadArgument;

    if (depth != _supported_depth)
        return kIOReturnBadArgument;

    // docs say that the aperture will always be kIOFBSystemAperture
    if (aperture != kIOFBSystemAperture)
        return kIOReturnBadArgument;

    bzero(pixelInfo, sizeof(IOPixelInformation));

    const DisplayMode *curMode = &_supported_modes[_current_mode_id];

    pixelInfo->activeWidth  = curMode->width;
    pixelInfo->activeHeight = curMode->height;
    pixelInfo->flags        = curMode->flags;
    pixelInfo->bitsPerPixel = _supported_depth;
    pixelInfo->componentMasks[0] = 0xFF0000;
    pixelInfo->componentMasks[1] = 0x00FF00;
    pixelInfo->componentMasks[2] = 0x0000FF;
    pixelInfo->componentCount    = 3;
    pixelInfo->bitsPerComponent  = 8;
    
    return kIOReturnSuccess;
}

#pragma mark -
#pragma mark Pixel Formats
#pragma mark -

const char *
qxlGraphics::getPixelFormats() {
    // We only support the 32-bit Pixel Format
    static char const pixelFormatStrings[] = IO32BitDirectPixels "\0";
    
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
