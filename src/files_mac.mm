#include "../include/files.h"

#import <AppKit/NSOpenPanel.h>
#import <AppKit/NSSavePanel.h>
#import <Foundation/NSString.h>
#import <Foundation/NSURL.h>

void MacFileManager::getSelectPath(SDL_Window* parent, SelectHandler complete) const
{
    SDL_PropertiesID properties = SDL_GetWindowProperties(parent);

    if (!properties)
    {
        complete("");

        return;
    }

    NSWindow* window = (NSWindow*)SDL_GetPointerProperty(properties, SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, nullptr);

    if (!window)
    {
        complete("");

        return;
    }

    NSOpenPanel* panel = [ NSOpenPanel openPanel ];

    [ panel setTitle: @"Select File" ];
    [ panel beginSheetModalForWindow: window completionHandler: ^(NSModalResponse response)
    {
        if (response == NSModalResponseOK)
        {
            complete([ [ panel URL ] fileSystemRepresentation ]);
        }

        else
        {
            complete("");
        }
    } ];
}

void MacFileManager::getSavePath(SDL_Window* parent, const std::string& name, SelectHandler complete) const
{
    SDL_PropertiesID properties = SDL_GetWindowProperties(parent);

    if (!properties)
    {
        complete("");

        return;
    }

    NSWindow* window = (NSWindow*)SDL_GetPointerProperty(properties, SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, nullptr);

    if (!window)
    {
        complete("");

        return;
    }

    NSSavePanel* panel = [ NSSavePanel savePanel ];

    [ panel setTitle: @"Save File" ];
    [ panel setNameFieldStringValue: [ NSString stringWithUTF8String: name.c_str() ] ];

    [ panel beginSheetModalForWindow: window completionHandler: ^(NSModalResponse response)
    {
        if (response == NSModalResponseOK)
        {
            complete([ [ panel URL ] fileSystemRepresentation ]);
        }

        else
        {
            complete("");
        }
    } ];
}

std::filesystem::path MacFileManager::getResourcePath(const std::string& name) const
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
