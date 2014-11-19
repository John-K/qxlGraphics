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
#include <IOKit/graphics/IOGraphicsTypes.h>
#include <IOKit/graphics/IOFramebuffer.h>

class IOPCIDevice;
class IODeviceMemory;
class IOMemoryMap;

typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t flags;
// helpful flags values
#define DISP_FLAGS (kDisplayModeValidFlag | kDisplayModeSafeFlag)
#define DISP_FLAGS_DEFAULT (kDisplayModeValidFlag | kDisplayModeSafeFlag | kDisplayModeDefaultFlag)
} DisplayMode;

typedef struct {
    IOSelect type;
    IOFBInterruptProc callback;
    OSObject *target;
    void *ref;
    bool enabled;
} FBInterrupt;

class qxlGraphics : public IOFramebuffer {
    typedef IOFramebuffer super;
    
    IOPCIDevice * _provider;
    
    // PCI BAR mappings
    IODeviceMemory *_vram_bar;
    
    IOPhysicalAddress _io_base;
    IOPhysicalAddress _rom_base;
    IOPhysicalAddress _ram_base;
    IOPhysicalAddress _vram_base;
    
    IOMemoryMap *_io_bar_map;
    IOMemoryMap *_rom_bar_map;
    IOMemoryMap *_ram_bar_map;
    IOMemoryMap *_vram_bar_map;
    
    // QXL metadata
    QXLRom *_rom;
    QXLRam *_ram_header;
    
    // Current display mode information
    static const DisplayMode _supported_modes[];
    IODisplayModeID _current_mode_id;
    const IOIndex _supported_depth = 32;
    const uint32_t _refresh_60Hz = 60 << 16;
    
    // setup our logging facilities
    __attribute__((__format__ (__printf__, 3, 4)))
    void logf(const char *function, const char *fmt, ...);
#define LOG(x, ...) logf(__func__, x, ##__VA_ARGS__)
    
    // interrupt stuff
    FBInterrupt _interrupt;

public:
    // IOService routines
    bool start(IOPCIDevice *provider);
    void stop(IOPCIDevice *provider);
    
    // Setup
    IOReturn enableController(void);
    IOItemCount getConnectionCount();
    bool isConsoleDevice();
    
    // Attributes
    IOReturn getAttribute(IOSelect attribute, uintptr_t *value);
    IOReturn setAttribute(IOSelect attribute, uintptr_t value);
    IOReturn getAttributeForConnection(IOIndex connectIndex, IOSelect attribute, uintptr_t *value);
    IOReturn setAttributeForConnection(IOIndex connectIndex, IOSelect attribute, uintptr_t value);
    
    // Display Modes
    IOItemCount getDisplayModeCount(void);
    IOReturn getDisplayModes(IODisplayModeID *allDisplayModes);
    IOReturn setDisplayMode(IODisplayModeID displayMode, IOIndex depth);
    IOReturn getCurrentDisplayMode(IODisplayModeID *displayMode, IOIndex *depth);
    UInt64   getPixelFormatsForDisplayMode(IODisplayModeID displayMode, IOIndex depth);
    IOReturn getInformationForDisplayMode(IODisplayModeID displayMode, IODisplayModeInformation* info);
    IOReturn getPixelInformation(IODisplayModeID displayMode, IOIndex depth, IOPixelAperture aperture, IOPixelInformation* pixelInfo);

    // Pixel formats
    const char* getPixelFormats();
    
    // Framebuffer memory
    IODeviceMemory* getVRAMRange();
    IODeviceMemory* getApertureRange(IOPixelAperture aperture);
    
    // Cursor - for future acceleration support
    IOReturn setCursorState(SInt32 x, SInt32 y, bool visible);
    IOReturn setCursorImage(void* cursorImage);
    
    // Interrupts
    IOReturn setInterruptState(void* interruptRef, UInt32 state);
    IOReturn unregisterInterrupt(void* interruptRef);
    IOReturn registerForInterruptType(IOSelect interruptType, IOFBInterruptProc proc, OSObject* target, void* ref, void** interruptRef);
    
    // EDID
    IOReturn getDDCBlock(IOIndex connectIndex, UInt32 blockNumber, IOSelect blockType, IOOptionBits options, UInt8 *data, IOByteCount *length);
    bool hasDDCConnect(IOIndex connectIndex);
};