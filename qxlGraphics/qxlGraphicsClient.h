//
//  qxlGraphicsClient.h
//  qxlGraphics User Client
//
//  Created by John Kelley on 11/10/14.
//  Copyright (c) 2014 John Kelley. All rights reserved.
//

#pragma once

// Ignore "warning: 'register' storage class specifier is deprecated [-Wdeprecated-register]"
// in kern/queue.h
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-register"
#include <IOKit/IOUserClient.h>
#pragma clang diagnostic pop

class qxlGraphicsClient : public IOUserClient {
    OSDeclareDefaultStructors(qxlGraphicsClient);
    
public:
};