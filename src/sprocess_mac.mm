#include "../include/sprocess.h"

#import <Foundation/NSBundle.h>

std::string MacProcessManager::getExecutablePath() const
{
    return [ [ [ NSBundle mainBundle ] executablePath ] cStringUsingEncoding: NSUTF8StringEncoding ];
}
