#include "eiosplatform.h"

#import <Foundation/Foundation.h>

std::string eIosDocumentsDir() {
    NSArray* paths = NSSearchPathForDirectoriesInDomains(
                NSDocumentDirectory, NSUserDomainMask, YES);
    NSString* dir = [paths firstObject];
    std::string result([dir UTF8String]);
    if(result.empty() || result.back() != '/') result += "/";
    return result;
}
