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

std::filesystem::path MacFileManager::getResourcePath(const std::string name) const
{
    #ifdef SQUIRREL_RELEASE

    int slash = name.size() - 1;

    while (slash > 0 && name[slash] != '/')
    {
        slash--;
    }

    NSString* dir = nil;

    if (slash > 0)
    {
        dir = [ NSString stringWithUTF8String: name.substr(0, slash).c_str() ];
    }

    unsigned int dot = slash;

    while (dot < name.size() && name[dot] != '.')
    {
        dot++;
    }

    NSString* ext = nil;

    if (dot < name.size())
    {
        ext = [ NSString stringWithUTF8String: name.substr(dot + 1).c_str() ];
    }

    NSString* file = [ NSString stringWithUTF8String: name.substr(slash + 1, dot - slash - 1).c_str() ];
    NSString* path = [ [ NSBundle mainBundle ] pathForResource: file ofType: ext inDirectory: dir ];

    if (path)
    {
        return std::filesystem::path([ path cStringUsingEncoding: NSUTF8StringEncoding ]);
    }

    #endif

    return std::filesystem::path("resources") / name;
}
