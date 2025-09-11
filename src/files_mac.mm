#include "../include/files.h"

#import <AppKit/NSSavePanel.h>
#import <Foundation/NSString.h>
#import <Foundation/NSURL.h>

std::filesystem::path MacFileManager::getSavePath(const std::string name) const
{
    NSSavePanel* panel = [ NSSavePanel new ];

    [ panel setTitle: @"Save File" ];
    [ panel setNameFieldStringValue: [ NSString stringWithUTF8String: name.c_str() ] ];

    NSModalResponse response = [ panel runModal ];

    if (response == NSModalResponseOK)
    {
        return std::string([ [ panel URL ] fileSystemRepresentation ]);
    }

    return "";
}
