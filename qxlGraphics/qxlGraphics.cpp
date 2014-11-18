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
    
    LOG("with provider %p", provider);
    
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
    LOG("called");
    return kIOReturnSuccess;
}

void
qxlGraphics::logf(const char *function, const char *fmt, ...) {
    char buffer[512];
    va_list ap;
    
    buffer[0] = '\0';
    strlcat(buffer, function, sizeof buffer);
    strlcat(buffer, ": ", sizeof buffer);
    strlcat(buffer, fmt, sizeof buffer);
    
    va_start(ap, fmt);
    IOLog(buffer, ap);
    va_end(ap);
}
          
#pragma mark -
#pragma mark Attributes
#pragma mark -

IOReturn
qxlGraphics::getAttribute(IOSelect attribute, uintptr_t *value) {
    IOReturn ret = kIOReturnUnsupported;
    
    switch (attribute) {
        case kIOHardwareCursorAttribute:
            LOG("for kIOHardwareCursorAttribute");
            // we don't currently implement a HW cursor
            *value = 0;
            ret = kIOReturnSuccess;
            break;
            
        default:
            LOG("calling super for attrib %d", attribute);
            ret = super::getAttribute(attribute, value);
    }
    
    return ret;
}

IOReturn
qxlGraphics::setAttribute(IOSelect attribute, uintptr_t value) {
    IOReturn ret = kIOReturnUnsupported;
    
    switch (attribute) {
        default:
            LOG("calling super for attrib %d with value at %p", attribute, (void *)value);
            ret = super::setAttribute(attribute, value);
    }
    
    return ret;
}

IOReturn
qxlGraphics::getAttributeForConnection(IOIndex connectIndex, IOSelect attribute, uintptr_t *value) {
    IOReturn ret = kIOReturnUnsupported;
    
    switch (attribute) {
        case kConnectionSupportsHLDDCSense:
            LOG("for kConnectionSupportsHLDDCSense");
            // we support EDID queries
            *value = 1;
            ret = kIOReturnSuccess;
            break;
            
        default:
            LOG("calling super for attrib %d", attribute);
            ret = super::getAttributeForConnection(connectIndex, attribute, value);
    }
    return ret;
}

IOReturn
qxlGraphics::setAttributeForConnection(IOIndex connectIndex, IOSelect attribute, uintptr_t value) {
    IOReturn ret = kIOReturnUnsupported;
    
    switch (attribute) {
        default:
            LOG("calling super for attrib %d with value at %p", attribute, (void *)value);
            ret = super::setAttributeForConnection(connectIndex, attribute, value);
    }
    return ret;
}

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
    IOItemCount cnt = sizeof(_supported_modes)/sizeof(DisplayMode);
    LOG("%d", cnt);
    return cnt;
}

IOReturn
qxlGraphics::getDisplayModes(IODisplayModeID *allDisplayModes) {
    LOG("called");
    for (uint32_t i = 0; i < getDisplayModeCount(); ++i) {
        *allDisplayModes++ = i;
    }
    
    return kIOReturnSuccess;
}

IOReturn
qxlGraphics::getCurrentDisplayMode(IODisplayModeID *displayMode, IOIndex *depth) {
    LOG("mode %d, depth %d", _current_mode_id, _supported_depth);
    
    *displayMode = _current_mode_id;
    *depth = _supported_depth;
    
    return kIOReturnSuccess;
}

IOReturn
qxlGraphics::setDisplayMode(IODisplayModeID displayMode, IOIndex depth) {
    LOG("mode %d, depth %d", displayMode, depth);
    
    if (depth != _supported_depth)
        return kIOReturnBadArgument;
    
    _current_mode_id = displayMode;
    
    //TODO: inform qxl hardware of the mode change
    
    return kIOReturnSuccess;
}

IOReturn
qxlGraphics::getInformationForDisplayMode(IODisplayModeID displayMode, IODisplayModeInformation* info) {
    LOG("mode %d", displayMode);
    
    if (displayMode > getDisplayModeCount() - 1) {
        return kIOReturnBadArgument;
    }
 
    const DisplayMode *curMode = &_supported_modes[_current_mode_id];
    
    LOG("%dx%d flags 0x%08X", curMode->width, curMode->height, curMode->flags);
    
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
    LOG("called");
    return 0;
}

IOReturn
qxlGraphics::getPixelInformation(IODisplayModeID displayMode, IOIndex depth, IOPixelAperture aperture, IOPixelInformation* pixelInfo) {
    LOG("mode %d", displayMode);
    
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
    
    LOG("called");
    
    return pixelFormatStrings;
}

#pragma mark -
#pragma mark Framebuffer memory
#pragma mark -

IODeviceMemory *
qxlGraphics::getVRAMRange() {
    LOG("vram_bar_map is %p", _vram_bar_map);
    
    if (!_vram_bar_map)
        return 0;
    
    return IODeviceMemory::withSubRange(_vram_bar, 0U, _vram_bar_map->getSize());
}

IODeviceMemory*
qxlGraphics::getApertureRange(IOPixelAperture aperture) {
    LOG("for aperture 0x%08x", aperture);
    
    if (aperture != kIOFBSystemAperture)
        return 0;
    
    return getVRAMRange();
}

#pragma mark -
#pragma mark EDID support
#pragma mark -

IOReturn
qxlGraphics::getDDCBlock(IOIndex connectIndex, UInt32 blockNumber, IOSelect blockType, IOOptionBits options, UInt8 *data, IOByteCount *length) {
    //TODO: implement sending EDID data back
    LOG("called");
    return kIOReturnNoResources;
}

bool
qxlGraphics::hasDDCConnect(IOIndex connectIndex) {
    // we always provide EDID data
    LOG("called");
    return true;
}